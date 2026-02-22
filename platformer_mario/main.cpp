#include <raylib.h>
#include <cmath>
#include <vector>

using namespace std;

struct Platform {
  Rectangle rect;
  Color color;
  int kind;
};

struct Coin {
  Vector2 pos;
  bool collected;
};

struct Hazard {
  Rectangle rect;
};

struct Player {
  Vector2 pos;
  Vector2 size;
  Vector2 vel;
  bool onGround;
  int coins;
};

static float Clampf(float v, float min, float max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

static float MoveTowards(float current, float target, float maxDelta) {
  float delta = target - current;
  if (fabsf(delta) <= maxDelta) return target;
  return current + (delta > 0.0f ? maxDelta : -maxDelta);
}

static Rectangle PlayerRect(const Player &p) {
  return Rectangle{p.pos.x, p.pos.y, p.size.x, p.size.y};
}

static unsigned char ClampColor(int v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (unsigned char)v;
}

static Color ShadeColor(Color c, int dr, int dg, int db, int da = 0) {
  return Color{ClampColor((int)c.r + dr), ClampColor((int)c.g + dg),
               ClampColor((int)c.b + db), ClampColor((int)c.a + da)};
}

static void DrawGroundPlatform(const Rectangle &rect, Color base) {
  Color top = ShadeColor(base, 28, 24, 12);
  Color edge = ShadeColor(base, -32, -28, -24);
  DrawRectangleRec(rect, base);
  DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, 10, top);
  for (float x = rect.x + 6; x < rect.x + rect.width; x += 26) {
    DrawTriangle(Vector2{x, rect.y + 10}, Vector2{x + 8, rect.y + 2},
                 Vector2{x + 16, rect.y + 10}, top);
  }
  DrawRectangleLinesEx(rect, 2.0f, edge);
  for (float y = rect.y + 18; y < rect.y + rect.height; y += 18) {
    DrawLine((int)rect.x + 6, (int)y, (int)(rect.x + rect.width - 8),
             (int)y, ShadeColor(base, -18, -20, -22));
  }
}

static void DrawBrickPlatform(const Rectangle &rect, Color base) {
  Color mortar = ShadeColor(base, -30, -30, -30);
  DrawRectangleRec(rect, base);
  DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, 5,
                ShadeColor(base, 24, 12, 6));
  DrawRectangleLinesEx(rect, 2.0f, mortar);
  float brickW = 28.0f;
  float brickH = 14.0f;
  int row = 0;
  for (float y = rect.y + 4; y < rect.y + rect.height; y += brickH) {
    float offset = (row % 2 == 0) ? 0.0f : brickW * 0.5f;
    for (float x = rect.x + offset; x < rect.x + rect.width; x += brickW) {
      DrawRectangleLines((int)x, (int)y, (int)brickW, (int)brickH, mortar);
    }
    row++;
  }
}

static void DrawPipePlatform(const Rectangle &rect, Color base) {
  Color dark = ShadeColor(base, -38, -48, -34);
  Color light = ShadeColor(base, 32, 40, 18);
  DrawRectangleGradientH((int)rect.x, (int)rect.y, (int)rect.width,
                         (int)rect.height, light, dark);
  Rectangle rim{rect.x - 8.0f, rect.y - 14.0f, rect.width + 16.0f, 14.0f};
  DrawRectangleRec(rim, base);
  DrawRectangleLinesEx(rim, 2.0f, dark);
  DrawRectangle((int)(rect.x + rect.width * 0.18f), (int)rect.y + 6,
                (int)(rect.width * 0.18f), (int)rect.height - 12, light);
}

static void DrawCoinSprite(const Coin &coin, float t, Color base) {
  float wobble = sinf(t * 6.0f + coin.pos.x * 0.05f) * 4.0f;
  float spin = fabsf(sinf(t * 4.2f + coin.pos.y * 0.04f));
  float width = 8.0f + spin * 8.0f;
  DrawEllipse((int)coin.pos.x, (int)(coin.pos.y + wobble), width, 12.0f, base);
  DrawEllipseLines((int)coin.pos.x, (int)(coin.pos.y + wobble), width, 12.0f,
                   ShadeColor(base, -40, -30, -20));
  DrawEllipse((int)(coin.pos.x - 2), (int)(coin.pos.y + wobble - 3),
              width * 0.25f, 3.0f, ShadeColor(base, 40, 40, 20));
}

