/***
 *
 * Microworld 2 - Send in the Rogues
 * main.cpp
 * zax
 * adopted from a previous microworld in July 2026
 * 
 * HISTORY
 *
 * Version 1.1
 * - Fixes uninitialized treasures variables.
 * - Adds max_turn member data to AI class.
 *
 * Version 1.0
 * - Initial version
 */

#include<SFML/Graphics.hpp>
#include <algorithm>
#include<cstring>
#include <stdexcept>
#include<string>
#include<random>
#include<chrono>
#include<map>
#include"defs.hpp"
#include"display.hpp"
#include"world.hpp"
#include"agent.hpp"
#include"percepts.hpp"

/* The main sim loop. */
void loop(
	  World & world, 
	  std::vector<Agent> & agents,
	  int update_delay, int max_turns,
	  std::mt19937_64 & rng);

/* Called when the game ends. */
void game_over(
	       World& world,
	       std::string reason,
	       std::vector<Agent> & agents);


inline std::string config_filename = "config.json";
inline unsigned screen_width = 1000;
inline unsigned screen_height = 850;

int main(int argc, char **argv) {

  /**************************************************************
    VARS
  */
  bool RUN_SFML_TEST = false;
  bool DEBUG = false;
  bool RANDOMIZE_STARTS = false;
  bool RANDOMIZE_HEADINGS = false;
  screen_width = 1000;
  screen_height = 850;
  std::random_device rd;
  std::mt19937_64 rng(rd());
  int update_delay = 1000;
  int max_turns = 1000;

  /**************************************************************
    SET UP THE WORLD
  */
  std::string world_filename = "default_world.txt";
  std::string world_config_filename = "default_world_config.json";

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-w") == 0) {
      if(argc > i+1) {
	std::string base = std::string(argv[i+1]);
	world_filename = base+".txt";
	world_config_filename = base+"_config.json";
      } else {
	std::cerr << "Missing filename after -w\n";
	return 1;
      }
    } else if(strcmp(argv[i], "-rs") == 0) {
      RANDOMIZE_STARTS = true;
    } else if(strcmp(argv[i], "-rh") == 0) {
      RANDOMIZE_HEADINGS = true;
    } else if(strcmp(argv[i], "-rsh") == 0 || strcmp(argv[i], "-rhs") == 0) {
      RANDOMIZE_HEADINGS = true;
      RANDOMIZE_STARTS = true;
    }
    else if (strcmp(argv[i], "-s") == 0) {
      if (argc > i + 1) {
	try {
	  unsigned long seed = std::stol(argv[i + 1]);
	  rng.seed(seed);
	}
	catch (std::invalid_argument& e) {
	  std::cerr << "Invalid arg for -s" << argv[i + 1] << std::endl
		    << e.what() << std::endl;
	  return 1;
	}
      }
    }
    else if(strcmp(argv[i], "-d") == 0) {
      if(argc > i+1) {
	try {
	  update_delay = std::stol(argv[i+1]);
	} catch(std::invalid_argument & e) {
	  std::cerr << "Invalid arg for -d" << argv[i+1] << std::endl 
		    << e.what() << std::endl;
	  return 1;
	}
      } else {
	std::cerr << "Missing value for -d" << std::endl;
	return 1;
      }
    } else if(strcmp(argv[i], "-t") == 0) {
      if(argc > i+1) {
	try {
	  max_turns = std::stoi(argv[i+1]);
	} catch(std::invalid_argument & e) {
	  std::cerr << "Invalid value for -t" << argv[i+1] << std::endl
		    << e.what() <<  std::endl;
	  return 1;
	}
      } else {
	std::cerr << "Missing value for -t" << std::endl;
	return 1;
      }
    }
  }

    
    
  World world;
    
  if(!world.LoadConfig(world_config_filename)) {
    return 1;
  }
  if(!world.LoadMap(world_filename)) {
    return 1;
  }

  /**************************************************************
    SET UP THE AGENTS
  */
  if(RANDOMIZE_STARTS) {
    std::shuffle(
		 world.agent_starts.begin(), 
		 world.agent_starts.end(),
		 rng
		 );
  }

  std::vector<Vec2> headings = {
    Vec2(0,-1),
    Vec2(1,0),
    Vec2(0,1),
    Vec2(-1, 0)
  };

  AgentSight agent_sight;
  agent_sight.forward = world.config.agent_sight[0];
  agent_sight.left = world.config.agent_sight[1];
  agent_sight.right = world.config.agent_sight[2];
  agent_sight.behind = world.config.agent_sight[3];

  std::vector<Agent> agents;
  for(unsigned i = 0; i < world.config.num_agents; i++) {

    if(RANDOMIZE_HEADINGS) {
      std::shuffle(
		   headings.begin(), headings.end(), rng
		   );
    }

    Agent a(
            world.agent_starts[i%world.agent_starts.size()],
            headings[0],
            agent_sight,
            i,
            &rng,
            world.config.agent_speed,
            AgentType::AGENT,
            world.GetSymbols(),
	    world.GetCosts(),
	    max_turns
	    );

    agents.push_back(std::move(a));
  }

    

    
    
  /***************************************************************
    SET UP DISPLAY
  */
  if(!dspl::InitDisplay(screen_width, screen_height, world.config)) {
    return 1;
  }
    
  /***************************************************************
    RUN SIMULATION
  */
  loop(world, agents, update_delay, max_turns, rng);


}

