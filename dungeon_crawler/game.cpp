#include "game_internal.h"

#include <algorithm>

void InitGame(Game &game, int screenWidth, int screenHeight) {
  game.mode = GameMode::Title;
  game.screenWidth = screenWidth;
  game.screenHeight = screenHeight;
  game.tileSize = 30;
  game.dungeonRect = Rectangle{0, 0, 960.0f, (float)screenHeight};
  game.uiRect = Rectangle{960.0f, 0, (float)screenWidth - 960.0f,
                          (float)screenHeight};
  game.animTime = 0.12f;
  game.shake = 0.0f;
  ResetInput(game.input);
  ResetGame(game);
}

void UpdateGame(Game &game, float dt) {
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
