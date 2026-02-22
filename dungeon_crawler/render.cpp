#include "render.h"

#include "ui.h"

#include <raylib.h>

#include <cmath>

static Color Tint(Color c, float f) {
  return Color{(unsigned char)(c.r * f), (unsigned char)(c.g * f),
               (unsigned char)(c.b * f), c.a};
}

static float EaseOut(float t) {
  float inv = 1.0f - t;
  return 1.0f - inv * inv;
}

static Vector2 ActorPixel(const Game &game, const Actor &actor) {
  float t = EaseOut(actor.moveT);
  float x = (actor.prev.x + (actor.cell.x - actor.prev.x) * t) * game.tileSize;
  float y = (actor.prev.y + (actor.cell.y - actor.prev.y) * t) * game.tileSize;
  return Vector2{x, y};
}

void DrawGame(const Game &game) {
  Color bgTop{16, 22, 32, 255};
  Color bgBottom{6, 10, 18, 255};
  DrawRectangleGradientV(0, 0, game.screenWidth, game.screenHeight, bgTop,
                         bgBottom);

  Vector2 jitter{0.0f, 0.0f};
  if (game.shake > 0.0f) {
    jitter.x = (float)GetRandomValue(-2, 2);
    jitter.y = (float)GetRandomValue(-2, 2);
  }

  BeginScissorMode((int)game.dungeonRect.x, (int)game.dungeonRect.y,
                   (int)game.dungeonRect.width, (int)game.dungeonRect.height);

  Color floorA{44, 46, 54, 255};
  Color floorB{50, 52, 60, 255};
  Color wallA{20, 22, 28, 255};
  Color wallB{32, 34, 40, 255};
  Color unseen{3, 4, 6, 255};
  Color exitColor{90, 120, 160, 255};

  for (int y = 0; y < game.dungeon.height; y++) {
    for (int x = 0; x < game.dungeon.width; x++) {
      int idx = TileIndex(game.dungeon, x, y);
      bool seen = game.dungeon.seen[idx] != 0;
      bool vis = game.visible[idx] != 0;
      float px = game.dungeonRect.x + jitter.x + x * game.tileSize;
      float py = game.dungeonRect.y + jitter.y + y * game.tileSize;

      if (!seen) {
        DrawRectangle((int)px, (int)py, game.tileSize, game.tileSize, unseen);
        continue;
      }

      Color base = ((x + y) % 2 == 0) ? floorA : floorB;
      if (GetTile(game.dungeon, x, y) == TileType::Wall) {
        base = ((x + y) % 2 == 0) ? wallA : wallB;
      }

      if (!vis) base = Tint(base, 0.45f);
      DrawRectangle((int)px, (int)py, game.tileSize, game.tileSize, base);

      if (GetTile(game.dungeon, x, y) == TileType::Wall && vis) {
        DrawRectangle((int)px, (int)py, game.tileSize, 4,
                      Tint(Color{120, 120, 140, 255}, 0.3f));
      }

      if (vis && game.dungeon.exit.x == x && game.dungeon.exit.y == y) {
        DrawRectangle((int)px + 6, (int)py + 6, game.tileSize - 12,
                      game.tileSize - 12, exitColor);
      }
    }
  }

  Vector2 lightCenter = ActorPixel(game, game.player.actor);
  lightCenter.x += game.tileSize * 0.5f + jitter.x;
  lightCenter.y += game.tileSize * 0.5f + jitter.y;
  DrawCircleGradient((int)lightCenter.x, (int)lightCenter.y, 140.0f,
                     Color{80, 110, 140, 50}, Color{0, 0, 0, 0});

  for (const auto &item : game.items) {
    int idx = TileIndex(game.dungeon, item.cell.x, item.cell.y);
    if (item.picked || game.visible[idx] == 0) continue;
    Vector2 pos = ActorPixel(game, Actor{item.cell, item.cell, 1.0f});
    pos.x += jitter.x;
    pos.y += jitter.y;
    Color c = item.type == ItemType::Gold ? Color{220, 190, 90, 255}
                                          : Color{170, 80, 140, 255};
    DrawCircle((int)(pos.x + game.tileSize * 0.5f),
               (int)(pos.y + game.tileSize * 0.5f), 6.0f, c);
    DrawCircleLines((int)(pos.x + game.tileSize * 0.5f),
                    (int)(pos.y + game.tileSize * 0.5f), 6.0f,
                    Color{30, 20, 10, 200});
  }

  for (const auto &enemy : game.enemies) {
    if (enemy.hp <= 0) continue;
    int idx = TileIndex(game.dungeon, enemy.actor.cell.x, enemy.actor.cell.y);
    if (game.visible[idx] == 0) continue;
    Vector2 pos = ActorPixel(game, enemy.actor);
    pos.x += jitter.x;
    pos.y += jitter.y;
    Color c = enemy.type == 0 ? Color{100, 180, 100, 255}
                               : Color{210, 200, 180, 255};
    DrawRectangle((int)(pos.x + 6), (int)(pos.y + 8), game.tileSize - 12,
                  game.tileSize - 12, c);
    DrawRectangleLines((int)(pos.x + 6), (int)(pos.y + 8), game.tileSize - 12,
                       game.tileSize - 12, Color{30, 20, 20, 180});
  }

  Vector2 playerPos = ActorPixel(game, game.player.actor);
  playerPos.x += jitter.x;
  playerPos.y += jitter.y;
  DrawCircle((int)(playerPos.x + game.tileSize * 0.5f),
             (int)(playerPos.y + game.tileSize * 0.5f), 10.0f,
             Color{220, 230, 245, 255});
  DrawCircleLines((int)(playerPos.x + game.tileSize * 0.5f),
                  (int)(playerPos.y + game.tileSize * 0.5f), 10.0f,
                  Color{40, 50, 70, 220});

  EndScissorMode();
  DrawUI(game);

  if (game.mode == GameMode::Title || game.mode == GameMode::GameOver) {
    DrawRectangle(0, 0, game.screenWidth, game.screenHeight,
                  Color{0, 0, 0, 160});
  }

  if (game.mode == GameMode::Title) {
    DrawText("CRYPTBOUND", 460, 200, 48, Color{230, 230, 240, 255});
    DrawText("Press Enter or A to begin", 420, 270, 20,
             Color{200, 200, 210, 255});
  }

  if (game.mode == GameMode::GameOver) {
    DrawText("YOU FELL", 520, 240, 48, Color{230, 210, 210, 255});
    DrawText("Press R or Start to retry", 410, 300, 20,
             Color{200, 200, 210, 255});
  }
}