void loop(
	  World& world,
	  std::vector<Agent>& agents,
	  int update_delay, int max_turns,
	  std::mt19937_64 & rng) {

  bool finished = false;
  std::string finish_reason = "NONE";
  int turn = 0;

  unsigned remaining_mines = world.GetNumMines();
  unsigned remaining_treasures = world.GetNumTreasures();
  std::vector<int> agent_points;
  for(size_t i = 0; i < agents.size(); i++) {
    agent_points.push_back(0);
  }

  std::uniform_real_distribution<double> slipDist(0,1);

  auto last_update = std::chrono::high_resolution_clock::now();

  // Init comms
  AgentComm comms(agents.size());

  bool paused = false;

  size_t cur_agent_index = 0;

  /******************************************************
        MAIN LOOP
  *******************************************************/
  while(dspl::IsOpen()) {

    // Handle events
    while(const std::optional<sf::Event> event = dspl::PollEvent()) {
      if(event->is<sf::Event::KeyPressed>()) {
	const auto *key = event->getIf<sf::Event::KeyPressed>();
	if (key->code == sf::Keyboard::Key::Escape) dspl::Close();
	else if(key->code == sf::Keyboard::Key::Q) finished = true;
	else if (key->code == sf::Keyboard::Key::P) paused = !paused;
      }
    }

    if (!finished && !paused) {

      // If time to run AI updates.
      
      if (agents[cur_agent_index].GetAgentState() == AgentState::Active) {
	auto cur_time = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(cur_time - last_update).count() > update_delay) {

	  size_t i = cur_agent_index;

	  agents[i].AddPoints(world.config.round_cost);
	  
	  AgentSight sight = agents[i].GetSight();
	  Vec2 location = agents[i].GetLoc();
	  Vec2 forward = agents[i].GetHeading();
	  unsigned speed = agents[i].GetSpeed();
	  Vec2 left = Vec2(forward.y, -forward.x);
	  Vec2 right = Vec2(-forward.y, forward.x);
	  Vec2 back = Vec2(-forward.x, -forward.y);
	  Percepts percepts;
	  std::vector<Vec2> seen_cells;
	  int id = agents[i].GetID();

	  // Process and find all percepts
	  percepts.current = world.GetCells(location, forward, 1, seen_cells);
	  percepts.forward = world.GetCells(location + forward, forward, sight.forward, seen_cells);
	  percepts.backward = world.GetCells(location + back, back, sight.behind, seen_cells);

	  percepts.left = world.GetCells(location + left, left, sight.left, seen_cells);
	  percepts.right = world.GetCells(location + right, right, sight.right, seen_cells);
                    
	  percepts.detector = world.ManhattanDistanceToNearestMine(location);

	  percepts.others.clear();
	  for(size_t a = 0; a < agents.size(); a++) {
	    Vec2 dir = agents[i].GetRelativeDirTo(agents[a].GetLoc());
	    percepts.others.push_back(dir);
	  }

	  // Call the AI
	  std::vector<std::string> cmds = agents[i].RunAI(percepts, &comms);
	  if(cmds.empty()) {
	    agents[i].AddPoints(world.config.inactive_cost);
	  }

	  // Handle AI commands.
	  unsigned s = 0;
	  unsigned max_cmds = agents[i].GetSpeed();
	  while (s < max_cmds && s < cmds.size()) {
	    // Does the agent slip? If so, it fails
	    // to perform this command.
	    if(slipDist(rng) < world.config.slip_chance) {
	      s++; continue;
	    }
	    std::string cmd = cmds[s];
	    location = agents[i].GetLoc();
	    forward = agents[i].GetHeading();
	    left = Vec2(forward.y, -forward.x);
	    right = Vec2(-forward.y, forward.x);
	    back = Vec2(-forward.x, -forward.y);
	    // Move forward
	    if (cmd == "F") {
	      location = location + forward;
	      if (world.CanMoveIntoCell(location)) {
		agents[i].SetLoc(location);
		if(world.HasHitMine(agents[i].GetLoc())) {
		  agents[i].AddPoints(world.config.death_cost);
		  agents[i].SetAgentState(AgentState::Dead);
		  break; // stop processing commands
		} else {
		  agents[i].AddPoints(world.config.move_cost);
		}
	      }
	      else {
		// Runs into wall
		agents[i].AddPoints(world.config.crash_cost);
	      }
	    }

	    // Move backward
	    else if (cmd == "B") {
	      location = location + back;
	      if (world.CanMoveIntoCell(location)) {
		agents[i].SetLoc(location);
		if(world.HasHitMine(agents[i].GetLoc())) {
		  agents[i].AddPoints(world.config.death_cost);
		  agents[i].SetAgentState(AgentState::Dead);
		  break; // stop processing commands
		} else {
		  agents[i].AddPoints(world.config.move_cost);
		}
	      }
	      else {
		// Backs into wall;
		agents[i].AddPoints(world.config.crash_cost);
	      }
	    }
	    // Two turn commands
	    else if (cmd == "L") {
	      agents[i].SetHeading(left);
	      agents[i].AddPoints(world.config.turn_cost);
	    }
	    else if (cmd == "R") {
	      agents[i].SetHeading(right);
	      agents[i].AddPoints(world.config.turn_cost);
	    }
	    // Use command
	    else if (cmd == "U") {
	      Vec2 result_location;
	      Result res = world.UseCell(location, result_location);
	      if (res == Result::TELEPORT) {
		agents[i].SetLoc(result_location);
		agents[i].AddPoints(world.config.tele_cost);
	      } else {
		// Used non-existent teleporter.
		agents[i].AddPoints(world.config.notele_cost);
	      }
	    } else if (cmd == "T") {
	      if(world.TakeTreasure(location)) {
		agents[i].AddPoints(world.config.treasure_cost);
		if(world.GetNumTreasuresRemaining() <= 0) {
		  break; // stop processing commands, game over
		}
	      } else {
		agents[i].AddPoints(world.config.notreasure_cost);
	      }
	    }
	    // Disarm command
	    else if (cmd == "D") {
	      Vec2 disarm_loc = location + forward;
	      if(world.DisarmMine(disarm_loc)) {
		agents[i].AddPoints(world.config.disarm_cost);
	      } else {
		// Disarmed non-existent trap.
		agents[i].AddPoints(world.config.notrap_cost);
	      }
	    }
	    s++;
	  }
	  cur_agent_index = cur_agent_index+1;
	  if(cur_agent_index >= agents.size()) {
	    turn++;
	    cur_agent_index = cur_agent_index%agents.size();
	  }

	  last_update = std::chrono::high_resolution_clock::now();

	} else {
	  // Failed update due to time delay - do nothing.
	}
      } else {
	// Agent is dead - just go to next agent.
	cur_agent_index = cur_agent_index+1;
	if(cur_agent_index >= agents.size()) {
	  turn++;
	  cur_agent_index = cur_agent_index%agents.size();
	}
      }
    } // !finished && !paused

    /**********************************************************
            DRAWING CODE
    ***********************************************************/
    dspl::Clear();

    dspl::DrawMap(world.map,world.config);
    for(std::vector<Agent>::iterator it = agents.begin();
	it != agents.end(); it++) {
        
      if(it->GetAgentState() != AgentState::Active) continue;
      dspl::DrawAgent(it->GetLoc(), it->GetHeading(), "A", sf::Color::Black);
    }
        
    dspl::DrawMsg("Turn: "+std::to_string(turn), 10, 700, 20, sf::Color::Yellow);

    if (paused) {
      dspl::DrawMsg("SIM IS PAUSED", 300, 700, 20, sf::Color::Red);
    }

    // Make several checks to see if the sim has finished.
    // First, are we at max_turns?
    if(!finished && turn >= max_turns) {
      finished = true;
      finish_reason = "MAX TURNS REACHED";
    }

    if(world.GetNumTreasuresRemaining() <= 0) {
      finished = true;
      finish_reason = "ALL TREASURES HAVE BEEN COLLECTED";
    }

    // Second, are any agents left in the environment?
    if (!finished) {
      bool one_alive = false;
      for (size_t i = 0; i < agents.size(); i++) {
	if (agents[i].GetAgentState() == AgentState::Active) {
	  one_alive = true;
	  break;
	}
      }
      if (!one_alive) {
	finished = true;
	finish_reason = "ALL AGENTS WERE DESTROYED";
      }
    }

    // If the sim is over...
    if (finished) {
      game_over(world, finish_reason, agents);
    }

    dspl::Display();
  }
}

void game_over(
	       World& world,
	       std::string reason,
	       std::vector<Agent>& agents) {

  dspl::DrawMsg(reason, 200,700,20);

  float x = 100.0f;
  float y = 730.f;
  int total = 0;
  std::string msg;
  for(size_t i = 0; i < agents.size(); i++) {
    int points = agents[i].GetPoints();
    msg = ("Agent "+std::to_string(i)+": "+std::to_string(points));
    dspl::DrawMsg(msg, x, y, 15, sf::Color::White);
    total += points;
    y+=20.f;
    if(y + 20.0 > screen_height) {
      x += 150.0f;
      y = 730.f;
    }
  }
  msg = "Total: " + std::to_string(total);
  dspl::DrawMsg(msg, x, y, 15, sf::Color::White);

  x = 400.0f;
  y = 730.f;
  total = world.GetNumMinesRemaining();
  msg = "Mines Left: " + std::to_string(total);
  dspl::DrawMsg(msg, x, y, 15, sf::Color::White);

  y+=20.f;
  total = world.GetNumTreasuresRemaining();
  msg = "Treasures Left: " + std::to_string(total);
  dspl::DrawMsg(msg, x, y, 15, sf::Color::White);
}
