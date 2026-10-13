#pragma once

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "comm.hpp"
#include "percepts.hpp"

#define RIGHT "R"
#define LEFT "L"
#define FORWARD "F"
#define BACKWARD "B"
#define USE "U"
#define TREASURE "T"
#define DISARM "D"

// struct ExtendedSymbols: public Symbols {
//
// };

#define UNEXPLORED_SYMBOL "?"
#define POTENTIAL_TRAP_SYMBOL "P"
#define SAFE_SYMBOL "S"  // agent is SURE there is no trap on it, or is on it
#define SELF_SYMBOL "A"
// other agents would be just number of the agent

#define DEFAULT_MAP_SIZE 70

struct Coordinates {
  unsigned short x;
  unsigned short y;
};

class AI {
 protected:
  // Necessary, do not delete.
  unsigned id;
  unsigned agent_speed;
  std::mt19937_64 *rng;
  Symbols symbols;
  // ExtendedSymbols extended_symbols;
  Costs costs;
  int max_turn;

  // additional data
  unsigned num_turns;
  Coordinates agent_position;  // in the begining puts it in the middle
  int rotation = 0;  // 0 - initial, -1 - left, 1 - right, 2 - backwards
  unsigned clock;
  bool tried_disarming = false;
  int prev_distance = -1;
  std::string prev_cmd;

  // data structure for map
  std::vector<std::vector<std::string>> agent_map;
  // helper methods
  void PrintMap();
  void AddToMap(Coordinates cor, Percepts &percepts);
  std::vector<std::string> ExecuteCommands(std::vector<std::string> cmds,
                                           Percepts &percepts);

  // right now is void tbc
  void EncodeCommunications(AgentComm *comms);
  void DecodeCommunications(AgentComm *comms);

 public:
  AI();
  AI(unsigned id, unsigned agent_speed, std::mt19937_64 *rng, Symbols symbols,
     Costs costs, int max_turn);
  void PrintPercepts(const Percepts &percepts);
  std::vector<std::string> Run(Percepts &percepts, AgentComm *comms);
};
