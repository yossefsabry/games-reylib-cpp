#pragma once

#include "types.h"

#include <cstdint>
#include <vector>

struct Dungeon {
  int width;
  int height;
  std::vector<TileType> tiles;
  std::vector<uint8_t> seen;
  std::vector<Room> rooms;
  GridPos exit;
};

Dungeon GenerateDungeon(int width, int height, int seed);
bool InBounds(const Dungeon &dungeon, int x, int y);
int TileIndex(const Dungeon &dungeon, int x, int y);
TileType GetTile(const Dungeon &dungeon, int x, int y);
bool IsWalkable(const Dungeon &dungeon, int x, int y);
GridPos RandomFloorInRoom(const Dungeon &dungeon, const Room &room);
