#pragma once

#include "../Header/Renderer2D.h"

struct AppState
{
    bool isOn = false;
    bool lockedByFullBowl = false;
    float ventOpenness = 0.0f; // 0 closed, 1 open
    float ventAnimSpeed = 1.5f; // openness units per second
    bool prevMouseDown = false;
};

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp);
void updateVent(AppState& state, float deltaTime);
