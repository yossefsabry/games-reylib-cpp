#pragma once

#include "game.h"

#include <string>

void ResetGame(Game &game);
void BuildFloor(Game &game, int seed);
void UpdateActors(Game &game, float dt);
void UpdateVisibility(Game &game);
void AddLog(Game &game, const std::string &text, float ttl = 7.0f);
bool HandleMove(Game &game, int dx, int dy);
void EnemyTurn(Game &game);
