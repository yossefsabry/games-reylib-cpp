#include "dungeon.h"

#include <raylib.h>

#include <vector>

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
  TileType tile = GetTile(dungeon, x, y);
  return tile == TileType::Floor || tile == TileType::Door;
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

static std::vector<GridPos> BuildCorridorPath(GridPos a, GridPos b) {
  std::vector<GridPos> path;
  int x = a.x;
  int y = a.y;
  path.push_back(GridPos{x, y});
  while (x != b.x) {
    x += (b.x > x) ? 1 : -1;
    path.push_back(GridPos{x, y});
  }
  while (y != b.y) {
    y += (b.y > y) ? 1 : -1;
    path.push_back(GridPos{x, y});
  }
  return path;
}

static void CarvePath(Dungeon &dungeon, const std::vector<GridPos> &path) {
  for (const auto &cell : path) {
    SetTile(dungeon, cell.x, cell.y, TileType::Floor);
  }
}

static void PlaceDoorAtBoundary(Dungeon &dungeon, const Room &room,
                                const std::vector<GridPos> &path,
                                bool fromStart) {
  if (path.size() < 2) return;
  if (fromStart) {
    for (size_t i = 1; i < path.size(); i++) {
      if (room.Contains(path[i - 1]) && !room.Contains(path[i])) {
        SetTile(dungeon, path[i - 1].x, path[i - 1].y, TileType::Door);
        return;
      }
    }
  } else {
    for (size_t i = path.size() - 1; i > 0; i--) {
      if (room.Contains(path[i]) && !room.Contains(path[i - 1])) {
        SetTile(dungeon, path[i].x, path[i].y, TileType::Door);
        return;
      }
    }
  }
}

GridPos RandomFloorInRoom(const Dungeon &dungeon, const Room &room) {
  int x = GetRandomValue(room.x + 1, room.x + room.w - 2);
  int y = GetRandomValue(room.y + 1, room.y + room.h - 2);
  if (GetTile(dungeon, x, y) == TileType::Floor) return GridPos{x, y};
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
      GridPos a = dungeon.rooms.back().Center();
      GridPos b = room.Center();
      std::vector<GridPos> path = BuildCorridorPath(a, b);
      CarvePath(dungeon, path);
      PlaceDoorAtBoundary(dungeon, dungeon.rooms.back(), path, true);
      PlaceDoorAtBoundary(dungeon, room, path, false);
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
