#include "ui.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

static void DrawOutlinedText(const char *text, int x, int y, int size,
                             Color color) {
  DrawText(text, x + 1, y + 1, size, Color{10, 8, 12, 220});
  DrawText(text, x, y, size, color);
}

static void DrawPlate(Rectangle rec, Color fill, Color border, Color highlight,
                      Color shadow) {
  DrawRectangleRec(rec, fill);
  DrawRectangleLinesEx(rec, 2.0f, border);
  DrawRectangle((int)rec.x + 1, (int)rec.y + 1, (int)rec.width - 2, 2,
                highlight);
  DrawRectangle((int)rec.x + 1, (int)(rec.y + rec.height - 3),
                (int)rec.width - 2, 2, shadow);
}

static void DrawRivets(Rectangle rec, int spacing) {
  int y = (int)(rec.y + 6);
  int right = (int)(rec.x + rec.width - 6);
  for (int x = (int)rec.x + 8; x < right; x += spacing) {
    DrawCircle(x, y, 2.0f, Color{70, 60, 90, 220});
    DrawCircle(x + 1, y + 1, 1.0f, Color{20, 16, 26, 200});
  }
}

static int ClampInt(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

static void DrawBadge(Rectangle rec, const char *label) {
  DrawRectangleRec(rec, Color{46, 38, 54, 220});
  DrawRectangleLinesEx(rec, 2.0f, Color{90, 76, 110, 200});
  DrawOutlinedText(label, (int)rec.x + 8, (int)rec.y + 4, 14,
                   Color{220, 210, 230, 255});
}

static void DrawHeart(int x, int y, int scale, float fill) {
  static const char *pattern[6] = {
      "0110110", "1111111", "1111111", "0111110", "0011100", "0001000"};
  int w = 7;
  int h = 6;
  float clamped = std::max(0.0f, std::min(1.0f, fill));
  int filledCols = (int)std::round(w * clamped);
  Color base{64, 36, 44, 255};
  Color full{210, 70, 70, 255};
  for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
      if (pattern[py][px] != '1') continue;
      Color c = px < filledCols ? full : base;
      DrawRectangle(x + px * scale, y + py * scale, scale, scale, c);
    }
  }
}

static void DrawPotion(int x, int y, int scale) {
  Color glass{70, 120, 150, 255};
  Color liquid{200, 120, 210, 255};
  DrawRectangle(x + scale, y + scale, scale * 2, scale * 3, glass);
  DrawRectangle(x + scale, y + scale * 2, scale * 2, scale * 2, liquid);
  DrawRectangle(x + scale, y, scale * 2, scale, Color{40, 40, 48, 255});
  DrawRectangleLines(x + scale, y + scale, scale * 2, scale * 3,
                     Color{20, 18, 24, 200});
}

static void DrawCoin(int x, int y, int radius) {
  DrawCircle(x, y, radius, Color{220, 190, 90, 255});
  DrawCircleLines(x, y, radius, Color{100, 70, 30, 255});
  DrawCircle(x - radius / 3, y - radius / 3, radius / 5,
             Color{255, 240, 200, 220});
}

static void DrawLogOverlay(const Game &game, Rectangle rec) {
  if (game.log.empty()) return;
  DrawRectangleGradientV((int)rec.x, (int)rec.y, (int)rec.width,
                         (int)rec.height, Color{18, 16, 24, 200},
                         Color{10, 8, 16, 200});
  DrawRectangleLinesEx(rec, 2.0f, Color{60, 52, 78, 220});
  DrawRivets(rec, 24);

  int maxLines = 2;
  int start = 0;
  if ((int)game.log.size() > maxLines) start = game.log.size() - maxLines;
  int lineY = (int)rec.y + 10;
  for (int i = start; i < (int)game.log.size(); i++) {
    DrawOutlinedText(game.log[i].text.c_str(), (int)rec.x + 12, lineY, 16,
                     Color{210, 210, 220, 255});
    lineY += 20;
  }
}

