#pragma once

#include<algorithm>
#include<string>
#include<random>
#include<map>
#include<cstdlib>
#include<iostream>
#include<fstream>
#include"percepts.hpp"
#include"comm.hpp"

class AI {
protected:
  // Necessary, do not delete.
  unsigned id;
  unsigned agent_speed;
  std::mt19937_64* rng;
  Symbols symbols;
  Costs costs;
  int max_turn;
public:
  AI();
  AI(
     unsigned id, 
     unsigned agent_speed,
     std::mt19937_64* rng,
     Symbols symbols,
     Costs costs,
     int max_turn);
  void PrintPercepts(const Percepts & percepts);
  std::vector<std::string> Run(
			       Percepts & percepts,
			       AgentComm * comms);
};



