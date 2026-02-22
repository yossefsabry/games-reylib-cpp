#pragma once

#include "dungeon.h"
#include "input.h"
#include "types.h"

#include <raylib.h>

#include <cstdint>
#include <vector>

struct Game {
  GameMode mode;
  int screenWidth;
  int screenHeight;
  int tileSize;
  Rectangle dungeonRect;
  Rectangle uiRect;
  float animTime;

  Dungeon dungeon;
  Player player;
  std::vector<Enemy> enemies;
  std::vector<Item> items;
  std::vector<LogLine> log;
  std::vector<uint8_t> visible;
  InputState input;

  int turn;
  int floor;
  float shake;
};

void InitGame(Game &game, int screenWidth, int screenHeight);
void UpdateGame(Game &game, float dt);
