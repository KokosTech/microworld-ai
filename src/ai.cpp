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
                std::vector<std::string>(DEFAULT_MAP_SIZE, UNEXPLORED_SYMBOL)) {
}

// MAPPING METHODS

void AI::PrintPercepts(const Percepts& percepts) {
  std::cout
      << "\033[2J\033[1;1H";  // for smoother transition of the map printed
  std::cout << "------------------ MAP ----------------------\n";
  PrintMap();
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

void AI::PrintMap() {
  const char* arrows[4] = {"^", ">", "v", "<"};
  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      const std::string& cell = agent_map[y][x];
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

  for (const auto& c : cmds) std::cout << c << " ";
  std::cout << std::endl;

  ++this->clock;
  for (const auto& cmd : cmds) {
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

      if (next_x < 0 || next_y < 0 || next_x >= DEFAULT_MAP_SIZE ||
          next_y >= DEFAULT_MAP_SIZE) {
        continue;
      }

      // Check whether the destination is known to be a wall.

      if (agent_map[next_y][next_x] == symbols.wall) {
        continue;
      }

      agent_position.x = next_x;
      agent_position.y = next_y;
    }
  }
  // this->prev_distance = percepts.detector;
  // this->prev_cmd = cmds[-1];
  return cmds;
}

// =================================================
// === ENTRY POINT FOR AI
// =================================================

std::vector<std::string> AI::Run(Percepts& percepts, AgentComm* comms) {
  // handle teleport landing (aka new frame)
  // add percepts to map
  // handle trap logic
  // merge teleport frames - to be done in the future
  // decision
  // publish comm (send to other rogues
}