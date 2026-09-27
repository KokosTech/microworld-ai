#include"agent.hpp"

Agent::Agent() {}
Agent::Agent(
	     Vec2 loc, Vec2 heading, AgentSight sight, unsigned id, std::mt19937_64* rng,
	     unsigned speed, AgentType type, Symbols symbols, Costs costs, int max_turn) 
  : loc(loc), heading(heading), sight(sight), id(id), state(AgentState::Active),
    speed(speed), type(type), points(0)
{
  // if (type == AgentType::FOX) {
  //     ai = std::make_unique<FoxAI>(id, speed, rng, goal, exit, teleporters);
  // }
  // else if (type == AgentType::HOUND) {
  //     ai = std::make_unique<HoundAI>(id, speed, rng);
  // }
  ai = std::make_unique<AI>(id, speed, rng, symbols, costs, max_turn);
}
unsigned Agent::GetID() const { return id; }
Vec2 Agent::GetLoc() const { return loc; }
Vec2 Agent::GetHeading() const { return heading; }
AgentState Agent::GetAgentState() const { return state; }
unsigned Agent::GetSpeed() const { return speed; }
AgentType Agent::GetType() const { return type;  }
int Agent::GetPoints() const { return points; }
void Agent::SetPoints(int p) { points = p; }
void Agent::SetSpeed(unsigned s) { speed = s; }
void Agent::SetAgentState(AgentState as) { state = as; }
void Agent::SetLoc(Vec2 v) {
  loc = v;
}
void Agent::SetHeading(Vec2 v) {
  heading = v;
}
void Agent::AddPoints(int p) { points += p; }
AgentSight Agent::GetSight() { return sight; }
Vec2 Agent::GetRelativeDirTo(Vec2 v) {
  Vec2 diff = v - loc;
  diff.y = -diff.y;
  if(heading==SOUTH) {
    diff.x = -diff.x;
    diff.y = -diff.y;
  } else if(heading==EAST) {
    int temp = diff.x;
    diff.x = -diff.y;
    diff.y = temp;
  } else if(heading==WEST) {
    int temp = -diff.x;
    diff.x = diff.y;
    diff.y = temp;
  }
  return diff;
}

std::vector<std::string> Agent::RunAI(
				      Percepts & percepts,
				      AgentComm * comms
				      ) {
  if (state == AgentState::Active) {
    return ai->Run(percepts, comms);
  }
  else {
    return std::vector<std::string>();
  }
}
