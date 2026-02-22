#pragma once

struct InputAction {
  int dx;
  int dy;
  bool wait;
  bool usePotion;
  bool restart;
  bool confirm;
};

struct InputState {
  int gamepad;
  float axisTimer;
  int axisX;
  int axisY;
};

void ResetInput(InputState &state);
InputAction ReadInput(InputState &state, float dt);
