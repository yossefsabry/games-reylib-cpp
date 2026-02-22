#include "input.h"

#include <raylib.h>

#include <cmath>

void ResetInput(InputState &state) {
  state.axisTimer = 0.0f;
  state.axisX = 0;
  state.axisY = 0;
}

static int AxisDir(float v, float threshold) {
  if (v > threshold) return 1;
  if (v < -threshold) return -1;
  return 0;
}

static void AddAxisMove(InputState &state, float dt, int &dx, int &dy) {
  if (!IsGamepadAvailable(0)) return;

  float ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
  float ay = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
  int axisX = AxisDir(ax, 0.55f);
  int axisY = AxisDir(ay, 0.55f);

  if (axisX == 0 && axisY == 0) {
    state.axisTimer = 0.0f;
    state.axisX = 0;
    state.axisY = 0;
    return;
  }

  if (axisX != state.axisX || axisY != state.axisY) {
    state.axisTimer = 0.0f;
    state.axisX = axisX;
    state.axisY = axisY;
  }

  state.axisTimer -= dt;
  if (state.axisTimer > 0.0f) return;

  if (axisX != 0 && (std::abs(ax) >= std::abs(ay) || axisY == 0)) {
    dx = axisX;
    dy = 0;
  } else {
    dx = 0;
    dy = axisY;
  }

  state.axisTimer = 0.18f;
}

InputAction ReadInput(InputState &state, float dt) {
  InputAction action = {};
  action.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                   IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
  action.restart = IsKeyPressed(KEY_R) ||
                   IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
  action.usePotion = IsKeyPressed(KEY_H) ||
                     IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
  action.wait = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_PERIOD) ||
                IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);

  int dx = 0;
  int dy = 0;

  if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dx = -1;
  else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dx = 1;
  else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dy = -1;
  else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dy = 1;

  if (dx == 0 && dy == 0 && IsGamepadAvailable(0)) {
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) dx = -1;
    else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) dx = 1;
    else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) dy = -1;
    else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) dy = 1;
  }

  if (dx == 0 && dy == 0) {
    AddAxisMove(state, dt, dx, dy);
  }

  action.dx = dx;
  action.dy = dy;
  return action;
}
