// formatting used - google clang in clion

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
                std::vector<std::string>(DEFAULT_MAP_SIZE, UNEXPLORED_SYMBOL)),
      agent_safe(DEFAULT_MAP_SIZE, std::vector<int>(DEFAULT_MAP_SIZE, 0)),
      no_traps_left(false),
      pending_teleport(false),
      probing(false),
      probe_cell({0, 0}),
      has_target(false),
      explore_target({0, 0}) {}

// =================================================
// === MAPPING METHODS
// =================================================

void AI::PrintPercepts(const Percepts& percepts) {
  std::cout
      << "\033[2J\033[1;1H";  // for smoother transition of the map printed
  std::cout << "------------------ MAP ----------------------\n";
  PrintMap(agent_map);
  std::cout << "---------------------------------------------\n";
  std::cout << "------------------ SAFE MAP ----------------------\n";
  PrintMap(agent_safe);
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

template <typename T>
void AI::PrintMap(const std::vector<std::vector<T>>& map) {
  const char* arrows[4] = {"^", ">", "v", "<"};

  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      const T& cell = map[y][x];
      // check if type is str - didnt even know it was possible
      // https://stackoverflow.com/questions/13636540/how-to-check-for-the-type-of-a-template-parameter
      // https://en.cppreference.com/cpp/types/is_same
      if constexpr (std::is_same_v<T, std::string>) {
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
      } else {
        std::cout << cell << " ";
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
  for (size_t i = 0; i < cmds.size(); ++i) std::cout << cmds[i] << " ";
  std::cout << std::endl;

  ++this->clock;
  for (size_t i = 0; i < cmds.size(); ++i) {
    const std::string& cmd = cmds[i];
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

      if (!IsInBounds(next_x, next_y)) {
        continue;
      }

      // Check whether the destination is known to be a wall.

      if (agent_map[next_y][next_x] == symbols.wall) {
        continue;
      }

      agent_position.x = next_x;
      agent_position.y = next_y;
    } else if (cmd == USE) {
      // We only know where we land when the next percepts arrive.
      if (!percepts.current.empty() && IsTeleporter(percepts.current[0])) {
        pending_teleport = true;
        pending_from = percepts.current[0];
      }
    }
  }
  return cmds;
}

// =================================================
// === TRAVERSING HELPERS + TRAP HANDLING
// =================================================

bool AI::IsTeleporter(const std::string& s) {
  for (size_t i = 0; i < symbols.teleporters.size(); ++i) {
    if (symbols.teleporters[i] == s) return true;
  }
  return false;
}

bool AI::IsSafe(int x, int y) {
  const std::string& cell = agent_map[y][x];

  if (cell == UNEXPLORED_SYMBOL || cell == UNREACHABLE_SYMBOL ||
      cell == symbols.wall)
    return false;

  if (cell == SAFE_SYMBOL || cell == symbols.treasure ||
      cell == symbols.disarmed_mine || cell == symbols.exploded_mine ||
      IsTeleporter(cell)) {
    return true;
  }

  return no_traps_left || agent_safe[y][x] == 1;
}

// the frontier is basically the safe cells that have unknown relatives
bool AI::IsFrontier(int x, int y) {
  if (!IsSafe(x, y)) return false;

  for (int d = 0; d < 4; ++d) {
    int nx = x + DX[d];
    int ny = y + DY[d];
    if (!IsInBounds(nx, ny)) continue;
    const std::string& cell = agent_map[ny][nx];
    if (cell == symbols.wall || cell == UNREACHABLE_SYMBOL) continue;
    if (cell == UNEXPLORED_SYMBOL || !IsSafe(nx, ny)) return true;
  }

  return false;
}

// auto safe if we're on it
// if d < 0 -> no need to waste power
// anything with lower manhatan distance than d is safe
void AI::MarkSafe(const Percepts& percepts) {
  agent_safe[agent_position.y][agent_position.x] = 1;  // we are alive here

  if (percepts.detector < 0) {
    no_traps_left = true;
    return;
  }

  int r = percepts.detector - 1;

  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      if (std::abs(dx) + std::abs(dy) > r) continue;
      int x = agent_position.x + dx;
      int y = agent_position.y + dy;
      if (IsInBounds(x, y)) agent_safe[y][x] = 1;
    }
  }
}

