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
      clock(0) {}

void AI::PrintPercepts(const Percepts& percepts) {
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

std::vector<std::string> AI::ExecuteCommands(std::vector<std::string> cmds, Percepts& percepts) {
  std::cout << "CMDS:      ";

  for (std::vector<std::string>::const_iterator it = cmds.begin();
       it != cmds.end(); it++)
    std::cout << *it << " ";

  std::cout << std::endl;

  ++this->clock;
  this->prev_distance = percepts.detector;
  this->prev_cmd = cmds[-1];
  return {cmds};
}

std::vector<std::string> AI::Run(Percepts& percepts, AgentComm* comms) {
  // MISC PRINT

  std::cout << "------------------------------------------------\n";
  std::cout << "AGENT ID: " << id << std::endl;
  PrintPercepts(percepts);

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
  // } else if (prev_distance == percepts.detector && (prev_cmd == FORWARD || prev_cmd == BACKWARD)) {
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
