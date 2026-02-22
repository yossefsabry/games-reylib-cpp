#pragma once

#include <string>

struct GridPos {
  int x;
  int y;
};

inline bool operator==(GridPos a, GridPos b) {
  return a.x == b.x && a.y == b.y;
}

struct Room {
  int x;
  int y;
  int w;
  int h;

  GridPos Center() const {
    return GridPos{x + w / 2, y + h / 2};
  }

  bool Contains(GridPos p) const {
    return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
  }
};

enum class TileType { Wall, Floor, Door };
enum class ItemType { Potion, Gold };
enum class GameMode { Title, Playing, GameOver };

struct Actor {
  GridPos cell;
  GridPos prev;
  float moveT;
};

struct Player {
  Actor actor;
  int hp;
  int maxHp;
  int potions;
  int gold;
  int attack;
  int defense;
};

struct Enemy {
  Actor actor;
  int hp;
  int type;
};

struct Item {
  GridPos cell;
  ItemType type;
  int amount;
  bool picked;
};

struct LogLine {
  std::string text;
  float ttl;
};
