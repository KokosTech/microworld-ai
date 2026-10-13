#include "ai.hpp"

#include "SFML/Window/Keyboard.hpp"

/***************************************************************
AI CLASS DEFINITION
*/
AI::AI() {}

AI::AI(unsigned id, unsigned agent_speed, std::mt19937_64* rng, Symbols symbols,
       Costs costs, int max_turn)
    : id(id),
      agent_speed(agent_speed),
      rng(rng),
      symbols(symbols),
      costs(costs),
      max_turn(max_turn),
      clock(0),
      rotation(0),
      agent_position({DEFAULT_MAP_SIZE / 2, DEFAULT_MAP_SIZE / 2}),
      agent_map(DEFAULT_MAP_SIZE,
                std::vector<std::string>(DEFAULT_MAP_SIZE, UNEXPLORED_SYMBOL)) {
}

void AI::PrintMap() {
  for (auto row : agent_map) {
    for (auto cell : row) {
      if (cell == UNEXPLORED_SYMBOL) {
        std::cout << cell << " ";
      } else if (cell == SAFE_SYMBOL) {
        std::cout << "\033[1;42m" << cell << "\033[0m ";
      } else {
        std::cout << "\033[1;41m" << cell << "\033[0m ";
      }
    }
    std::cout << std::endl;
  }
}

void AI::PrintPercepts(const Percepts& percepts) {
  std::cout << "------------------ MAP ----------------------\n";
  PrintMap();
  std::cout << "---------------------------------------------\n";
  std::cout << "AGENT ID: " << id << std::endl;
  std::cout << "AGENT POS: " << agent_position.x << "," << agent_position.y
            << std::endl;
  std::cout << "AGENT ROT: " << rotation << std::endl;
  std::cout << "DISTANCE: " << percepts.detector << std::endl;
  std::cout << "CURRENT:  " << percepts.current[0] << std::endl;
  std::cout << "FORWARD:  ";
  for (std::vector<std::string>::const_iterator it = percepts.forward.begin();
       it != percepts.forward.end(); it++)
    std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "LEFT:     ";
  for (std::vector<std::string>::const_iterator it = percepts.left.begin();
       it != percepts.left.end(); it++)
    std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "BACKWARD: ";
  for (std::vector<std::string>::const_iterator it = percepts.backward.begin();
       it != percepts.backward.end(); it++)
    std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "RIGHT:    ";
  for (std::vector<std::string>::const_iterator it = percepts.right.begin();
       it != percepts.right.end(); it++)
    std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "Others:\n";
  for (size_t i = 0; i < percepts.others.size(); i++) {
    std::cout << "   " << i << ": " << percepts.others[i].to_string()
              << std::endl;
  }
}

void AI::AddToMap(Coordinates cor, Percepts& percepts) {
  auto in_bounds = [](int x, int y) {
    return x >= 0 && y >= 0 && x < DEFAULT_MAP_SIZE && y < DEFAULT_MAP_SIZE;
  };
  auto convert = [&](const std::string& s) {
    return s == symbols.open ? SAFE_SYMBOL : s;
  };

  // current cell
  if (in_bounds(cor.x, cor.y) && !percepts.current.empty()) {
    agent_map[cor.y][cor.x] = convert(percepts.current[0]);
  }

  // forward vector for rotation: 0 up, 1 right, 2 down, -1 left
  int dx = 0, dy = 0;
  switch (rotation) {
    case 0:
      dx = 0;
      dy = -1;
      break;
    case 1:
      dx = 1;
      dy = 0;
      break;
    case 2:
      dx = 0;
      dy = 1;
      break;
    case -1:
      dx = -1;
      dy = 0;
      break;
    default:
      return;  // invalid rotation, don't corrupt the map
  }

  auto scan = [&](int sx, int sy, const std::vector<std::string>& seen) {
    for (size_t i = 0; i < seen.size(); ++i) {
      int x = cor.x + sx * (int)(i + 1);
      int y = cor.y + sy * (int)(i + 1);
      if (!in_bounds(x, y)) break;
      agent_map[y][x] = convert(seen[i]);
    }
  };

  scan(dx, dy, percepts.forward);
  scan(-dx, -dy, percepts.backward);
  scan(dy, -dx, percepts.left);   // left of (dx,dy)
  scan(-dy, dx, percepts.right);  // right of (dx,dy)
}

