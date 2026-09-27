#include"world.hpp"
#include "defs.hpp"


World::World() 
  : num_mines(0), num_mines_remaining(0),
    num_treasures(0), num_treasures_remaining(0)
{}



bool World::LoadConfig(std::string & filename) {
  std::ifstream ifs("worlds/"+filename);
  nlohmann::json j;
  ifs >> j;

  try {
    config.num_agents = j.at("num_agents");
    config.agent_speed = j.at("agent_speed");
    config.agent_sight = j.at("agent_sight");
    config.wall = j.at("wall");
    config.open = j.at("open");
    config.mine = j.at("mine");
    config.disarmed_mine = j.at("disarmed_mine");
    config.exploded_mine = j.at("exploded_mine");
    config.tele_symbols = j["tele_symbols"].get<std::vector<std::string>>();
    config.tele_dests = j["tele_dests"].get<std::vector<unsigned>>();
    config.agent_start = j.at("agent_start");
    config.treasure = j.at("treasure");
    config.wall_color = j["wall_color"].get<std::vector<unsigned>>();
    config.open_color = j["open_color"].get<std::vector<unsigned>>();
    config.round_cost = j.at("round_cost");
    config.move_cost = j.at("move_cost");
    config.turn_cost = j.at("turn_cost");
    config.tele_cost = j.at("tele_cost");
    config.crash_cost = j.at("crash_cost");
    config.notele_cost = j.at("notele_cost");
    config.notrap_cost = j.at("notrap_cost");
    config.death_cost = j.at("death_cost");
    config.disarm_cost = j.at("disarm_cost");
    config.inactive_cost = j.at("inactive_cost");
    config.treasure_cost = j.at("treasure_cost");
    config.notreasure_cost = j.at("notreasure_cost");
    config.slip_chance = j.at("slip_chance");

    symbols.teleporters = config.tele_symbols;
    symbols.wall = config.wall;
    symbols.open = config.open;
    symbols.disarmed_mine = config.disarmed_mine;
    symbols.exploded_mine = config.exploded_mine;
    symbols.treasure = config.treasure;

    costs.round_cost = config.round_cost;
    costs.move_cost = config.move_cost;
    costs.turn_cost = config.turn_cost;
    costs.tele_cost = config.tele_cost;
    costs.crash_cost = config.crash_cost;
    costs.notele_cost = config.notele_cost;
    costs.notrap_cost = config.notrap_cost;
    costs.death_cost = config.death_cost;
    costs.disarm_cost = config.disarm_cost;
    costs.inactive_cost = config.inactive_cost;
    costs.treasure_cost = config.treasure_cost;
    costs.notreasure_cost = config.notreasure_cost;

    for (size_t t = 0; t < config.tele_symbols.size(); t++) {
      Teleporter tele;
      tele.id = teleporters.size();
      tele.location = Vec2(-1, -1);
      tele.symbol = config.tele_symbols[t];
      tele.dest = config.tele_dests[t];
      teleporters.push_back(tele);
    }

    
  } catch(std::exception & e) {
    std::cerr << "Error parsing config.json file" << e.what() << std::endl;
    return false;
  }

  return true;

}

bool World::LoadMap(std::string & filename) {

  std::ifstream ifs("worlds/"+filename);
  if(!ifs.good()) {
    std::cerr << "Failed to open world file: " << filename << std::endl;
    return false;
  }

  std::string line;
  unsigned y = 0;
  while(std::getline(ifs, line)) {
    std::vector<std::string> row;
    for(size_t i = 0, x=0; i < line.size(); i++) {
      if(line[i] == ' ' || line[i] == '\n' || line[i] == '\r') continue;
      else {
	std::string s = line.substr(i, 1);
	row.push_back(s);

	if(s == config.agent_start) {
	  agent_starts.push_back(Vec2(x, y));

	} else if (s == config.mine) {
	  num_mines++;
	  mine_locations.push_back(Vec2(x,y));
	} else if (s == config.treasure) {
	  num_treasures++;
	  treasure_locations.push_back(Vec2(x,y));
	}
	else if (std::find(config.tele_symbols.begin(), config.tele_symbols.end(), s) !=
		 config.tele_symbols.end()) {
	  // Fill in teleporter map x,y
	  for (size_t t = 0; t < teleporters.size(); t++) {
	    if (s == teleporters[t].symbol) {
	      teleporters[t].location = Vec2(x, y);
	    }
	  }
	}

	x++;
      }
    }
    map.push_back(row);
    y++;
  }

  num_mines_remaining = num_mines;
  num_treasures_remaining = num_treasures;

  return true;
}

