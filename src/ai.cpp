#include"ai.hpp"

/***************************************************************
AI CLASS DEFINITION
*/
AI::AI() {}
AI::AI(
    unsigned id, 
    unsigned agent_speed,
    std::mt19937_64 * rng,
    Symbols symbols,
    Costs costs,
    int max_turn
)
  : id(id), agent_speed(agent_speed), rng(rng),
    symbols(symbols), costs(costs), max_turn(max_turn)
{}

void AI::PrintPercepts(const Percepts & percepts) {
  std::cout << "DISTANCE: " << percepts.detector << std::endl;
  std::cout << "CURRENT:  " << percepts.current[0] << std::endl;
  std::cout << "FORWARD:  ";
  for(std::vector<std::string>::const_iterator it = percepts.forward.begin();
      it != percepts.forward.end(); it++) std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "LEFT:     ";
  for(std::vector<std::string>::const_iterator it = percepts.left.begin();
      it != percepts.left.end(); it++) std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "BACKWARD: ";
  for(std::vector<std::string>::const_iterator it = percepts.backward.begin();
      it != percepts.backward.end(); it++) std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "RIGHT:    ";
  for(std::vector<std::string>::const_iterator it = percepts.right.begin();
      it != percepts.right.end(); it++) std::cout << *it << " ";
  std::cout << std::endl;
  std::cout << "Others:\n";
  for(size_t i = 0; i < percepts.others.size(); i++) {
    std::cout << "   " << i << ": " << percepts.others[i].to_string() << std::endl;
  }
}

std::vector<std::string> AI::Run(
    Percepts & percepts,
    AgentComm * comms
) {
  std::cout << "------------------------------------------------\n";
  std::cout << "AGENT ID: " << id << std::endl;
  PrintPercepts(percepts);
  std::vector<std::string> cmds {"R", "B", "L", "F", "U", "D"};
  std::shuffle(cmds.begin(), cmds.end(), *rng);
  std::cout << "CMD:      " << cmds[0] << std::endl;
  return {cmds[0]};
}