// only when d is 1 - try to disarm only non-safe cells (save on costs)
// my initial logic was to do all 4 bcz it doesnt cost that much
// then i realized that we do not have a lot of turns in the game
bool AI::ProbeNeighbors(std::string& cmd) {
  int best_dir = -1;
  int best_turns = 10;

  for (int d = 0; d < 4; d++) {
    int x = agent_position.x + DX[d];
    int y = agent_position.y + DY[d];

    if (!IsInBounds(x, y)) continue;

    const std::string& cell = agent_map[y][x];

    if (cell == symbols.wall || cell == UNEXPLORED_SYMBOL ||
        cell == UNREACHABLE_SYMBOL)
      continue;
    if (IsSafe(x, y)) continue;

    int turn = (d - rotation + 4) % 4;
    int turns = turn;

    if (turn == 3) turns = 1;
    if (turns < best_turns) {
      best_turns = turns;
      best_dir = d;
    }
  }
  if (best_dir == -1) return false;

  int turn = (best_dir - rotation + 4) % 4;
  if (turn == 0) {
    probing = true;
    probe_cell.x = agent_position.x + DX[best_dir];
    probe_cell.y = agent_position.y + DY[best_dir];
    cmd = DISARM;
  } else if (turn == 3) {
    cmd = LEFT;
  } else {
    cmd = RIGHT;
  }
  return true;
}

// look around UNKNOWN cells next to the agent
bool AI::LookBehind(std::string& cmd) {
  for (int turn = 1; turn < 4; turn++) {
    int d = (rotation + turn) % 4;
    int x = agent_position.x + DX[d];
    int y = agent_position.y + DY[d];

    if (!IsInBounds(x, y) || agent_map[y][x] != UNEXPLORED_SYMBOL) continue;

    if (turn == 3) {
      cmd = LEFT;
    } else {
      cmd = RIGHT;
    }

    return true;
  }
  return false;
}

// =================================================
// === PATHFINDING SHI
// =================================================

// basically get a map of "weights" and directions from cur pos
// some online forums call it a flooding algorithm
// ONLY SAFE CELLS
void AI::Bfs() {
  bfs_dist.assign(DEFAULT_MAP_SIZE, std::vector<int>(DEFAULT_MAP_SIZE, -1));
  bfs_dir.assign(DEFAULT_MAP_SIZE, std::vector<int>(DEFAULT_MAP_SIZE, -1));

  std::queue<Coordinates> q;
  q.push(agent_position);
  bfs_dist[agent_position.y][agent_position.x] = 0;

  while (!q.empty()) {
    Coordinates cur = q.front();
    q.pop();

    for (int d = 0; d < 4; ++d) {
      int nx = cur.x + DX[d];
      int ny = cur.y + DY[d];

      if (!IsInBounds(nx, ny) || bfs_dist[ny][nx] != -1 || !IsSafe(nx, ny))
        continue;

      bfs_dist[ny][nx] = bfs_dist[cur.y][cur.x] + 1;
      bfs_dir[ny][nx] = d;
      Coordinates next = {nx, ny};

      q.push(next);
    }
  }
}

// universal target
std::string AI::FirstStepTo(Coordinates target) {
  int x = target.x;
  int y = target.y;
  int d = bfs_dir[y][x];

  // basically backtracking
  while (bfs_dist[y][x] > 1) {
    x -= DX[d];
    y -= DY[d];
    d = bfs_dir[y][x];
  }

  int turn = (d - rotation + 4) % 4;

  if (turn == 0) return FORWARD;
  if (turn == 1) return RIGHT;
  if (turn == 3) return LEFT;

  return BACKWARD;
}

// eat the closest treasurr
bool AI::FindNearestTreasure(Coordinates& target) {
  int best = -1;

  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      if (agent_map[y][x] != symbols.treasure || bfs_dist[y][x] <= 0) continue;
      if (best == -1 || bfs_dist[y][x] < best) {
        best = bfs_dist[y][x];
        target.x = x;
        target.y = y;
      }
    }
  }

  return best != -1;
}