std::vector<std::string> AI::ExecuteCommands(std::vector<std::string> cmds,
                                             Percepts& percepts) {
  std::cout << "CMDS:      ";

  for (std::vector<std::string>::const_iterator it = cmds.begin();
       it != cmds.end(); it++)
    std::cout << *it << " ";

  std::cout << std::endl;

  ++this->clock;
  for (const auto& cmd : cmds) {
    if (cmd == RIGHT) {
      rotation = (rotation + 1) % 4;

      if (rotation == 3) rotation = -1;

    }

    else if (cmd == LEFT) {
      rotation = (rotation + 3) % 4;

      if (rotation == 2) rotation = 2;

    }

    else if (cmd == FORWARD) {
      switch (rotation) {
        case 0:
          agent_position.y--;
          break;

        case 1:
          agent_position.x++;
          break;

        case -1:
          agent_position.x--;
          break;

        case 2:
          agent_position.y++;
          break;
      }

    }

    else if (cmd == BACKWARD) {
      switch (rotation) {
        case 0:
          agent_position.y++;
          break;

        case 1:
          agent_position.x--;
          break;

        case -1:
          agent_position.x++;
          break;

        case 2:
          agent_position.y--;
          break;
      }
    }
  }
  // this->prev_distance = percepts.detector;
  // this->prev_cmd = cmds[-1];
  return cmds;
}

std::vector<std::string> AI::Run(Percepts& percepts, AgentComm* comms) {
  // MISC PRINT

  std::cout << "------------------------------------------------\n";
  std::cout << "AGENT ID: " << id << std::endl;
  PrintPercepts(percepts);

  // init map from beginning / middle
  AddToMap(agent_position, percepts);

  // LOGIC
  if (percepts.forward[0] == symbols.disarmed_mine && tried_disarming) {
    tried_disarming = false;
  }

  // STATE 0 - initial mapping (turning around 270) - will be used when mapping
  // (rn not)
  if (clock < 3) {
    // add additional checks if they are known walls
    return ExecuteCommands({RIGHT}, percepts);
  }

  // get treasure
  if (std::find(percepts.current.begin(), percepts.current.end(),
                symbols.treasure) != percepts.current.end()) {
    return ExecuteCommands({TREASURE}, percepts);
  }

  // command you intend to execute - add some randomness for fun testing
  std::uniform_int_distribution<> distr(0, 10);
  std::string intention;
  int num = distr(*rng);
  if (num < 8) {
    intention = FORWARD;
  } else if (num == 8) {
    intention = RIGHT;
  } else if (num == 9) {
    intention = LEFT;
  } else {
    intention = BACKWARD;
  }

  // if (prev_distance > percepts.detector) {
  //   intention = prev_cmd;
  // } else if (prev_distance < percepts.detector) {
  //   intention = BACKWARD;
  // } else if (prev_distance == percepts.detector && (prev_cmd == FORWARD ||
  // prev_cmd == BACKWARD)) {
  //   intention = num % 2 ? RIGHT : LEFT;
  // }

  // std::string seen_treasure_direction;

  if (!percepts.forward.empty()) {
    if (std::find(percepts.forward.begin(), percepts.forward.end(),
                  symbols.treasure) != percepts.forward.end()) {
      intention = FORWARD;
    }

    if (percepts.forward[0] == symbols.wall) {
      std::uniform_int_distribution<> distr(0, 1);
      intention = distr(*rng) ? LEFT : RIGHT;
    }

    if (percepts.forward[0] == symbols.treasure) {
      return ExecuteCommands({FORWARD}, percepts);
    }
  }
  if (!percepts.backward.empty()) {
    if (std::find(percepts.backward.begin(), percepts.backward.end(),
                  symbols.treasure) != percepts.backward.end()) {
      intention = BACKWARD;
    }

    if (percepts.backward[0] == symbols.treasure) {
      return ExecuteCommands({BACKWARD}, percepts);
    }
  }

  if (!percepts.left.empty()) {
    if (std::find(percepts.left.begin(), percepts.left.end(),
                  symbols.treasure) != percepts.left.end()) {
      intention = LEFT;
    }
  }
  if (!percepts.right.empty()) {
    if (std::find(percepts.right.begin(), percepts.right.end(),
                  symbols.treasure) != percepts.right.end()) {
      intention = RIGHT;
    }
  }

  std::vector<std::string> safe_symbols = {symbols.treasure, symbols.wall,
                                           symbols.disarmed_mine};
  for (auto i : symbols.teleporters) {
    safe_symbols.push_back(i);
  }

  if (percepts.detector == 1 && !tried_disarming) {
    if (percepts.forward[0] == symbols.wall ||
        percepts.forward[0] == symbols.disarmed_mine) {
      // intention = RIGHT;
      return ExecuteCommands({RIGHT}, percepts);
    } else {
      // intention = DISARM;
      tried_disarming = true;
      return ExecuteCommands({DISARM}, percepts);
    }
  } else if (percepts.detector == 1 && tried_disarming) {
    // intention = RIGHT;
    tried_disarming = false;
    return ExecuteCommands({RIGHT}, percepts);
  }

  // i wonder how this will affect the agent - probably loops
  if (intention == RIGHT && percepts.right[0] == symbols.wall) {
    intention = LEFT;
  } else if (intention == LEFT && percepts.left[0] == symbols.wall) {
    intention = RIGHT;
  }

  // EXECUTE COMMAND
  return ExecuteCommands({intention}, percepts);
}
