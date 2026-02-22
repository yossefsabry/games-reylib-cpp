#include "ui.h"

#include <raylib.h>

#include <algorithm>

static void DrawPanel(Rectangle rec, Color fill, Color border) {
  DrawRectangleRounded(rec, 0.08f, 10, fill);
  DrawRectangleRoundedLines(rec, 0.08f, 10, border);
}

static void DrawBar(Rectangle rec, float pct, Color fill, Color back) {
  DrawRectangleRounded(rec, 0.2f, 6, back);
  Rectangle inner = rec;
  inner.width *= std::max(0.0f, std::min(1.0f, pct));
  DrawRectangleRounded(inner, 0.2f, 6, fill);
}

static void DrawMiniMap(const Game &game, Rectangle rec) {
  int w = game.dungeon.width;
  int h = game.dungeon.height;
  float tile = std::min((rec.width - 20) / w, (rec.height - 36) / h);
  float startX = rec.x + (rec.width - tile * w) * 0.5f;
  float startY = rec.y + 26 + (rec.height - 32 - tile * h) * 0.5f;

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = TileIndex(game.dungeon, x, y);
      if (game.dungeon.seen[idx] == 0) continue;
      Color c = GetTile(game.dungeon, x, y) == TileType::Wall
                    ? Color{40, 44, 52, 255}
                    : Color{90, 96, 108, 255};
      if (game.visible[idx] == 0) c = Color{30, 32, 40, 255};
      DrawRectangle((int)(startX + x * tile), (int)(startY + y * tile),
                    (int)tile, (int)tile, c);
    }
  }

  Vector2 p = {(float)game.player.actor.cell.x, (float)game.player.actor.cell.y};
  DrawRectangle((int)(startX + p.x * tile), (int)(startY + p.y * tile),
                (int)tile, (int)tile, Color{120, 180, 220, 255});
  DrawRectangle((int)(startX + game.dungeon.exit.x * tile),
                (int)(startY + game.dungeon.exit.y * tile), (int)tile,
                (int)tile, Color{120, 160, 200, 255});
}

static void DrawLog(const Game &game, Rectangle rec) {
  int maxLines = 6;
  int start = 0;
  if ((int)game.log.size() > maxLines) start = game.log.size() - maxLines;
  int lineY = (int)rec.y + 30;
  for (int i = start; i < (int)game.log.size(); i++) {
    DrawText(game.log[i].text.c_str(), (int)rec.x + 12, lineY, 16,
             Color{200, 200, 210, 255});
    lineY += 20;
  }

  int hintY = (int)(rec.y + rec.height - 54);
  DrawText("Move: WASD / D-Pad", (int)rec.x + 12, hintY, 14,
           Color{160, 170, 180, 255});
  DrawText("Potion: H / X  Wait: Space / Y", (int)rec.x + 12, hintY + 18, 14,
           Color{160, 170, 180, 255});
}

void DrawUI(const Game &game) {
  Rectangle panel = game.uiRect;
  DrawRectangleGradientV((int)panel.x, (int)panel.y, (int)panel.width,
                         (int)panel.height, Color{18, 20, 28, 255},
                         Color{12, 14, 22, 255});
  DrawRectangleLinesEx(panel, 2.0f, Color{40, 46, 60, 255});

  float margin = 16.0f;
  float cardW = panel.width - margin * 2.0f;
  DrawText("CRYPTBOUND", (int)(panel.x + margin), 24, 24,
           Color{220, 220, 230, 255});
  DrawText(TextFormat("Floor %i  Turn %i", game.floor, game.turn),
           (int)(panel.x + margin), 52, 16, Color{170, 180, 190, 255});

  Rectangle stats{panel.x + margin, 80, cardW, 150};
  Rectangle map{panel.x + margin, 250, cardW, 190};
  Rectangle log{panel.x + margin, 460, cardW, 230};
  DrawPanel(stats, Color{22, 26, 34, 255}, Color{50, 60, 74, 255});
  DrawPanel(map, Color{22, 26, 34, 255}, Color{50, 60, 74, 255});
  DrawPanel(log, Color{22, 26, 34, 255}, Color{50, 60, 74, 255});

  DrawText("Vitality", (int)(stats.x + 12), (int)(stats.y + 12), 16,
           Color{190, 200, 210, 255});
  Rectangle bar{stats.x + 12, stats.y + 36, stats.width - 24, 18};
  DrawBar(bar, (float)game.player.hp / game.player.maxHp,
          Color{180, 70, 70, 255}, Color{60, 30, 30, 255});
  DrawText(TextFormat("HP %i / %i", game.player.hp, game.player.maxHp),
           (int)(stats.x + 12), (int)(stats.y + 62), 16,
           Color{210, 210, 220, 255});

  DrawText(TextFormat("Potions: %i", game.player.potions),
           (int)(stats.x + 12), (int)(stats.y + 90), 16,
           Color{190, 200, 210, 255});
  DrawText(TextFormat("Gold: %i", game.player.gold), (int)(stats.x + 12),
           (int)(stats.y + 110), 16, Color{190, 200, 210, 255});
  DrawText(TextFormat("Atk %i  Def %i", game.player.attack,
                      game.player.defense),
           (int)(stats.x + 12), (int)(stats.y + 130), 16,
           Color{190, 200, 210, 255});

  DrawText("Map", (int)(map.x + 12), (int)(map.y + 12), 16,
           Color{190, 200, 210, 255});
  DrawMiniMap(game, map);

  DrawText("Log", (int)(log.x + 12), (int)(log.y + 12), 16,
           Color{190, 200, 210, 255});
  DrawLog(game, log);
}