// nearest frontier (i think i read about it in the book)
// it also said tiebreaking is important
// in this case -> rng is used
bool AI::FindNearestFrontier(Coordinates& target) {
  int best = -1;
  int ties = 0;

  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      if (bfs_dist[y][x] <= 0 || !IsFrontier(x, y)) continue;
      if (best == -1 || bfs_dist[y][x] < best) {
        best = bfs_dist[y][x];
        ties = 1;
        target.x = x;
        target.y = y;
      } else if (bfs_dist[y][x] == best) {
        ++ties;
        if ((*rng)() % ties == 0) {
          target.x = x;
          target.y = y;
        }
      }
    }
  }

  return best != -1;
}

// not always the closest - prefers unused (or least used)
// it's kinda like the weighed algos in class but on much simpler scale
// you could say the agent is goal based in this sense
bool AI::FindNearestTeleporter(Coordinates& target) {
  int best = -1;

  for (int y = 0; y < DEFAULT_MAP_SIZE; ++y) {
    for (int x = 0; x < DEFAULT_MAP_SIZE; ++x) {
      const std::string& cell = agent_map[y][x];

      if (!IsTeleporter(cell) || bfs_dist[y][x] < 0) continue;
      if (TimesUsed(cell) >= MAX_TELEPORTER_USES) continue;

      int score = bfs_dist[y][x] + 100 * TimesUsed(cell);

      if (best == -1 || score < best) {
        best = score;
        target.x = x;
        target.y = y;
      }
    }
  }

  return best != -1;
}

// =================================================
// === TELEPORT HANDL
// =================================================

// as i dont have enough time, this is a middle ground solution
// just having a new map - reset everything !!!!
void AI::ResetMap() {
  agent_map.assign(DEFAULT_MAP_SIZE, std::vector<std::string>(
                                         DEFAULT_MAP_SIZE, UNEXPLORED_SYMBOL));
  agent_safe.assign(DEFAULT_MAP_SIZE, std::vector<int>(DEFAULT_MAP_SIZE, 0));

  agent_position.x = DEFAULT_MAP_SIZE / 2;
  agent_position.y = DEFAULT_MAP_SIZE / 2;

  has_target = false;
  probing = false;

  for (size_t i = 0; i < peers.size(); ++i) peers[i].has_prev = false;
}

int AI::TimesUsed(const std::string& teleporter) {
  int count = 0;

  for (size_t i = 0; i < used_teleporters.size(); ++i) {
    if (used_teleporters[i] == teleporter) ++count;
  }

  return count;
}

// track used teleporters and reset map
// in the future (winter break) - i would track from / to etc
// just didnt have the time for it
void AI::PostTeleport(const Percepts& percepts) {
  pending_teleport = false;
  used_teleporters.push_back(pending_from);

  // the teleporter leads to itself DO NOT RESET MAP !!!
  if (percepts.current[0] == pending_from) return;
  ResetMap();
}

// =================================================
// === COMMS - personally i hope no one has thought of this bcz it literally was
// my brightest moment from the project in the panitza library
// =================================================

Coordinates Rotate(Coordinates v, int k) {
  for (int i = 0; i < k; i++) {
    int old_x = v.x;
    v.x = -v.y;
    v.y = old_x;
  }
  return v;
}

// nibble -
// https://www.geeksforgeeks.org/computer-organization-architecture/what-is-a-nibble/
unsigned GetNibble(uint32_t word, int n) { return (word >> (4 * n)) & 15; }

unsigned AI::Encode(const std::string& s) {
  if (s == symbols.wall) return CODE_WALL;
  if (s == symbols.open) return CODE_OPEN;
  if (s == symbols.treasure) return CODE_TREASURE;
  if (s == symbols.disarmed_mine) return CODE_DISARMED;
  if (s == symbols.exploded_mine) return CODE_EXPLODED;
  for (size_t i = 0; i < symbols.teleporters.size() && i < 6; i++) {
    if (s == symbols.teleporters[i]) return CODE_TELEPORTER + (unsigned)i;
  }
  return CODE_UNKNOWN;
}

std::string AI::Decode(unsigned n) {
  if (n == CODE_WALL) return symbols.wall;
  if (n == CODE_OPEN) return symbols.open;
  if (n == CODE_TREASURE) return symbols.treasure;
  if (n == CODE_DISARMED) return symbols.disarmed_mine;
  if (n == CODE_EXPLODED) return symbols.exploded_mine;
  if (n >= CODE_TELEPORTER &&
      n - CODE_TELEPORTER < symbols.teleporters.size()) {
    return symbols.teleporters[n - CODE_TELEPORTER];
  }

  return UNEXPLORED_SYMBOL;
}

