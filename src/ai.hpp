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

class AI {
 protected:
  // Necessary, do not delete.
  unsigned id;
  unsigned agent_speed;
  std::mt19937_64 *rng;
  Symbols symbols;
  Costs costs;
  int max_turn;

  // additional data
  unsigned clock;
  bool tried_disarming = false;
  int prev_distance = -1;

  // data structure for map

  // helper methods

  std::vector<std::string> ExecuteCommands(std::vector<std::string> cmds, Percepts& percepts);

 public:
  AI();
  AI(unsigned id, unsigned agent_speed, std::mt19937_64 *rng, Symbols symbols,
     Costs costs, int max_turn);
  void PrintPercepts(const Percepts &percepts);
  std::vector<std::string> Run(Percepts &percepts, AgentComm *comms);
};
