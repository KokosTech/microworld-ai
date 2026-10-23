// formatting used - google clang in clion

#include "ai.hpp"

#include <algorithm>

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
                std::vector<std::string>(DEFAULT_MAP_SIZE, UNEXPLORED_SYMBOL)),
      agent_safe(DEFAULT_MAP_SIZE, std::vector<int>(DEFAULT_MAP_SIZE, 0)),
      no_traps_left(false),
      pending_teleport(false),
      looking(false),
      look_cell({0, 0}),
      probing(false),
      probe_cell({0, 0}),
      has_target(false),
      explore_target({0, 0}) {}

// MAPPING METHODS

void AI::PrintPercepts(const Percepts& percepts) {
  std::cout
      << "\033[2J\033[1;1H";  // for smoother transition of the map printed
  std::cout << "------------------ MAP ----------------------\n";
  PrintMap(agent_map);
  std::cout << "---------------------------------------------\n";
  std::cout << "------------------ SAFE MAP ----------------------\n";
  PrintMap(agent_safe);
  std::cout << "---------------------------------------------\n";
  std::cout << "AGENT ID: " << id << std::endl;
  std::cout << "AGENT POS: " << agent_position.x << "," << agent_position.y
            << std::endl;
  std::cout << "AGENT ROT: " << rotation << std::endl;
  std::cout << "DISTANCE: " << percepts.detector << std::endl;
  if (!percepts.current.empty())
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

template <typename T>
void AI::PrintMap(const std::vector<std::vector<T>>& map) {
  const char* arrows[4] = {"^", ">", "v", "<"};
  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      const T& cell = map[y][x];
      // check if type is str - didnt even know it was possible
      // https://stackoverflow.com/questions/13636540/how-to-check-for-the-type-of-a-template-parameter
      // https://en.cppreference.com/cpp/types/is_same
      if constexpr (std::is_same_v<T, std::string>) {
        if (x == agent_position.x && y == agent_position.y) {
          // show the agent and its facing direction
          std::cout << "\033[1;46m" << arrows[rotation] << "\033[0m ";
        } else if (cell == UNEXPLORED_SYMBOL) {
          std::cout << cell << " ";
        } else if (cell == symbols.wall) {
          std::cout << "\033[1;41m" << cell << "\033[0m ";
        } else if (cell == SAFE_SYMBOL) {
          std::cout << "\033[1;42m" << cell << "\033[0m ";
        } else if (cell == UNREACHABLE_SYMBOL) {
          std::cout << "\033[1;47m" << cell << "\033[0m ";
        } else {
          std::cout << "\033[1;44m" << cell << "\033[0m ";
        }
      } else {
        std::cout << cell << " ";
      }
    }
    std::cout << std::endl;
  }
}

bool AI::IsInBounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < DEFAULT_MAP_SIZE && y < DEFAULT_MAP_SIZE;
}

std::string AI::ConvertSymbol(const std::string& symbol) const {
  return symbol == symbols.open ? std::string(SAFE_SYMBOL) : symbol;
}

void AI::ScanDirection(Coordinates cor, int dir,
                       const std::vector<std::string>& seen) {
  for (size_t i = 0; i < seen.size(); ++i) {
    int x = cor.x + DX[dir] * static_cast<int>(i + 1);
    int y = cor.y + DY[dir] * static_cast<int>(i + 1);
    if (!IsInBounds(x, y)) break;
    if (agent_map[y][x] != SAFE_SYMBOL) agent_map[y][x] = seen[i];
  }
}

void AI::AddToMap(Coordinates cor, Percepts& percepts) {
  // current cell
  if (IsInBounds(cor.x, cor.y) && !percepts.current.empty()) {
    agent_map[cor.y][cor.x] = ConvertSymbol(percepts.current[0]);
  }

  // direction / percepts cells
  ScanDirection(cor, rotation, percepts.forward);
  ScanDirection(cor, (rotation + 2) % 4, percepts.backward);
  ScanDirection(cor, (rotation + 3) % 4, percepts.left);
  ScanDirection(cor, (rotation + 1) % 4, percepts.right);
}

std::vector<std::string> AI::ExecuteCommands(std::vector<std::string> cmds,
                                             Percepts& percepts) {
  std::cout << "CMDS:      ";
  for (size_t i = 0; i < cmds.size(); ++i) std::cout << cmds[i] << " ";
  std::cout << std::endl;

  ++this->clock;
  for (size_t i = 0; i < cmds.size(); ++i) {
    const std::string& cmd = cmds[i];
    if (cmd == RIGHT) {
      rotation = (rotation + 1) % 4;
    } else if (cmd == LEFT) {
      rotation = (rotation + 3) % 4;
    } else if (cmd == FORWARD || cmd == BACKWARD) {
      int direction = rotation;

      // Backward movement uses the opposite direction.

      if (cmd == BACKWARD) {
        direction = (rotation + 2) % 4;
      }

      int next_x = agent_position.x + DX[direction];
      int next_y = agent_position.y + DY[direction];

      // Check map boundaries.

      if (!IsInBounds(next_x, next_y)) {
        continue;
      }

      // Check whether the destination is known to be a wall.

      if (agent_map[next_y][next_x] == symbols.wall) {
        continue;
      }

      agent_position.x = next_x;
      agent_position.y = next_y;
    } else if (cmd == USE) {
      // We only know where we land when the next percepts arrive.
      if (!percepts.current.empty() && IsTeleporter(percepts.current[0])) {
        pending_teleport = true;
        pending_from = percepts.current[0];
      }
    }
  }
  return cmds;
}

// PATH FINDING

//...

// =================================================
// === ENTRY POINT FOR AI
// =================================================

std::vector<std::string> AI::Run(Percepts& percepts, AgentComm* comms) {
  // handle teleport landing (aka new map)
  HandleLanding(percepts);

  // add percepts to map
  AddToMap(agent_position, percepts);

  // handle trap logic
  MarkSafe(percepts);

  // merge teleport frames - to be done in the future (for now the map is reset)
  // todo

  // read what the other rogues sent
  // ReadPeers(percepts, comms);
  // PrintPercepts(percepts);

  // decision
  // int old_rotation = rotation;  // the heading our percepts were taken from
  std::vector<std::string> cmds = Decide(percepts);

  // publish comm (send to other rogues)
  // Publish(percepts, comms, old_rotation, cmds);
  return cmds;
}