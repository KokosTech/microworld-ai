#pragma once

#include<memory>
#include<vector>
#include"vec2.hpp"
#include"ai.hpp"
#include"defs.hpp"
#include"percepts.hpp"

enum class AgentState {
  Active,
  Dead
};


class Agent {
private:
  Vec2 loc;
  Vec2 heading;
  AgentSight sight;
  unsigned speed;
  unsigned id;
  std::unique_ptr<AI> ai;
  AgentState state;
  AgentType type;
  int points;
public:
  Agent();
  Agent(
        Vec2 loc, Vec2 heading, AgentSight sight, unsigned id, std::mt19937_64 * rng,
        unsigned speed, AgentType type, Symbols symbols, Costs costs, int max_turn);
  unsigned GetID() const;
  Vec2 GetLoc() const;
  Vec2 GetHeading() const;
  AgentState GetAgentState() const;
  unsigned GetSpeed() const;
  AgentType GetType() const;
  int GetPoints() const;
  void SetPoints(int p);
  void SetSpeed(unsigned s);
  void SetAgentState(AgentState as);
  void SetLoc(Vec2 v);
  void SetHeading(Vec2 v);
  void AddPoints(int p);
  AgentSight GetSight();
  Vec2 GetRelativeDirTo(Vec2 v);
  std::vector<std::string> RunAI(
				 Percepts & percepts,
				 AgentComm * comms
				 );
};