unsigned AI::Nib(const std::vector<std::string>& cells, size_t i) {
  if (i < cells.size()) return Encode(cells[i]);
  return CODE_UNKNOWN;
}

// https://www.geeksforgeeks.org/cpp/left-shift-right-shift-operators-c-cpp/
uint32_t AI::PackCells(const std::vector<std::string>& cells, int pos,
                       int count) {
  uint32_t word = 0;
  for (int i = 0; i < count; ++i) word |= Nib(cells, i) << (4 * (pos - i));
  return word;
}

void AI::UnpackCells(Coordinates start, int dir, uint32_t word,
                     int first_nibble, int count) {
  for (int i = 0; i < count; i++) {
    unsigned code = GetNibble(word, first_nibble - i);
    int x = start.x + DX[dir] * (i + 1);
    int y = start.y + DY[dir] * (i + 1);

    if (code == CODE_UNKNOWN || !IsInBounds(x, y)) continue;

    std::string& cell = agent_map[y][x];
    // only fill cells we do not know, or remove a treasure
    if (cell == UNEXPLORED_SYMBOL ||
        (cell == symbols.treasure && code == CODE_OPEN)) {
      cell = Decode(code);
    }
  }
}

// where another agent is on our map
Coordinates AI::CalcOtherPosition(const Percepts& percepts, size_t j) {
  int forward = rotation;
  int right = (rotation + 1) % 4;
  int vx = percepts.others[j].x;
  int vy = percepts.others[j].y;

  Coordinates pos;

  pos.x = agent_position.x + vx * DX[right] + vy * DX[forward];
  pos.y = agent_position.y + vx * DY[right] + vy * DY[forward];

  return pos;
}

// share map with the rest
void AI::Publish(const Percepts& percepts, AgentComm* comms, int old_rotation,
                 const std::vector<std::string>& cmds) {
  unsigned move = 0;

  if (!cmds.empty()) {
    if (cmds[0] == FORWARD) move = 1;
    if (cmds[0] == BACKWARD) move = 2;
    if (cmds[0] == USE && pending_teleport) move = 3;
  }

  // [DR] F F F L L R R - described in the report
  uint32_t word = ((move << 2) | (unsigned)old_rotation) << 28;

  word |= PackCells(percepts.forward, 6, 3);
  word |= PackCells(percepts.left, 3, 2);
  word |= PackCells(percepts.right, 1, 2);

  comms->SetSignal(id, word);
}

// now this is the hard part - reading
void AI::ReadPeers(const Percepts& percepts, AgentComm* comms) {
  if (peers.size() != percepts.others.size())
    peers.resize(percepts.others.size());

  for (size_t j = 0; j < percepts.others.size(); j++) {
    if (j == id) continue;

    Coordinates now = CalcOtherPosition(percepts, j);

    // a rogue stands there, so no trap
    // you could also say that the rogue is dead, but i dont think my logic
    // allows death -> i assume it is not dead
    if (IsInBounds(now.x, now.y)) agent_safe[now.y][now.x] = 1;

    // if (j >= comms->GetNumAgents()) continue;

    uint32_t word = comms->GetSignal((int)j);
    int their_rotation = GetNibble(word, 7) & 3;
    unsigned their_move = GetNibble(word, 7) >> 2;
    LearnRotation(peers[j], their_rotation, their_move, now);

    peers[j].prev = now;
    peers[j].has_prev = true;

    // todo make #define for the moves it's kinda confusing with just 3 etc
    // we can only use their cells once we know how their map is turned
    if (peers[j].k == -1 || their_move == 3) continue;

    // here we consume the info from them

    // convert rotation from last turn to now
    // save new location and use rotation
    int heading = (their_rotation + peers[j].k) % 4;
    Coordinates start = now;
    if (their_move == 1) {
      start.x -= DX[heading];
      start.y -= DY[heading];
    } else if (their_move == 2) {
      start.x += DX[heading];
      start.y += DY[heading];
    }

    // finally learn the cells from the others
    UnpackCells(start, heading, word, 6, 3);            // forward
    UnpackCells(start, (heading + 3) % 4, word, 3, 2);  // left
    UnpackCells(start, (heading + 1) % 4, word, 1, 2);  // right
  }
}

