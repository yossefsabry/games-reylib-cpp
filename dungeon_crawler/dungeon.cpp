#include "dungeon.h"

#include <raylib.h>

static bool RoomsOverlap(const Room &a, const Room &b) {
  return a.x - 1 < b.x + b.w && a.x + a.w + 1 > b.x &&
         a.y - 1 < b.y + b.h && a.y + a.h + 1 > b.y;
}

bool InBounds(const Dungeon &dungeon, int x, int y) {
  return x >= 0 && y >= 0 && x < dungeon.width && y < dungeon.height;
}

int TileIndex(const Dungeon &dungeon, int x, int y) {
  return y * dungeon.width + x;
}

TileType GetTile(const Dungeon &dungeon, int x, int y) {
  return dungeon.tiles[TileIndex(dungeon, x, y)];
}

bool IsWalkable(const Dungeon &dungeon, int x, int y) {
  if (!InBounds(dungeon, x, y)) return false;
  return GetTile(dungeon, x, y) == TileType::Floor;
}

static void SetTile(Dungeon &dungeon, int x, int y, TileType type) {
  if (!InBounds(dungeon, x, y)) return;
  dungeon.tiles[TileIndex(dungeon, x, y)] = type;
}

static void CarveRoom(Dungeon &dungeon, const Room &room) {
  for (int y = room.y; y < room.y + room.h; y++) {
    for (int x = room.x; x < room.x + room.w; x++) {
      SetTile(dungeon, x, y, TileType::Floor);
    }
  }
}

static void CarveCorridor(Dungeon &dungeon, GridPos a, GridPos b) {
  int x = a.x;
  int y = a.y;
  while (x != b.x) {
    x += (b.x > x) ? 1 : -1;
    SetTile(dungeon, x, y, TileType::Floor);
  }
  while (y != b.y) {
    y += (b.y > y) ? 1 : -1;
    SetTile(dungeon, x, y, TileType::Floor);
  }
}

GridPos RandomFloorInRoom(const Dungeon &dungeon, const Room &room) {
  int x = GetRandomValue(room.x + 1, room.x + room.w - 2);
  int y = GetRandomValue(room.y + 1, room.y + room.h - 2);
  if (IsWalkable(dungeon, x, y)) return GridPos{x, y};
  return room.Center();
}

Dungeon GenerateDungeon(int width, int height, int seed) {
  SetRandomSeed(seed);
  Dungeon dungeon;
  dungeon.width = width;
  dungeon.height = height;
  dungeon.tiles.assign(width * height, TileType::Wall);
  dungeon.seen.assign(width * height, 0);
  dungeon.rooms.clear();

  int targetRooms = GetRandomValue(8, 12);
  int attempts = 0;
  while ((int)dungeon.rooms.size() < targetRooms && attempts < 120) {
    attempts++;
    int w = GetRandomValue(4, 8);
    int h = GetRandomValue(4, 7);
    int x = GetRandomValue(1, width - w - 2);
    int y = GetRandomValue(1, height - h - 2);
    Room room{x, y, w, h};

    bool overlap = false;
    for (const auto &r : dungeon.rooms) {
      if (RoomsOverlap(room, r)) {
        overlap = true;
        break;
      }
    }
    if (overlap) continue;

    CarveRoom(dungeon, room);
    if (!dungeon.rooms.empty()) {
      CarveCorridor(dungeon, dungeon.rooms.back().Center(), room.Center());
    }
    dungeon.rooms.push_back(room);
  }

  if (dungeon.rooms.empty()) {
    Room room{2, 2, width - 4, height - 4};
    CarveRoom(dungeon, room);
    dungeon.rooms.push_back(room);
  }

  dungeon.exit = dungeon.rooms.back().Center();
  return dungeon;
}