std::string World::GetCell(Vec2 location) {
  return map[location.y][location.x];
}

void World::SetCell(Vec2 location, std::string s) {
  map[location.y][location.x] = s;
}

std::vector<std::string> World::GetCells(
					 Vec2 loc, Vec2 heading, int dist,
					 std::vector<Vec2>& seen_cells
					 ) {
  std::vector<std::string> v;
  for(int i = 0; i < dist; i++) {
    Vec2 offset = heading * i;
    Vec2 newloc = loc + offset;
    std::string cell = map[newloc.y][newloc.x];

    // Agents can't see starting locations and mines
    // that have not been disarmed.
    if (cell == config.mine || cell == config.agent_start) {
      cell = config.open;
    }

    v.push_back(cell);
    if(cell == config.wall) {
      break;
    }
    seen_cells.push_back(newloc);
  }
  return v;
}

bool World::CanMoveIntoCell(Vec2 location) {
  if(map[location.y][location.x] == config.wall) {
    return false;
  }
  return true;
}
Result World::UseCell(Vec2 location, Vec2 & result_location) {
  std::string cell = map[location.y][location.x];

  for (size_t i = 0; i < teleporters.size(); i++) {
    if (teleporters[i].symbol == cell) {
      result_location = teleporters[teleporters[i].dest].location;
      return Result::TELEPORT;
    }
  }
    

  return Result::NONE;
}

unsigned World::GetNumMines() const { return num_mines; }

unsigned World::GetNumMinesRemaining() const {
  return num_mines_remaining;
}
void World::ChangeNumMinesRemaining(unsigned amt) {
  num_mines_remaining += amt;
}

unsigned World::GetNumTreasures() const {
  return num_treasures;
}
unsigned World::GetNumTreasuresRemaining() const {
  return num_treasures_remaining;
}
void World::ChangeNumTreasuresRemaining(unsigned amt) {
  num_treasures_remaining += amt;
}

bool World::HasHitMine(const Vec2 & loc) {
  for(std::vector<Vec2>::iterator it = mine_locations.begin();
      it != mine_locations.end(); it++) {
    if(*it==loc) {
      ChangeNumMinesRemaining(-1);
      SetCell(loc, config.exploded_mine);
      mine_locations.erase(it);
      return true;
    }
  }
  return false;
}
bool World::DisarmMine(const Vec2 & loc) {
  bool res = HasHitMine(loc);
  if(res) {
    SetCell(loc, config.disarmed_mine);
  }
  return res;
}

int World::ManhattanDistanceToNearestMine(const Vec2 & loc) {
  int d = std::numeric_limits<int>::max();
  for(std::vector<Vec2>::const_iterator it = mine_locations.begin();
      it != mine_locations.end(); it++) {
    int md = it->ManhattanDistance(loc);
    if(md < d) d = md;
  }
  if(d == std::numeric_limits<int>::max()) {
    d = -1;
  }
  return d;
}

bool World::TakeTreasure(const Vec2 & loc) {
  for(std::vector<Vec2>::iterator it = treasure_locations.begin();
      it != treasure_locations.end(); it++) {
    if(*it==loc) {
      ChangeNumTreasuresRemaining(-1);
      SetCell(loc, config.open);
      treasure_locations.erase(it);
      return true;
    }
  }
  return false;
}

Symbols World::GetSymbols() const {
  return symbols;
}

Costs World::GetCosts() const {
  return costs;
}
