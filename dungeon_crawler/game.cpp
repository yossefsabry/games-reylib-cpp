#include "game_internal.h"

#include <algorithm>
#include <cmath>

static void UpdateLayout(Game &game, int screenWidth, int screenHeight) {
  game.screenWidth = screenWidth;
  game.screenHeight = screenHeight;

  float uiHeight = std::max(52.0f, std::min(80.0f, screenHeight * 0.1f));
  float margin = 12.0f;
  float availW = std::max(100.0f, screenWidth - margin * 2.0f);
  float availH = std::max(100.0f, screenHeight - uiHeight - margin * 2.0f);
  int tile = (int)std::floor(std::min(availW / 32.0f, availH / 24.0f));
  if (tile < 12) tile = 12;
  if (tile > 40) tile = 40;

  game.tileSize = tile;
  float dungeonW = game.tileSize * 32.0f;
  float dungeonH = game.tileSize * 24.0f;
  float dungeonX = (screenWidth - dungeonW) * 0.5f;
  float dungeonY = uiHeight +
                   std::max(0.0f, (screenHeight - uiHeight - dungeonH) * 0.5f);
  game.dungeonRect = Rectangle{dungeonX, dungeonY, dungeonW, dungeonH};
  game.uiRect = Rectangle{0, 0, (float)screenWidth, uiHeight};
}

void InitGame(Game &game, int screenWidth, int screenHeight) {
  game.mode = GameMode::Title;
  UpdateLayout(game, screenWidth, screenHeight);
  game.animTime = 0.12f;
  game.shake = 0.0f;
  ResetInput(game.input);
  ResetGame(game);
}

void UpdateGame(Game &game, float dt) {
  int width = GetScreenWidth();
  int height = GetScreenHeight();
  if (width != game.screenWidth || height != game.screenHeight) {
    UpdateLayout(game, width, height);
  }

  InputAction action = ReadInput(game.input, dt);
  UpdateActors(game, dt);

  for (auto &line : game.log) line.ttl -= dt;
  game.log.erase(std::remove_if(game.log.begin(), game.log.end(),
                                [](const LogLine &line) {
                                  return line.ttl <= 0.0f;
                                }),
                 game.log.end());

  if (game.shake > 0.0f) game.shake = std::max(0.0f, game.shake - dt);
  UpdateVisibility(game);

  if (game.mode == GameMode::Title) {
    if (action.confirm) {
      game.mode = GameMode::Playing;
      AddLog(game, "Press H to use a potion.");
    }
    return;
  }

  if (game.mode == GameMode::GameOver) {
    if (action.restart || action.confirm) {
      ResetGame(game);
      game.mode = GameMode::Playing;
    }
    return;
  }

  if (game.player.actor.moveT < 1.0f) return;

  bool acted = false;
  if (action.usePotion) {
    if (game.player.potions > 0 && game.player.hp < game.player.maxHp) {
      game.player.potions--;
      int heal = GetRandomValue(5, 9);
      game.player.hp = std::min(game.player.maxHp, game.player.hp + heal);
      AddLog(game, "You drink a potion.");
      acted = true;
    } else {
      AddLog(game, "No potions to use.");
    }
  } else if (action.wait) {
    AddLog(game, "You hold position.");
    acted = true;
  } else if (action.dx != 0 || action.dy != 0) {
    acted = HandleMove(game, action.dx, action.dy);
  }

  if (!acted) return;

  game.turn++;
  if (game.player.actor.cell == game.dungeon.exit) {
    game.floor++;
    AddLog(game, "You descend deeper...");
    BuildFloor(game, GetRandomValue(1, 999999));
    return;
  }

  EnemyTurn(game);
  if (game.player.hp <= 0) {
    game.mode = GameMode::GameOver;
    AddLog(game, "You fall in the dark.");
  }
}
