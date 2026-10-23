/* ============================
       Kaloyan Doychinov
        / 200 275 606 /
  COS 4070 A - AI Programming

         Known bugs:
- it was segfaulting multiple times but i think i stabilized it
- slip chance won't work
- agent speed > 1 would break it
- teleporting is suboptimal (resets map)
- doesnt dynamically use costs - it's based on assumptions that may not be true
============================ */
// formatting used - google clang in clion
// comments are for purely report-reason - aka theyll help me what to write on
// the report, sometimes they may not really be connected to the code next to

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "comm.hpp"
#include "percepts.hpp"

// you may be wondering why 160 - i think this is the biggest a map can fit for
// the edgecase of being on the edge
// you always start in the center (in memory)
// but you could be at the edge -> DMS / 2 = Biggest possible size
// i think the biggest size possible is around 60 -> 120
// but just to be sure I put 160
const int DEFAULT_MAP_SIZE = 160;

const std::string UNEXPLORED_SYMBOL = "?";
const std::string SAFE_SYMBOL = "s";

// we looked, still do not know what it is
// tbh this is a very controversial imo symbol bcz it was from one of my ideas
// that was scrapped, i kinda forgot its purpose
const std::string UNREACHABLE_SYMBOL = "x";

const std::string FORWARD = "F";
const std::string BACKWARD = "B";
const std::string LEFT = "L";
const std::string RIGHT = "R";
const std::string USE = "U";
const std::string TREASURE = "T";
const std::string DISARM = "D";

// stop using a teleporter after this times
// the idea is that we do not abuse teleporter travel
// we are supposed to explore the area first AND THEN use a teleporter
// it is basically a conservative approach to teleporter
const int MAX_TELEPORTER_USES = 3;

// =================================================
// === GLOBAL CONST FOR COMM - this is my baby
// =================================================

// [DR] F F F L L R R - described in the report
// codes for one cell in the 32-bit message (4 bits each)
const unsigned CODE_UNKNOWN = 0;
const unsigned CODE_WALL = 1;
const unsigned CODE_OPEN = 2;
const unsigned CODE_TREASURE = 3;
const unsigned CODE_DISARMED = 4;
const unsigned CODE_EXPLODED = 5;

// this is the initial one, teleporters
// range 6..11 = the six teleporter symbols
// rest of the symbols are unused 12..15
const unsigned CODE_TELEPORTER = 6;

// direction 0 = north, 1 = east, 2 = south, 3 = west (turning right adds 1)
const int DX[4] = {0, 1, 0, -1};
const int DY[4] = {-1, 0, 1, 0};

struct Coordinates {
  int x;
  int y;
};

// struct to bundle info for other rogues - keepin track
struct PeerInfo {
  int k = -1;  // rotation offset (can be 0 or positive) -1 is null
  bool has_prev = false;
  Coordinates prev = {0, 0};
};

class AI {
 protected:
  // Necessary, do not delete.
  unsigned id;
  unsigned agent_speed;
  std::mt19937_64* rng;
  Symbols symbols;
  // ExtendedSymbols extended_symbols;
  Costs costs;
  int max_turn;

  // rogue info
  int clock;
  int rotation;
  Coordinates agent_position;  // put in the middle of the map in the beginning
  std::vector<std::vector<std::string>> agent_map;
  std::vector<std::vector<int>> agent_safe;  // 0 - unknown, 1 - safe
  bool no_traps_left;

  // teleporting support
  bool pending_teleport;
  std::string pending_from;
  std::vector<std::string> used_teleporters;

  // disarming
  bool probing;
  Coordinates probe_cell;

  // the frontier cell we are walking to (kept so we do not change our mind
  // every turn)
  bool has_target;
  Coordinates explore_target;

  // path finding - bfs is used due to time mismanagement....
  std::vector<std::vector<int>> bfs_dist;
  std::vector<std::vector<int>> bfs_dir;

  // the other rogues
  std::vector<PeerInfo> peers;

  // map
  bool IsInBounds(int x, int y) const;
  std::string ConvertSymbol(const std::string& symbol) const;
  bool IsTeleporter(const std::string& s);
  bool IsSafe(int x, int y);
  bool IsFrontier(int x, int y);
  void ScanDirection(Coordinates cor, int dir,
                     const std::vector<std::string>& seen);
  void AddToMap(Coordinates cor, Percepts& percepts);
  void MarkSafe(const Percepts& percepts);

  // teleporting
  void ResetMap();
  void PostTeleport(const Percepts& percepts);
  int TimesUsed(const std::string& teleporter);

  // path finding
  void Bfs();
  std::string FirstStepTo(Coordinates target);
  bool FindNearestTreasure(Coordinates& target);
  bool FindNearestFrontier(Coordinates& target);
  bool FindNearestTeleporter(Coordinates& target);

  // behaviour
  std::vector<std::string> Decide(Percepts& percepts);
  std::vector<std::string> ExecuteCommands(std::vector<std::string> cmds,
                                           Percepts& percepts);
  bool LookBehind(std::string& cmd);
  bool ProbeNeighbors(std::string& cmd);

  // communication my beloved
  unsigned Encode(const std::string& s);
  std::string Decode(unsigned n);
  unsigned Nib(const std::vector<std::string>& cells, size_t i);
  uint32_t PackCells(const std::vector<std::string>& cells, int pos, int count);
  Coordinates CalcOtherPosition(const Percepts& percepts, size_t j);
  void LearnRotation(PeerInfo& peer, int their_rotation, unsigned their_move,
                     Coordinates now);
  void UnpackCells(Coordinates start, int dir, uint32_t word, int first_nibble,
                int count);
  void ReadPeers(const Percepts& percepts, AgentComm* comms);
  void Publish(const Percepts& percepts, AgentComm* comms, int old_rotation,
               const std::vector<std::string>& cmds);

 public:
  AI();
  AI(unsigned id, unsigned agent_speed, std::mt19937_64* rng, Symbols symbols,
     Costs costs, int max_turn);

  // printing
  template <typename T>
  void PrintMap(const std::vector<std::vector<T>>& map);
  void PrintPercepts(const Percepts& percepts);
  std::vector<std::string> Run(Percepts& percepts, AgentComm* comms);
};
