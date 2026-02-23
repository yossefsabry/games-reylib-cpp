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

struct Level {
  float worldWidth;
  float worldHeight;
  float groundY;
  vector<Platform> platforms;
  vector<Hazard> hazards;
  vector<Coin> coins;
  Rectangle goal;
  Rectangle goalBase;
};

const int PLATFORM_GROUND = 0;
const int PLATFORM_BRICK = 1;
const int PLATFORM_PIPE = 2;

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

static Rectangle UnionRect(Rectangle a, Rectangle b) {
  float minX = (a.x < b.x) ? a.x : b.x;
  float minY = (a.y < b.y) ? a.y : b.y;
  float maxX = (a.x + a.width > b.x + b.width) ? a.x + a.width
                                               : b.x + b.width;
  float maxY = (a.y + a.height > b.y + b.height) ? a.y + a.height
                                                 : b.y + b.height;
  return Rectangle{minX, minY, maxX - minX, maxY - minY};
}

static float Hash01(int n) {
  float s = sinf((float)n * 12.9898f) * 43758.5453f;
  return s - floorf(s);
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

static void DrawPlayerSprite(const Player &player, float t, bool onGround,
                             float velX, float velY) {
  float speed = fabsf(velX);
  bool idle = onGround && speed < 12.0f;
  bool jumping = !onGround;
  float bob = idle ? sinf(t * 6.0f + player.pos.x * 0.02f) * 1.2f : 0.0f;
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

  float look = Clampf(velX / 260.0f, -1.0f, 1.0f);
  float lookY = Clampf(velY / 600.0f, -1.0f, 1.0f);
  float eyeY = body.y + 22.0f + (jumping ? -2.0f : 0.0f) + lookY * 1.2f;
  float leftX = body.x + 7.0f + look * 2.0f;
  float rightX = body.x + 20.0f + look * 2.0f;

  bool blink = idle && fmodf(t + player.pos.x * 0.01f, 3.6f) < 0.12f;
  if (jumping) {
    float tilt = velY < -120.0f ? -1.0f : (velY > 200.0f ? 1.0f : 0.0f);
    DrawLine((int)leftX, (int)eyeY, (int)(leftX + 4),
             (int)(eyeY + 1 + tilt),
             Color{40, 20, 10, 255});
    DrawLine((int)rightX, (int)eyeY, (int)(rightX + 4),
             (int)(eyeY + 1 + tilt),
             Color{40, 20, 10, 255});
  } else if (blink) {
    DrawLine((int)leftX, (int)eyeY + 1, (int)(leftX + 4), (int)eyeY + 1,
             Color{40, 20, 10, 255});
    DrawLine((int)rightX, (int)eyeY + 1, (int)(rightX + 4), (int)eyeY + 1,
             Color{40, 20, 10, 255});
  } else {
    DrawRectangle((int)leftX, (int)eyeY, 4, 4, Color{40, 20, 10, 255});
    DrawRectangle((int)rightX, (int)eyeY, 4, 4, Color{40, 20, 10, 255});
    if (speed > 40.0f) {
      DrawLine((int)leftX - 1, (int)eyeY - 2, (int)(leftX + 4), (int)eyeY - 1,
               Color{120, 70, 50, 200});
      DrawLine((int)rightX - 1, (int)eyeY - 2, (int)(rightX + 4),
               (int)eyeY - 1, Color{120, 70, 50, 200});
    }
  }

  DrawRectangle((int)body.x + 3, (int)(body.y + body.height - 10), 12, 8,
                Color{70, 40, 20, 255});
  DrawRectangle((int)body.x + (int)body.width - 15,
                (int)(body.y + body.height - 10), 12, 8,
                Color{70, 40, 20, 255});
}

static void ResetGame(Player &player, Level &level, bool &win, bool &dead,
                      float &timer) {
  player.pos = Vector2{80.0f, level.groundY - 48.0f};
  player.size = Vector2{34.0f, 48.0f};
  player.vel = Vector2{0.0f, 0.0f};
  player.onGround = false;
  player.coins = 0;
  win = false;
  dead = false;
  timer = 0.0f;
  for (auto &coin : level.coins) coin.collected = false;
}

static Level BuildLevel(int index, Color groundColor, Color brickColor,
                        Color pipeColor) {
  Level level;
  level.worldHeight = 900.0f;
  level.groundY = 800.0f;
  int seed = 1000 + index * 97 + index * index * 7;
  float targetWidth = 2600.0f + index * 140.0f + index * index * 3.0f;
  level.worldWidth = targetWidth;

  const float groundHeight = 100.0f;
  const float hazardHeight = 28.0f;
  const float safeZoneX = 520.0f;

  vector<Rectangle> groundSegments;

  auto addPlatform = [&](float x, float y, float w, float h, Color c, int kind) {
    level.platforms.push_back(Platform{Rectangle{x, y, w, h}, c, kind});
  };

  auto overlapsPlatform = [&](Rectangle rect) {
    for (const auto &plat : level.platforms) {
      if (CheckCollisionRecs(rect, plat.rect)) return true;
    }
    return false;
  };

  auto tooCloseToPlatform = [&](Rectangle rect, float marginX, float marginY) {
    Rectangle padded{rect.x - marginX, rect.y - marginY,
                     rect.width + marginX * 2.0f,
                     rect.height + marginY * 2.0f};
    for (const auto &plat : level.platforms) {
      if (CheckCollisionRecs(padded, plat.rect)) return true;
    }
    return false;
  };

  float x = 0.0f;
  float startWidth = 720.0f;
  addPlatform(x, level.groundY, startWidth, groundHeight, groundColor,
              PLATFORM_GROUND);
  groundSegments.push_back(Rectangle{x, level.groundY, startWidth, groundHeight});
  x += startWidth + 140.0f;

  float maxGap = 170.0f + index * 1.6f;
  if (maxGap > 240.0f) maxGap = 240.0f;
  float minGap = 110.0f + index * 0.9f;
  if (minGap > maxGap - 30.0f) minGap = maxGap - 30.0f;

  int seg = 0;
  while (x < targetWidth - 700.0f) {
    float noise = Hash01(seed + seg * 13 + 5);
    float width = Clampf(520.0f - index * 6.0f + noise * 80.0f, 300.0f, 560.0f);
    float gapNoise = Hash01(seed + seg * 31 + 11);
    float gap = minGap + (maxGap - minGap) * gapNoise;
    addPlatform(x, level.groundY, width, groundHeight, groundColor,
                PLATFORM_GROUND);
    groundSegments.push_back(Rectangle{x, level.groundY, width, groundHeight});
    x += width + gap;
    seg++;
  }

  float finalWidth = 700.0f;
  if (x + finalWidth < targetWidth) {
    float fillerWidth = Clampf(targetWidth - finalWidth - x, 300.0f, 520.0f);
    addPlatform(x, level.groundY, fillerWidth, groundHeight, groundColor,
                PLATFORM_GROUND);
    groundSegments.push_back(
        Rectangle{x, level.groundY, fillerWidth, groundHeight});
    x += fillerWidth + 160.0f;
  }
  level.worldWidth = x + finalWidth;
  addPlatform(x, level.groundY, finalWidth, groundHeight, groundColor,
              PLATFORM_GROUND);
  groundSegments.push_back(Rectangle{x, level.groundY, finalWidth, groundHeight});

  int brickTarget = (int)(level.worldWidth / 380.0f) + index / 3;
  int brickAttempts = brickTarget * 4;
  int bricksPlaced = 0;
  for (int i = 0; i < brickAttempts && bricksPlaced < brickTarget; i++) {
    float step = level.worldWidth / (brickTarget + 2.0f);
    float jitter = (Hash01(seed + i * 17 + 23) - 0.5f) * 160.0f;
    float bx = 160.0f + (i + 1) * step + jitter;
    if (bx < safeZoneX + 120.0f) continue;
    float width = Clampf(220.0f - index * 1.6f + Hash01(seed + i * 7) * 44.0f,
                         160.0f, 240.0f);
    float lift = 160.0f + (i % 4) * 40.0f + index * 1.2f;
    float by = Clampf(level.groundY - lift, 500.0f, level.groundY - 150.0f);
    Rectangle rect{bx, by, width, 32.0f};
    if (overlapsPlatform(rect)) continue;
    if (tooCloseToPlatform(rect, 28.0f, 18.0f)) continue;
    addPlatform(bx, by, width, 32.0f, brickColor, PLATFORM_BRICK);
    if (i % 2 == 0) {
      level.coins.push_back(Coin{Vector2{bx + width * 0.5f, by - 26.0f}, false});
    }
    bricksPlaced++;
  }

  int pipeTarget = 1 + index / 4;
  int pipeAttempts = pipeTarget * 5;
  int pipesPlaced = 0;
  for (int i = 0; i < pipeAttempts && pipesPlaced < pipeTarget; i++) {
    if (groundSegments.empty()) break;
    Rectangle segRect =
        groundSegments[(i * 3 + index) % (int)groundSegments.size()];
    if (segRect.x < safeZoneX) continue;
    float pWidth = 80.0f + (i % 2) * 10.0f;
    float pHeight = Clampf(110.0f + index * 5.0f + (i % 3) * 14.0f, 110.0f,
                           190.0f);
    float usable = segRect.width - pWidth - 20.0f;
    if (usable < 10.0f) continue;
    float px = segRect.x + 10.0f + Hash01(seed + i * 29) * usable;
    Rectangle rect{px, level.groundY - pHeight, pWidth, pHeight};
    if (overlapsPlatform(rect)) continue;
    if (tooCloseToPlatform(rect, 18.0f, 12.0f)) continue;
    addPlatform(px, level.groundY - pHeight, pWidth, pHeight, pipeColor,
                PLATFORM_PIPE);
    level.coins.push_back(
        Coin{Vector2{px + pWidth * 0.5f, level.groundY - pHeight - 26.0f},
             false});
    pipesPlaced++;
  }

  int hazardTarget = 2 + index / 2;
  int attempts = hazardTarget * 4;
  int placed = 0;
  for (int i = 0; i < attempts && placed < hazardTarget; i++) {
    if (groundSegments.empty()) break;
    Rectangle segRect =
        groundSegments[(i * 2 + index) % (int)groundSegments.size()];
    if (segRect.x < safeZoneX + 80.0f) continue;
    if (segRect.x + segRect.width > level.worldWidth - 460.0f) continue;
    float hWidth = 80.0f + (i % 3) * 20.0f;
    float usable = segRect.width - hWidth - 80.0f;
    if (usable < 10.0f) continue;
    float hx = segRect.x + 40.0f + Hash01(seed + i * 41) * usable;
    Rectangle hRect{hx, level.groundY - hazardHeight, hWidth, hazardHeight};
    bool blocked = false;
    for (const auto &plat : level.platforms) {
      if (plat.kind == PLATFORM_PIPE && CheckCollisionRecs(hRect, plat.rect)) {
        blocked = true;
        break;
      }
      if (plat.kind == PLATFORM_BRICK && CheckCollisionRecs(hRect, plat.rect)) {
        blocked = true;
        break;
      }
    }
    if (blocked) continue;
    if (tooCloseToPlatform(hRect, 26.0f, 12.0f)) continue;
    level.hazards.push_back(Hazard{hRect});
    placed++;
  }

  if (placed == 0 && !groundSegments.empty()) {
    Rectangle segRect = groundSegments[(int)groundSegments.size() / 2];
    float hWidth = 100.0f;
    float hx = segRect.x + segRect.width * 0.5f - hWidth * 0.5f;
    if (hx < safeZoneX + 80.0f) hx = safeZoneX + 120.0f;
    Rectangle hRect{hx, level.groundY - hazardHeight, hWidth, hazardHeight};
    level.hazards.push_back(Hazard{hRect});
  }

  level.goal = Rectangle{level.worldWidth - 180.0f, level.groundY - 280.0f, 24.0f,
                         260.0f};
  level.goalBase =
      Rectangle{level.worldWidth - 205.0f, level.groundY - 20.0f, 70.0f, 20.0f};

  return level;
}

int main() {
  int screenWidth = 1280;
  int screenHeight = 720;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
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
  Color uiBack = Color{34, 28, 24, 210};
  Color uiBack2 = Color{52, 40, 32, 220};
  Color uiBorder = Color{255, 255, 255, 70};
  Color uiText = Color{250, 244, 232, 230};
  Color uiSub = Color{220, 204, 184, 230};
  Color uiAccent = Color{232, 86, 52, 240};
  Color cloudColor = Color{255, 255, 255, 220};

  const int totalLevels = 20;
  int levelIndex = 0;
  Level level = BuildLevel(levelIndex, groundColor, brickColor, pipeColor);

  Player player;
  bool win = false;
  bool dead = false;
  float timer = 0.0f;
  ResetGame(player, level, win, dead, timer);

  Camera2D camera = {};
  camera.offset = Vector2{screenWidth / 2.0f, screenHeight / 2.0f};
  camera.zoom = 1.0f;

  const float accel = 2400.0f;
  const float maxSpeed = 420.0f;
  const float friction = 2100.0f;
  const float jumpSpeed = 720.0f;
  const float gravity = 1800.0f;
  const float maxFall = 1200.0f;
  const float maxJumpHold = 0.18f;
  const float jumpHoldGravityScale = 0.35f;
  float jumpHoldTime = 0.0f;
  bool jumpHolding = false;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    if (dt > 0.033f) dt = 0.033f;
    float t = (float)GetTime();
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    camera.offset = Vector2{screenWidth * 0.5f, screenHeight * 0.5f};
    if (IsKeyPressed(KEY_Q)) break;
    static bool showPadDebug = false;
    if (IsKeyPressed(KEY_F3)) showPadDebug = !showPadDebug;

    int activePad = -1;
    const int maxPads = 4;
    for (int i = 0; i < maxPads; i++) {
      if (IsGamepadAvailable(i)) {
        activePad = i;
        break;
      }
    }
    bool hasPad = activePad >= 0;

    if (!win && !dead) {
      timer += dt;
      Rectangle prevRect = PlayerRect(player);
      float move = 0.0f;
      if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move -= 1.0f;
      if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move += 1.0f;

      if (hasPad) {
        float axis = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_LEFT_X);
        float axisAlt = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_RIGHT_X);
        if (fabsf(axisAlt) > fabsf(axis)) axis = axisAlt;
        float deadzone = 0.2f;
        if (fabsf(axis) > deadzone) {
          move = axis;
        }
        if (IsGamepadButtonDown(activePad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
          move = -1.0f;
        if (IsGamepadButtonDown(activePad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
          move = 1.0f;
      }

      if (move != 0.0f) {
        player.vel.x += move * accel * dt;
        player.vel.x = Clampf(player.vel.x, -maxSpeed, maxSpeed);
      } else {
        player.vel.x = MoveTowards(player.vel.x, 0.0f, friction * dt);
      }

      bool jumpPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                         IsKeyPressed(KEY_W);
      bool jumpHeld = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) ||
                      IsKeyDown(KEY_W);
      if (hasPad) {
        jumpPressed = jumpPressed ||
                      IsGamepadButtonPressed(activePad,
                                             GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                      IsGamepadButtonPressed(activePad,
                                             GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        jumpHeld = jumpHeld ||
                   IsGamepadButtonDown(activePad,
                                       GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                   IsGamepadButtonDown(activePad,
                                       GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
      }
      if (player.onGround && jumpPressed) {
        player.vel.y = -jumpSpeed;
        player.onGround = false;
        jumpHoldTime = 0.0f;
        jumpHolding = true;
      }

      if (!player.onGround && jumpHolding) {
        if (jumpHeld && player.vel.y < 0.0f) {
          jumpHoldTime += dt;
          if (jumpHoldTime > maxJumpHold) jumpHolding = false;
        } else {
          jumpHolding = false;
        }
      }

      float gravityScale = 1.0f;
      if (jumpHolding && player.vel.y < 0.0f) gravityScale = jumpHoldGravityScale;
      player.vel.y += gravity * gravityScale * dt;
      if (player.vel.y > maxFall) player.vel.y = maxFall;

      player.pos.x += player.vel.x * dt;
      Rectangle rect = PlayerRect(player);
      for (const auto &plat : level.platforms) {
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
      for (const auto &plat : level.platforms) {
        if (CheckCollisionRecs(rect, plat.rect)) {
          if (player.vel.y > 0.0f) {
            player.pos.y = plat.rect.y - player.size.y;
            player.onGround = true;
            jumpHolding = false;
          } else if (player.vel.y < 0.0f) {
            player.pos.y = plat.rect.y + plat.rect.height;
            jumpHolding = false;
          }
          player.vel.y = 0.0f;
          rect = PlayerRect(player);
        }
      }

      Rectangle sweep = UnionRect(prevRect, rect);
      for (const auto &hazard : level.hazards) {
        if (CheckCollisionRecs(rect, hazard.rect) ||
            CheckCollisionRecs(sweep, hazard.rect)) {
          dead = true;
        }
      }

      for (auto &coin : level.coins) {
        if (!coin.collected &&
            CheckCollisionCircleRec(coin.pos, 12.0f, rect)) {
          coin.collected = true;
          player.coins++;
        }
      }

      if (CheckCollisionRecs(rect, level.goal)) win = true;
      if (player.pos.y > level.worldHeight + 200.0f) dead = true;
    } else {
      bool nextPressed = IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ENTER) ||
                         IsKeyPressed(KEY_KP_ENTER);
      if (win && nextPressed) {
        if (levelIndex < totalLevels - 1) {
          levelIndex++;
        } else {
          levelIndex = 0;
        }
        level = BuildLevel(levelIndex, groundColor, brickColor, pipeColor);
        ResetGame(player, level, win, dead, timer);
      }
      if (IsKeyPressed(KEY_R)) {
        levelIndex = 0;
        level = BuildLevel(levelIndex, groundColor, brickColor, pipeColor);
        ResetGame(player, level, win, dead, timer);
      }
    }

    Vector2 target = Vector2{player.pos.x + player.size.x / 2.0f,
                             player.pos.y + player.size.y / 2.0f};
    float viewHalfW = screenWidth * 0.5f / camera.zoom;
    float viewHalfH = screenHeight * 0.5f / camera.zoom;
    if (level.worldWidth <= viewHalfW * 2.0f) {
      target.x = level.worldWidth * 0.5f;
    } else {
      target.x = Clampf(target.x, viewHalfW, level.worldWidth - viewHalfW);
    }
    if (level.worldHeight <= viewHalfH * 2.0f) {
      target.y = level.worldHeight * 0.5f;
    } else {
      target.y = Clampf(target.y, viewHalfH, level.worldHeight - viewHalfH);
    }
    camera.target = target;

    BeginDrawing();
    float dayShift = (float)levelIndex / (float)(totalLevels - 1);
    Color skyTopLevel = ShadeColor(skyTop, (int)(dayShift * 10),
                                   (int)(dayShift * -6),
                                   (int)(dayShift * -18));
    Color skyBottomLevel = ShadeColor(skyBottom, (int)(dayShift * 16),
                                      (int)(dayShift * 4),
                                      (int)(dayShift * -20));
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, skyTopLevel,
                           skyBottomLevel);
    float skyFlow = fmodf(t * 12.0f, 360.0f);
    for (int i = 0; i < 6; i++) {
      float bandY = 40.0f + i * 60.0f + sinf(t * 0.4f + i) * 6.0f;
      float bandX = -120.0f + fmodf(skyFlow + i * 80.0f, 360.0f);
      DrawRectangleGradientH((int)bandX, (int)bandY, 380, 22,
                             Color{255, 255, 255, 18},
                             Color{255, 255, 255, 0});
      DrawRectangleGradientH((int)(bandX + 420.0f), (int)(bandY + 8.0f), 320, 18,
                             Color{255, 255, 255, 14},
                             Color{255, 255, 255, 0});
    }

    Vector2 sunPos{(float)screenWidth - 150.0f, 120.0f};
    for (int i = 0; i < 8; i++) {
      float ang = i * (PI / 4.0f) + t * 0.05f;
      Vector2 a{sunPos.x + cosf(ang) * 160.0f, sunPos.y + sinf(ang) * 160.0f};
      Vector2 b{sunPos.x + cosf(ang + 0.22f) * 160.0f,
                sunPos.y + sinf(ang + 0.22f) * 160.0f};
      DrawTriangle(sunPos, a, b, Color{255, 235, 170, 35});
    }
    DrawCircleGradient((int)sunPos.x, (int)sunPos.y, 130, sunGlow,
                       Color{255, 255, 255, 0});
    DrawCircleGradient((int)sunPos.x, (int)sunPos.y, 75, sunCore,
                       Color{255, 255, 255, 0});
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight,
                           Color{255, 255, 255, 0}, Color{255, 255, 255, 36});
    for (int i = 0; i < 4; i++) {
      float fogX = (float)(i * 380) - fmodf(camera.target.x * 0.08f, 380.0f);
      float fogY = screenHeight * 0.58f + (i % 2) * 18.0f;
      DrawEllipse((int)fogX, (int)fogY, 320.0f, 70.0f,
                  Color{255, 255, 255, 26});
      DrawEllipse((int)(fogX + 160.0f), (int)(fogY + 12.0f), 260.0f, 60.0f,
                  Color{255, 255, 255, 22});
    }

    float mountainOffset = fmodf(camera.target.x * 0.12f + t * 6.0f, 520.0f);
    for (int i = -1; i < 6; i++) {
      float x = i * 520.0f - mountainOffset;
      DrawTriangle(Vector2{x, 520}, Vector2{x + 260.0f, 280},
                   Vector2{x + 520.0f, 520}, Color{120, 170, 210, 255});
      DrawTriangle(Vector2{x + 180.0f, 560}, Vector2{x + 420.0f, 320},
                   Vector2{x + 660.0f, 560}, Color{104, 154, 198, 255});
    }

    float cloudOffset = fmodf(camera.target.x * 0.25f + t * 18.0f, 360.0f);
    for (int i = -2; i < 8; i++) {
      float x = i * 320.0f - cloudOffset;
      float y = 80.0f + (i % 3) * 28.0f;
      DrawEllipse((int)(x + 60.0f), (int)y, 48.0f, 18.0f, cloudColor);
      DrawEllipse((int)(x + 95.0f), (int)(y + 6.0f), 58.0f, 20.0f,
                  cloudColor);
      DrawEllipse((int)(x + 135.0f), (int)(y + 2.0f), 45.0f, 16.0f,
                  cloudColor);
    }

    for (int i = 0; i < 6; i++) {
      float bx = fmodf(camera.target.x * 0.12f + i * 180.0f + t * 20.0f,
                       (float)screenWidth + 220.0f) -
                 110.0f;
      float by = 120.0f + (i % 3) * 34.0f;
      Color bird = Color{60, 80, 110, 120};
      DrawLine((int)bx, (int)by, (int)(bx + 10.0f), (int)(by + 4.0f), bird);
      DrawLine((int)(bx + 10.0f), (int)(by + 4.0f), (int)(bx + 20.0f),
               (int)by, bird);
    }

    BeginMode2D(camera);
    for (int i = 0; i < 14; i++) {
      float hillX = i * 320.0f;
      DrawCircle((int)hillX, 820, 220, Color{112, 188, 122, 255});
      DrawCircle((int)(hillX + 160.0f), 850, 200, Color{94, 172, 108, 255});
    }

    for (const auto &plat : level.platforms) {
      if (plat.kind == PLATFORM_GROUND) {
        DrawGroundPlatform(plat.rect, plat.color);
      } else if (plat.kind == PLATFORM_BRICK) {
        DrawBrickPlatform(plat.rect, plat.color);
      } else {
        DrawPipePlatform(plat.rect, plat.color);
      }
    }

    for (const auto &hazard : level.hazards) {
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

    for (const auto &coin : level.coins) {
      if (coin.collected) continue;
      DrawCoinSprite(coin, t, coinColor);
    }

    DrawRectangleRec(level.goal, Color{245, 245, 245, 255});
    DrawRectangleRec(level.goalBase, Color{90, 60, 40, 255});
    float flagWave = sinf(t * 2.4f) * 8.0f;
    DrawTriangle(Vector2{level.goal.x + level.goal.width, level.goal.y + 30.0f},
                 Vector2{level.goal.x + level.goal.width + 70.0f,
                         level.goal.y + 60.0f + flagWave},
                 Vector2{level.goal.x + level.goal.width, level.goal.y + 90.0f},
                 Color{220, 30, 50, 255});
    DrawCircle((int)(level.goal.x + level.goal.width),
               (int)(level.goal.y + 20.0f), 6.0f,
               Color{240, 210, 120, 255});

    DrawPlayerSprite(player, t, player.onGround, player.vel.x, player.vel.y);

    EndMode2D();

    float hudWidth = (float)screenWidth - 40.0f;
    if (hudWidth < 260.0f) hudWidth = (float)screenWidth - 20.0f;
    if (hudWidth < 220.0f) hudWidth = 220.0f;
    Rectangle hudBar{20.0f, 14.0f, hudWidth, 64.0f};
    DrawRectangleRounded(hudBar, 0.18f, 12, uiBack);
    Rectangle hudInner{hudBar.x + 2.0f, hudBar.y + 2.0f, hudBar.width - 4.0f,
                       hudBar.height - 4.0f};
    DrawRectangleGradientH((int)hudInner.x, (int)hudInner.y, (int)hudInner.width,
                           (int)hudInner.height, uiBack2, uiBack);
    DrawRectangleRoundedLines(hudBar, 0.18f, 12, uiBorder);
    DrawRectangle((int)hudBar.x + 14, (int)hudBar.y + 8, 130, 4, uiAccent);

    DrawText("GREEN RIDGE", (int)hudBar.x + 16, (int)hudBar.y + 18, 20, uiText);
    DrawText(TextFormat("LEVEL %02i/%02i", levelIndex + 1, totalLevels),
             (int)hudBar.x + 18, (int)hudBar.y + 40, 12, uiSub);

    int coinX = (int)hudBar.x + 260;
    int coinY = (int)hudBar.y + 36;
    DrawCircle(coinX, coinY, 9, coinColor);
    DrawCircleLines(coinX, coinY, 9, ShadeColor(coinColor, -40, -30, -20));
    DrawText(TextFormat("%02i", player.coins), coinX + 16, coinY - 10, 22, uiText);

    int timeX = coinX + 120;
    int timeY = coinY;
    DrawCircleLines(timeX, timeY, 9, uiSub);
    DrawLine(timeX, timeY, timeX, timeY - 6, uiSub);
    DrawLine(timeX, timeY, timeX + 5, timeY + 2, uiSub);
    DrawText(TextFormat("%.1fs", timer), timeX + 16, timeY - 10, 20, uiText);

    float progress = Clampf(
        (player.pos.x + player.size.x) / (level.worldWidth - 100.0f), 0.0f, 1.0f);
    float barW = 220.0f;
    if (barW > hudBar.width - 220.0f) barW = hudBar.width - 220.0f;
    if (barW < 120.0f) barW = 120.0f;
    Rectangle bar{hudBar.x + hudBar.width - barW - 32.0f, hudBar.y + 38.0f, barW,
                  10.0f};
    DrawRectangleRounded(bar, 0.3f, 6, ShadeColor(uiBack2, 14, 12, 8));
    for (int i = 1; i < 5; i++) {
      float tickX = bar.x + bar.width * ((float)i / 5.0f);
      DrawLine((int)tickX, (int)bar.y - 2, (int)tickX,
               (int)(bar.y + bar.height + 2),
               ShadeColor(uiBack2, 28, 26, 22));
    }
    Rectangle fill{bar.x, bar.y, bar.width * progress, bar.height};
    DrawRectangleRounded(fill, 0.3f, 6, Color{124, 206, 120, 230});
    DrawText("GOAL", (int)(bar.x + bar.width - 34), (int)hudBar.y + 16, 12, uiSub);
    float flagX = bar.x + bar.width + 10.0f;
    DrawLine((int)flagX, (int)bar.y - 8, (int)flagX, (int)(bar.y + 12), uiSub);
    DrawTriangle(Vector2{flagX, bar.y - 6}, Vector2{flagX + 12.0f, bar.y - 2},
                 Vector2{flagX, bar.y + 2}, uiAccent);

    float hintW = screenWidth - 40.0f;
    if (hintW > 700.0f) hintW = 700.0f;
    Rectangle hint{20.0f, screenHeight - 40.0f, hintW, 24.0f};
    DrawRectangleRounded(hint, 0.25f, 8, ShadeColor(uiBack, 10, 8, 6));
    DrawText("Move A/D, Arrow, Stick  Jump Space/A (hold)  Restart R  Next N  Quit Q",
             (int)hint.x + 12,
             (int)hint.y + 5, 14, uiSub);

    if (showPadDebug) {
      float dbgX = screenWidth - 300.0f;
      if (dbgX < 10.0f) dbgX = 10.0f;
      Rectangle dbg{dbgX, 90.0f, 280.0f, 170.0f};
      DrawRectangleRounded(dbg, 0.18f, 8, Color{0, 0, 0, 140});
      DrawRectangleRoundedLines(dbg, 0.18f, 8, uiBorder);
      const char *padName = hasPad ? GetGamepadName(activePad) : "None";
      DrawText(TextFormat("Pad: %i", activePad), (int)dbg.x + 12, (int)dbg.y + 10,
               14, uiText);
      DrawText(TextFormat("Name: %s", padName ? padName : "Unknown"),
               (int)dbg.x + 12, (int)dbg.y + 28, 12, uiSub);
      if (hasPad) {
        float lx = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_LEFT_Y);
        float rx = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_RIGHT_X);
        float ry = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_RIGHT_Y);
        float lt = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_LEFT_TRIGGER);
        float rt = GetGamepadAxisMovement(activePad, GAMEPAD_AXIS_RIGHT_TRIGGER);
        DrawText(TextFormat("LX %.2f  LY %.2f", lx, ly), (int)dbg.x + 12,
                 (int)dbg.y + 52, 12, uiSub);
        DrawText(TextFormat("RX %.2f  RY %.2f", rx, ry), (int)dbg.x + 12,
                 (int)dbg.y + 68, 12, uiSub);
        DrawText(TextFormat("LT %.2f  RT %.2f", lt, rt), (int)dbg.x + 12,
                 (int)dbg.y + 84, 12, uiSub);

        int lineY = (int)dbg.y + 104;
        DrawText("Buttons:", (int)dbg.x + 12, lineY, 12, uiText);
        lineY += 16;
        DrawText(TextFormat("DPad L/R: %i %i",
                             IsGamepadButtonDown(activePad,
                                                 GAMEPAD_BUTTON_LEFT_FACE_LEFT),
                             IsGamepadButtonDown(activePad,
                                                 GAMEPAD_BUTTON_LEFT_FACE_RIGHT)),
                 (int)dbg.x + 12, lineY, 12, uiSub);
        lineY += 14;
        DrawText(TextFormat("A/B: %i %i",
                             IsGamepadButtonDown(activePad,
                                                 GAMEPAD_BUTTON_RIGHT_FACE_DOWN),
                             IsGamepadButtonDown(activePad,
                                                 GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)),
                 (int)dbg.x + 12, lineY, 12, uiSub);
      }
      DrawText("F3: toggle pad debug", (int)dbg.x + 12, (int)dbg.y + 148, 12,
               uiSub);
    }

    if (win || dead) {
      DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 120});
      Rectangle card{screenWidth / 2.0f - 210.0f, screenHeight / 2.0f - 110.0f,
                     420.0f, 190.0f};
      DrawRectangleRounded(card, 0.22f, 12, uiBack2);
      DrawRectangleRoundedLines(card, 0.22f, 12, uiBorder);
      DrawRectangle((int)card.x, (int)card.y, (int)card.width, 8, uiAccent);
      const char *title = win ? "LEVEL CLEAR" : "GAME OVER";
      DrawText(title, (int)card.x + 96, (int)card.y + 36, 30, uiText);
      DrawText(TextFormat("Coins: %i", player.coins), (int)card.x + 70,
               (int)card.y + 88, 22, uiSub);
      DrawText(TextFormat("Time: %.1fs", timer), (int)card.x + 230,
               (int)card.y + 88, 22, uiSub);
      if (win) {
        const char *nextHint = "Press N for next level";
        if (levelIndex == totalLevels - 1) {
          nextHint = "Press N to play again";
        }
        DrawText(nextHint, (int)card.x + 92, (int)card.y + 124, 18, uiText);
        DrawText("Press R to replay", (int)card.x + 126, (int)card.y + 148, 18,
                 uiSub);
      } else {
        DrawText("Press R to retry", (int)card.x + 126, (int)card.y + 136, 18,
                 uiSub);
      }
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
