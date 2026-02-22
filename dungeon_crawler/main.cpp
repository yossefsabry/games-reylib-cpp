#include <raylib.h>

#include "game.h"
#include "render.h"

int main() {
  const int screenWidth = 1280;
  const int screenHeight = 720;

  InitWindow(screenWidth, screenHeight, "Cryptbound - Roguelike Dungeon");
  SetTargetFPS(60);

  Game game;
  InitGame(game, screenWidth, screenHeight);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    UpdateGame(game, dt);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawGame(game);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