// and now the hardest - i call it the dance between the rogues
// syncing their rotations - lots of trials and errors
// and edgecases i havent thought of
// firstly i thougth of rotation - turns out it wasnt possible
// but moving backwards and forwards - it was possible to sync the rotations
// i was scared of breaking due to walls - BUT I precoded it not to run into
void AI::LearnRotation(PeerInfo& peer, int their_rotation, unsigned their_move,
                       Coordinates now) {
  if (!peer.has_prev) return;
  if (their_move != 1 && their_move != 2) return;

  int their_dir = their_rotation;
  if (their_move == 2) their_dir = (their_rotation + 2) % 4;

  Coordinates their_step = {DX[their_dir], DY[their_dir]};
  Coordinates seen_step = {now.x - peer.prev.x, now.y - peer.prev.y};

  for (int k = 0; k < 4; k++) {
    Coordinates turned = Rotate(their_step, k);
    // set k to known rotation
    if (turned.x == seen_step.x && turned.y == seen_step.y) peer.k = k;
  }
}

// =================================================
// === JUST AI RUN AND DECISION THINGY
// =================================================

// basically this is the main framework of our next command
// i have based it on my own logic (haven't researched anything, just felt like
// it was more of a common sense - after all we have to make it "human-like')
std::vector<std::string> AI::Decide(Percepts& percepts) {
  // upgraded logic of the trap disarmament
  // tried to, there was no trap - so now we know that cell is safe.
  if (probing) {
    probing = false;
    if (agent_map[probe_cell.y][probe_cell.x] != symbols.disarmed_mine) {
      agent_safe[probe_cell.y][probe_cell.x] = 1;
    }
  }

  // 1. standing on treasure: take it
  if (!percepts.current.empty() && percepts.current[0] == symbols.treasure) {
    return ExecuteCommands({TREASURE}, percepts);
  }

  // after here pathfinding is utilized

  Bfs();  // the best helper init function ever - the filler
  Coordinates target;
  std::string cmd;

  // 2. a treasure we can reach over safe cells
  if (FindNearestTreasure(target))
    return ExecuteCommands({FirstStepTo(target)}, percepts);

  // 3. a trap right next to us: look around, then disarm the suspects
  if (percepts.detector == 1) {
    if (LookBehind(cmd)) return ExecuteCommands({cmd}, percepts);
    if (ProbeNeighbors(cmd)) return ExecuteCommands({cmd}, percepts);
  }

  // 4. look at unknown cells next to us
  if (LookBehind(cmd)) return ExecuteCommands({cmd}, percepts);

  // 5. explore: keep walking to the same frontier while it is still a frontier
  if (has_target && bfs_dist[explore_target.y][explore_target.x] > 0 &&
      IsFrontier(explore_target.x, explore_target.y)) {
    return ExecuteCommands({FirstStepTo(explore_target)}, percepts);
  }
  has_target = false;
  if (FindNearestFrontier(target)) {
    has_target = true;
    explore_target = target;
    return ExecuteCommands({FirstStepTo(target)}, percepts);
  }

  // 6. nothing left in this section: use a teleporter - no more nearby traps,
  // no more frontier; it's escaping time!
  if (FindNearestTeleporter(target)) {
    if (target.x == agent_position.x && target.y == agent_position.y) {
      return ExecuteCommands({USE}, percepts);
    }
    return ExecuteCommands({FirstStepTo(target)}, percepts);
  }

  // i dont know what to do here, dont even know if it's possible to reach it
  // ofc if a $ is in an unreachable area, but z said - that aint happening
  return {};
}

std::vector<std::string> AI::Run(Percepts& percepts, AgentComm* comms) {
  // handle teleporting (aka new map)
  if (pending_teleport) PostTeleport(percepts);

  // add percepts to map
  AddToMap(agent_position, percepts);

  // handle trap logic
  MarkSafe(percepts);

  // merge teleport frames - to be done in the future (for now the map is reset)
  // todo

  // read what the other rogues sent
  ReadPeers(percepts, comms);
  PrintPercepts(percepts);

  // decision
  int old_rotation = rotation;  // the heading our percepts were taken from
  std::vector<std::string> cmds = Decide(percepts);

  // publish comm (send to other rogues)
  Publish(percepts, comms, old_rotation, cmds);
  return cmds;
}