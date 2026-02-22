#include "input.h"

#include <raylib.h>

#include <cmath>

void ResetInput(InputState &state) {
  state.gamepad = -1;
  state.axisTimer = 0.0f;
  state.axisX = 0;
  state.axisY = 0;
}

static int AxisDir(float v, float threshold) {
  if (v > threshold) return 1;
  if (v < -threshold) return -1;
  return 0;
}

static int FindGamepad() {
  for (int i = 0; i < 4; i++) {
    if (IsGamepadAvailable(i)) return i;
  }
  return -1;
}

static bool GamepadPressed(int index, int button) {
  if (index < 0) return false;
  return IsGamepadButtonPressed(index, button);
}

static void AddAxisMove(InputState &state, float dt, int gamepad, int &dx,
                        int &dy) {
  if (gamepad < 0) return;

  float ax = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
  float ay = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
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
  int activePad = FindGamepad();
  if (activePad != state.gamepad) {
    state.gamepad = activePad;
    state.axisTimer = 0.0f;
    state.axisX = 0;
    state.axisY = 0;
  }

  InputAction action = {};
  action.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                   GamepadPressed(state.gamepad,
                                  GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
  action.restart = IsKeyPressed(KEY_R) ||
                   GamepadPressed(state.gamepad,
                                  GAMEPAD_BUTTON_MIDDLE_RIGHT);
  action.usePotion = IsKeyPressed(KEY_H) ||
                     GamepadPressed(state.gamepad,
                                    GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
  action.wait = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_PERIOD) ||
                GamepadPressed(state.gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);

  int dx = 0;
  int dy = 0;

  if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dx = -1;
  else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dx = 1;
  else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dy = -1;
  else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dy = 1;

  if (dx == 0 && dy == 0) {
    if (GamepadPressed(state.gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) dx = -1;
    else if (GamepadPressed(state.gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
      dx = 1;
    } else if (GamepadPressed(state.gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
      dy = -1;
    } else if (GamepadPressed(state.gamepad,
                              GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
      dy = 1;
    }
  }

  if (dx == 0 && dy == 0) {
    AddAxisMove(state, dt, state.gamepad, dx, dy);
  }

  action.dx = dx;
  action.dy = dy;
  return action;
}