void DrawUI(const Game &game) {
  Rectangle bar = game.uiRect;
  DrawRectangleGradientV((int)bar.x, (int)bar.y, (int)bar.width,
                         (int)bar.height, Color{30, 24, 34, 220},
                         Color{20, 16, 26, 220});
  DrawRectangleLinesEx(bar, 2.0f, Color{60, 50, 70, 220});
  DrawRivets(bar, 36);

  int padding = (int)std::max(10.0f, bar.height * 0.2f);
  int plateH = (int)bar.height - 12;
  int plateY = (int)bar.y + 6;
  int heartScale = (int)std::max(2.0f, std::min(4.0f, bar.height / 18.0f));
  int heartW = 7 * heartScale;
  int heartH = 6 * heartScale;
  int hearts = (game.player.maxHp + 3) / 4;
  int heartGap = 6;
  int heartsBlockW = hearts > 0 ? hearts * (heartW + heartGap) - heartGap : 0;
  int potionStep = heartScale * 6;
  int minLeftW = 180;
  int leftContentW = heartsBlockW + potionStep * 2 + padding * 2;
  int leftW = ClampInt(leftContentW, minLeftW, (int)(bar.width * 0.55f));
  int leftX = (int)bar.x + padding;
  Rectangle leftPlate{(float)leftX, (float)plateY, (float)leftW,
                      (float)plateH};
  DrawPlate(leftPlate, Color{34, 28, 40, 220}, Color{70, 60, 80, 200},
            Color{100, 86, 120, 120}, Color{16, 12, 22, 200});

  int x = leftX + padding;
  int y = (int)(bar.y + (bar.height - heartH) * 0.5f - 2.0f);
  for (int i = 0; i < hearts; i++) {
    int hpChunk = game.player.hp - i * 4;
    float fill = std::max(0.0f, std::min(1.0f, hpChunk / 4.0f));
    DrawHeart(x, y, heartScale, fill);
    x += heartW + heartGap;
  }

  int potionsSpace = leftX + leftW - padding - x;
  int maxPotions = std::max(0, potionsSpace / potionStep);
  int showPotions = std::min(game.player.potions, maxPotions);
  for (int i = 0; i < showPotions; i++) {
    DrawPotion(x, y - heartScale, heartScale);
    x += potionStep;
  }
  if (game.player.potions > showPotions) {
    DrawOutlinedText(TextFormat("x%i", game.player.potions), x + 2,
                     y + 2, 16, Color{220, 210, 230, 255});
  }

  int goldTextW = MeasureText(TextFormat("%i", game.player.gold), 20);
  int levelTextW = MeasureText(TextFormat("Lv %i", game.floor), 20);
  int rightW = ClampInt(goldTextW + levelTextW + 110, 180,
                        (int)(bar.width * 0.32f));
  int rightX = (int)(bar.x + bar.width - rightW - padding);
  Rectangle rightPlate{(float)rightX, (float)plateY, (float)rightW,
                       (float)plateH};
  DrawPlate(rightPlate, Color{34, 28, 40, 220}, Color{70, 60, 80, 200},
            Color{100, 86, 120, 120}, Color{16, 12, 22, 200});

  int goldX = (int)(rightPlate.x + 16);
  DrawCoin(goldX, (int)(bar.y + bar.height * 0.5f), 8);
  DrawOutlinedText(TextFormat("%i", game.player.gold), goldX + 16,
                   (int)(bar.y + 16), 20, Color{240, 230, 210, 255});
  DrawOutlinedText(TextFormat("Lv %i", game.floor),
                   (int)(rightPlate.x + rightPlate.width - levelTextW - 16),
                   (int)(bar.y + 16), 20, Color{230, 230, 235, 255});

  int centerX = leftX + leftW + padding;
  int centerRight = rightX - padding;
  int centerW = centerRight - centerX;
  if (centerW > 160) {
    Rectangle centerPlate{(float)centerX, (float)plateY + 2.0f,
                          (float)centerW, (float)plateH - 4.0f};
    DrawPlate(centerPlate, Color{32, 26, 38, 220}, Color{70, 60, 80, 200},
              Color{100, 86, 120, 120}, Color{16, 12, 22, 200});
    DrawBadge(Rectangle{centerPlate.x + 10, centerPlate.y + 6, 70, 24},
              "TURN");
    DrawOutlinedText(TextFormat("%i", game.turn), (int)centerPlate.x + 90,
                     (int)centerPlate.y + 6, 20,
                     Color{220, 210, 230, 255});
    DrawOutlinedText("CRYPTBOUND", (int)centerPlate.x + 8,
                     (int)centerPlate.y + 30, 16,
                     Color{200, 190, 210, 255});
  }

  float logW = std::min(420.0f, game.dungeonRect.width * 0.6f);
  Rectangle logRec{game.dungeonRect.x + 12,
                   game.dungeonRect.y + game.dungeonRect.height - 64,
                   logW, 54};
  DrawLogOverlay(game, logRec);
}