static void DrawPlayerSprite(const Player &player, float t) {
  float bob = sinf(t * 8.0f + player.pos.x * 0.02f) * 1.5f;
  Rectangle body{player.pos.x, player.pos.y + bob, player.size.x,
                 player.size.y};
  DrawEllipse((int)(player.pos.x + player.size.x * 0.5f),
              (int)(player.pos.y + player.size.y + 6),
              player.size.x * 0.55f, 8.0f, Color{0, 0, 0, 70});
  DrawRectangleRounded(body, 0.2f, 6, Color{215, 50, 52, 255});
  DrawRectangle((int)body.x + 4, (int)body.y + 10, (int)body.width - 8,
                (int)body.height - 16, Color{248, 212, 166, 255});
  DrawRectangle((int)body.x - 2, (int)body.y + 6, (int)body.width + 4, 10,
                Color{150, 10, 20, 255});
  DrawRectangle((int)body.x + 6, (int)body.y + 22, 4, 4,
                Color{40, 20, 10, 255});
  DrawRectangle((int)body.x + 20, (int)body.y + 22, 4, 4,
                Color{40, 20, 10, 255});
  DrawRectangle((int)body.x + 3, (int)(body.y + body.height - 10), 12, 8,
                Color{70, 40, 20, 255});
  DrawRectangle((int)body.x + (int)body.width - 15,
                (int)(body.y + body.height - 10), 12, 8,
                Color{70, 40, 20, 255});
}

static void ResetGame(Player &player, vector<Coin> &coins, bool &win, bool &dead,
                      float &timer, float groundY) {
  player.pos = Vector2{80.0f, groundY - 48.0f};
  player.size = Vector2{34.0f, 48.0f};
  player.vel = Vector2{0.0f, 0.0f};
  player.onGround = false;
  player.coins = 0;
  win = false;
  dead = false;
  timer = 0.0f;
  for (auto &coin : coins) coin.collected = false;
}

