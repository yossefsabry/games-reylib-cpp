#include "game_internal.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>
static int Sign(int v) {
  return (v > 0) - (v < 0);
}
static void StartMove(Actor &actor, GridPos next) {
  actor.prev = actor.cell;
  actor.cell = next;
  actor.moveT = 0.0f;
}
static void UpdateActor(Actor &actor, float dt, float animTime) {
  if (actor.moveT >= 1.0f) return;
  actor.moveT += dt / animTime;
  if (actor.moveT > 1.0f) actor.moveT = 1.0f;
}
static Enemy *EnemyAt(Game &game, GridPos cell) {
  for (auto &enemy : game.enemies) {
    if (enemy.hp > 0 && enemy.actor.cell == cell) return &enemy;
  }
  return nullptr;
}
static bool IsOccupied(const Game &game, GridPos cell) {
  if (game.player.actor.cell == cell) return true;
  for (const auto &enemy : game.enemies) {
    if (enemy.hp > 0 && enemy.actor.cell == cell) return true;
  }
  return false;
}
static Item *ItemAt(Game &game, GridPos cell) {
  for (auto &item : game.items) {
    if (!item.picked && item.cell == cell) return &item;
  }
  return nullptr;
}
void AddLog(Game &game, const std::string &text, float ttl) {
  game.log.push_back(LogLine{text, ttl});
  if (game.log.size() > 6) game.log.erase(game.log.begin());
}
void UpdateActors(Game &game, float dt) {
  UpdateActor(game.player.actor, dt, game.animTime);
  for (auto &enemy : game.enemies) UpdateActor(enemy.actor, dt, game.animTime);
}

void UpdateVisibility(Game &game) {
  int size = game.dungeon.width * game.dungeon.height;
  if ((int)game.visible.size() != size) game.visible.assign(size, 0);
  std::fill(game.visible.begin(), game.visible.end(), 0);
  const int radius = 6;
  GridPos p = game.player.actor.cell;
  for (int y = p.y - radius; y <= p.y + radius; y++) {
    for (int x = p.x - radius; x <= p.x + radius; x++) {
      if (!InBounds(game.dungeon, x, y)) continue;
      int dx = x - p.x;
      int dy = y - p.y;
      if (dx * dx + dy * dy > radius * radius) continue;
      int idx = TileIndex(game.dungeon, x, y);
      game.visible[idx] = 1;
      game.dungeon.seen[idx] = 1;
    }
  }
}
static GridPos FindFreeCell(const Game &game, const Room &room) {
  for (int i = 0; i < 20; i++) {
    GridPos cell = RandomFloorInRoom(game.dungeon, room);
    if (!IsOccupied(game, cell) && !(cell == game.dungeon.exit)) return cell;
  }
  return room.Center();
}
static void PopulateDungeon(Game &game) {
  game.enemies.clear();
  game.items.clear();
  for (size_t i = 1; i < game.dungeon.rooms.size(); i++) {
    const Room &room = game.dungeon.rooms[i];
    int enemyCount = GetRandomValue(1, 3);
    for (int e = 0; e < enemyCount; e++) {
      Enemy enemy;
      enemy.actor.cell = FindFreeCell(game, room);
      enemy.actor.prev = enemy.actor.cell;
      enemy.actor.moveT = 1.0f;
      enemy.type = GetRandomValue(0, 1);
      enemy.hp = enemy.type == 0 ? 5 : 7;
      game.enemies.push_back(enemy);
    }
    if (GetRandomValue(0, 100) < 70) {
      Item item;
      item.cell = FindFreeCell(game, room);
      item.type = GetRandomValue(0, 100) < 40 ? ItemType::Potion : ItemType::Gold;
      item.amount = item.type == ItemType::Potion ? 1 : GetRandomValue(5, 14);
      item.picked = false;
      game.items.push_back(item);
    }
  }
}
void BuildFloor(Game &game, int seed) {
  game.dungeon = GenerateDungeon(32, 24, seed);
  game.dungeon.exit = game.dungeon.rooms.back().Center();
  game.visible.assign(game.dungeon.width * game.dungeon.height, 0);
  game.player.actor.cell = game.dungeon.rooms.front().Center();
  game.player.actor.prev = game.player.actor.cell;
  game.player.actor.moveT = 1.0f;
  PopulateDungeon(game);
  UpdateVisibility(game);
}
void ResetGame(Game &game) {
  game.turn = 0;
  game.floor = 1;
  game.player.maxHp = 24;
  game.player.hp = game.player.maxHp;
  game.player.potions = 2;
  game.player.gold = 0;
  game.player.attack = 4;
  game.player.defense = 1;
  game.log.clear();
  AddLog(game, "You enter the crypt...");
  BuildFloor(game, GetRandomValue(1, 999999));
}
void EnemyTurn(Game &game) {
  GridPos playerCell = game.player.actor.cell;
  for (auto &enemy : game.enemies) {
    if (enemy.hp <= 0) continue;
    GridPos epos = enemy.actor.cell;
    int dx = playerCell.x - epos.x;
    int dy = playerCell.y - epos.y;
    int dist = std::abs(dx) + std::abs(dy);
    if (dist <= 1) {
      int damage = enemy.type == 0 ? 2 : 3;
      damage = std::max(1, damage - game.player.defense);
      game.player.hp -= damage;
      game.shake = 0.2f;
      AddLog(game, "An enemy strikes you for " + std::to_string(damage) + "!");
      continue;
    }
    if (dist > 7 && GetRandomValue(0, 100) < 50) continue;
    int stepX = Sign(dx);
    int stepY = Sign(dy);
    GridPos target = epos;
    if (std::abs(dx) >= std::abs(dy)) target.x += stepX;
    else target.y += stepY;
    if (target == playerCell) {
      int damage = enemy.type == 0 ? 2 : 3;
      damage = std::max(1, damage - game.player.defense);
      game.player.hp -= damage;
      game.shake = 0.2f;
      AddLog(game, "An enemy strikes you for " + std::to_string(damage) + "!");
      continue;
    }
    bool blocked = !IsWalkable(game.dungeon, target.x, target.y) ||
                   IsOccupied(game, target);
    if (blocked && stepX != 0 && stepY != 0) {
      GridPos alt = epos;
      alt.y += stepY;
      if (IsWalkable(game.dungeon, alt.x, alt.y) && !IsOccupied(game, alt)) {
        target = alt;
        blocked = false;
      }
    }

    if (!blocked) StartMove(enemy.actor, target);
  }
}
bool HandleMove(Game &game, int dx, int dy) {
  GridPos next{game.player.actor.cell.x + dx, game.player.actor.cell.y + dy};
  if (!IsWalkable(game.dungeon, next.x, next.y)) return false;
  if (Enemy *enemy = EnemyAt(game, next)) {
    int damage = game.player.attack + GetRandomValue(0, 2);
    enemy->hp -= damage;
    AddLog(game, "You hit for " + std::to_string(damage) + ".");
    if (enemy->hp <= 0) {
      AddLog(game, "Enemy defeated.");
      game.player.gold += GetRandomValue(2, 6);
    }
    return true;
  }
  StartMove(game.player.actor, next);
  if (Item *item = ItemAt(game, next)) {
    item->picked = true;
    if (item->type == ItemType::Gold) {
      game.player.gold += item->amount;
      AddLog(game, "Picked up " + std::to_string(item->amount) + " gold.");
    } else {
      game.player.potions += item->amount;
      AddLog(game, "Found a potion.");
    }
  }
  return true;
}