int main() {
  const int screenWidth = 1280;
  const int screenHeight = 720;
  const float worldWidth = 4000.0f;
  const float worldHeight = 900.0f;
  const float groundY = 800.0f;

  InitWindow(screenWidth, screenHeight, "Platformer Mario - raylib");
  SetTargetFPS(60);

  Color skyTop = Color{92, 166, 255, 255};
  Color skyBottom = Color{206, 238, 255, 255};
  Color sunCore = Color{255, 242, 190, 255};
  Color sunGlow = Color{255, 232, 170, 120};
  Color groundColor = Color{210, 150, 70, 255};
  Color brickColor = Color{196, 92, 50, 255};
  Color pipeColor = Color{76, 176, 92, 255};
  Color hazardColor = Color{232, 68, 60, 255};
  Color coinColor = Color{244, 206, 70, 255};
  Color uiBack = Color{16, 30, 52, 200};
  Color uiBorder = Color{255, 255, 255, 80};
  Color uiText = Color{240, 246, 255, 230};
  Color uiSub = Color{180, 208, 235, 230};
  Color cloudColor = Color{255, 255, 255, 220};

  const int PLATFORM_GROUND = 0;
  const int PLATFORM_BRICK = 1;
  const int PLATFORM_PIPE = 2;

  vector<Platform> platforms;
  auto addPlatform = [&](float x, float y, float w, float h, Color c,
                         int kind) {
    platforms.push_back(Platform{Rectangle{x, y, w, h}, c, kind});
  };

  addPlatform(0.0f, groundY, 800.0f, 100.0f, groundColor, PLATFORM_GROUND);
  addPlatform(950.0f, groundY, 700.0f, 100.0f, groundColor, PLATFORM_GROUND);
  addPlatform(1850.0f, groundY, 900.0f, 100.0f, groundColor, PLATFORM_GROUND);
  addPlatform(3000.0f, groundY, 1000.0f, 100.0f, groundColor, PLATFORM_GROUND);

  addPlatform(280.0f, 660.0f, 200.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(620.0f, 590.0f, 180.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(980.0f, 520.0f, 180.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(1300.0f, 620.0f, 240.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(1700.0f, 700.0f, 160.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(2100.0f, 610.0f, 240.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(2480.0f, 540.0f, 200.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(2800.0f, 680.0f, 200.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(3150.0f, 590.0f, 220.0f, 32.0f, brickColor, PLATFORM_BRICK);
  addPlatform(3500.0f, 510.0f, 240.0f, 32.0f, brickColor, PLATFORM_BRICK);

  addPlatform(1120.0f, 690.0f, 80.0f, 110.0f, pipeColor, PLATFORM_PIPE);
  addPlatform(2240.0f, 660.0f, 90.0f, 140.0f, pipeColor, PLATFORM_PIPE);
  addPlatform(3320.0f, 680.0f, 90.0f, 120.0f, pipeColor, PLATFORM_PIPE);

  vector<Hazard> hazards;
  hazards.push_back(Hazard{Rectangle{1120.0f, 770.0f, 90.0f, 30.0f}});
  hazards.push_back(Hazard{Rectangle{2060.0f, 770.0f, 120.0f, 30.0f}});
  hazards.push_back(Hazard{Rectangle{2650.0f, 770.0f, 120.0f, 30.0f}});

  vector<Coin> coins = {
      Coin{Vector2{320.0f, 620.0f}, false},
      Coin{Vector2{680.0f, 550.0f}, false},
      Coin{Vector2{1020.0f, 480.0f}, false},
      Coin{Vector2{1380.0f, 580.0f}, false},
      Coin{Vector2{1720.0f, 660.0f}, false},
      Coin{Vector2{2140.0f, 570.0f}, false},
      Coin{Vector2{2540.0f, 500.0f}, false},
      Coin{Vector2{2860.0f, 640.0f}, false},
      Coin{Vector2{3220.0f, 550.0f}, false},
      Coin{Vector2{3600.0f, 470.0f}, false},
  };

  Rectangle goal = Rectangle{3820.0f, 520.0f, 24.0f, 260.0f};
  Rectangle goalBase = Rectangle{3795.0f, 780.0f, 70.0f, 20.0f};

  Player player;
  bool win = false;
  bool dead = false;
  float timer = 0.0f;
  ResetGame(player, coins, win, dead, timer, groundY);

  Camera2D camera = {};
  camera.offset = Vector2{screenWidth / 2.0f, screenHeight / 2.0f};
  camera.zoom = 1.0f;

  const float accel = 2400.0f;
  const float maxSpeed = 420.0f;
  const float friction = 2100.0f;
  const float jumpSpeed = 720.0f;
  const float gravity = 1800.0f;
  const float maxFall = 1200.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    if (dt > 0.033f) dt = 0.033f;
    float t = (float)GetTime();

    if (!win && !dead) {
      timer += dt;
      float move = 0.0f;
      if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move -= 1.0f;
      if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move += 1.0f;

      if (move != 0.0f) {
        player.vel.x += move * accel * dt;
        player.vel.x = Clampf(player.vel.x, -maxSpeed, maxSpeed);
      } else {
        player.vel.x = MoveTowards(player.vel.x, 0.0f, friction * dt);
      }

      if (player.onGround && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                              IsKeyPressed(KEY_W))) {
        player.vel.y = -jumpSpeed;
        player.onGround = false;
      }

      player.vel.y += gravity * dt;
      if (player.vel.y > maxFall) player.vel.y = maxFall;

      player.pos.x += player.vel.x * dt;
      Rectangle rect = PlayerRect(player);
      for (const auto &plat : platforms) {
        if (CheckCollisionRecs(rect, plat.rect)) {
          if (player.vel.x > 0.0f) {
            player.pos.x = plat.rect.x - player.size.x;
          } else if (player.vel.x < 0.0f) {
            player.pos.x = plat.rect.x + plat.rect.width;
          }
          player.vel.x = 0.0f;
          rect = PlayerRect(player);
        }
      }

      player.pos.y += player.vel.y * dt;
      rect = PlayerRect(player);
      player.onGround = false;
      for (const auto &plat : platforms) {
        if (CheckCollisionRecs(rect, plat.rect)) {
          if (player.vel.y > 0.0f) {
            player.pos.y = plat.rect.y - player.size.y;
            player.onGround = true;
          } else if (player.vel.y < 0.0f) {
            player.pos.y = plat.rect.y + plat.rect.height;
          }
          player.vel.y = 0.0f;
          rect = PlayerRect(player);
        }
      }

      for (const auto &hazard : hazards) {
        if (CheckCollisionRecs(rect, hazard.rect)) dead = true;
      }

      for (auto &coin : coins) {
        if (!coin.collected &&
            CheckCollisionCircleRec(coin.pos, 12.0f, rect)) {
          coin.collected = true;
          player.coins++;
        }
      }

      if (CheckCollisionRecs(rect, goal)) win = true;
      if (player.pos.y > worldHeight + 200.0f) dead = true;
    } else {
      if (IsKeyPressed(KEY_R)) {
        ResetGame(player, coins, win, dead, timer, groundY);
      }
    }

    Vector2 target = Vector2{player.pos.x + player.size.x / 2.0f,
                             player.pos.y + player.size.y / 2.0f};
    float viewHalfW = screenWidth * 0.5f / camera.zoom;
    float viewHalfH = screenHeight * 0.5f / camera.zoom;
    if (worldWidth <= viewHalfW * 2.0f) {
      target.x = worldWidth * 0.5f;
    } else {
      target.x = Clampf(target.x, viewHalfW, worldWidth - viewHalfW);
    }
    if (worldHeight <= viewHalfH * 2.0f) {
      target.y = worldHeight * 0.5f;
    } else {
      target.y = Clampf(target.y, viewHalfH, worldHeight - viewHalfH);
    }
    camera.target = target;

    BeginDrawing();
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, skyTop, skyBottom);
    DrawCircleGradient(screenWidth - 150, 120, 120, sunGlow,
                       Color{255, 255, 255, 0});
    DrawCircleGradient(screenWidth - 150, 120, 70, sunCore,
                       Color{255, 255, 255, 0});

    float mountainOffset = fmodf(camera.target.x * 0.12f, 520.0f);
    for (int i = -1; i < 6; i++) {
      float x = i * 520.0f - mountainOffset;
      DrawTriangle(Vector2{x, 520}, Vector2{x + 260.0f, 280},
                   Vector2{x + 520.0f, 520}, Color{120, 170, 210, 255});
      DrawTriangle(Vector2{x + 180.0f, 560}, Vector2{x + 420.0f, 320},
                   Vector2{x + 660.0f, 560}, Color{104, 154, 198, 255});
    }

    float cloudOffset = fmodf(camera.target.x * 0.25f, 360.0f);
    for (int i = -2; i < 8; i++) {
      float x = i * 320.0f - cloudOffset;
      float y = 80.0f + (i % 3) * 28.0f;
      DrawEllipse((int)(x + 60.0f), (int)y, 48.0f, 18.0f, cloudColor);
      DrawEllipse((int)(x + 95.0f), (int)(y + 6.0f), 58.0f, 20.0f,
                  cloudColor);
      DrawEllipse((int)(x + 135.0f), (int)(y + 2.0f), 45.0f, 16.0f,
                  cloudColor);
    }

    BeginMode2D(camera);
    for (int i = 0; i < 14; i++) {
      float hillX = i * 320.0f;
      DrawCircle((int)hillX, 820, 220, Color{112, 188, 122, 255});
      DrawCircle((int)(hillX + 160.0f), 850, 200, Color{94, 172, 108, 255});
    }

    for (const auto &plat : platforms) {
      if (plat.kind == PLATFORM_GROUND) {
        DrawGroundPlatform(plat.rect, plat.color);
      } else if (plat.kind == PLATFORM_BRICK) {
        DrawBrickPlatform(plat.rect, plat.color);
      } else {
        DrawPipePlatform(plat.rect, plat.color);
      }
    }

    for (const auto &hazard : hazards) {
      DrawRectangleGradientV((int)hazard.rect.x, (int)hazard.rect.y,
                             (int)hazard.rect.width, (int)hazard.rect.height,
                             ShadeColor(hazardColor, -10, -10, -10),
                             hazardColor);
      int spikeCount = (int)(hazard.rect.width / 18.0f);
      if (spikeCount < 3) spikeCount = 3;
      float spikeW = hazard.rect.width / spikeCount;
      for (int i = 0; i < spikeCount; i++) {
        float spikeX = hazard.rect.x + i * spikeW;
        Vector2 a{spikeX + 2.0f, hazard.rect.y + hazard.rect.height};
        Vector2 b{spikeX + spikeW - 2.0f, hazard.rect.y + hazard.rect.height};
        Vector2 c{spikeX + spikeW * 0.5f, hazard.rect.y + 6.0f};
        DrawTriangle(a, b, c, ShadeColor(hazardColor, 20, 10, 10));
      }
      DrawRectangleLinesEx(hazard.rect, 2.0f, ShadeColor(hazardColor, -40, -30, -30));
    }

    for (const auto &coin : coins) {
      if (coin.collected) continue;
      DrawCoinSprite(coin, t, coinColor);
    }

    DrawRectangleRec(goal, Color{245, 245, 245, 255});
    DrawRectangleRec(goalBase, Color{90, 60, 40, 255});
    float flagWave = sinf(t * 2.4f) * 8.0f;
    DrawTriangle(Vector2{goal.x + goal.width, goal.y + 30.0f},
                 Vector2{goal.x + goal.width + 70.0f,
                         goal.y + 60.0f + flagWave},
                 Vector2{goal.x + goal.width, goal.y + 90.0f},
                 Color{220, 30, 50, 255});
    DrawCircle((int)(goal.x + goal.width), (int)(goal.y + 20.0f), 6.0f,
               Color{240, 210, 120, 255});

    DrawPlayerSprite(player, t);

    EndMode2D();

    Rectangle hud{18.0f, 18.0f, 320.0f, 112.0f};
    DrawRectangleRounded(hud, 0.2f, 10, uiBack);
    DrawRectangleRoundedLines(hud, 0.2f, 10, uiBorder);
    DrawText("GREEN RIDGE", (int)hud.x + 16, (int)hud.y + 10, 18, uiText);

    DrawCircle((int)hud.x + 28, (int)hud.y + 50, 10, coinColor);
    DrawCircleLines((int)hud.x + 28, (int)hud.y + 50, 10,
                    ShadeColor(coinColor, -40, -30, -20));
    DrawText(TextFormat("%02i", player.coins), (int)hud.x + 46,
             (int)hud.y + 42, 22, uiText);

    int clockX = (int)hud.x + 150;
    int clockY = (int)hud.y + 50;
    DrawCircleLines(clockX, clockY, 10, uiSub);
    DrawLine(clockX, clockY, clockX, clockY - 5, uiSub);
    DrawLine(clockX, clockY, clockX + 4, clockY + 2, uiSub);
    DrawText(TextFormat("%.1fs", timer), clockX + 16, clockY - 8, 20, uiText);

    DrawText("LEVEL 1-1", (int)hud.x + 16, (int)hud.y + 78, 14, uiSub);
    Rectangle bar{hud.x + 96.0f, hud.y + 82.0f, hud.width - 120.0f, 10.0f};
    DrawRectangleRounded(bar, 0.3f, 6, ShadeColor(uiBack, 22, 26, 34, 0));
    float progress = Clampf((player.pos.x + player.size.x) / (worldWidth - 100.0f),
                            0.0f, 1.0f);
    Rectangle fill{bar.x, bar.y, bar.width * progress, bar.height};
    DrawRectangleRounded(fill, 0.3f, 6, Color{90, 200, 255, 230});

    Rectangle hint{18.0f, screenHeight - 44.0f, 460.0f, 28.0f};
    DrawRectangleRounded(hint, 0.2f, 8, Color{0, 0, 0, 120});
    DrawText("A/D or Arrow to move. Space jump. R restart.",
             (int)hint.x + 12, (int)hint.y + 6, 16, uiSub);

    if (win || dead) {
      DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 120});
      Rectangle card{screenWidth / 2.0f - 200.0f, screenHeight / 2.0f - 110.0f,
                     400.0f, 180.0f};
      DrawRectangleRounded(card, 0.2f, 12, Color{16, 30, 52, 230});
      DrawRectangleRoundedLines(card, 0.2f, 12, uiBorder);
      const char *title = win ? "LEVEL CLEAR" : "GAME OVER";
      DrawText(title, (int)card.x + 92, (int)card.y + 30, 30, uiText);
      DrawText(TextFormat("Coins: %i", player.coins), (int)card.x + 60,
               (int)card.y + 78, 22, uiSub);
      DrawText(TextFormat("Time: %.1fs", timer), (int)card.x + 220,
               (int)card.y + 78, 22, uiSub);
      DrawText("Press R to restart", (int)card.x + 90, (int)card.y + 120, 20,
               uiText);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
