#pragma once

#include "../Header/Renderer2D.h"

struct AppState
{
    bool isOn = false;
    bool lockedByFullBowl = false;
    float ventOpenness = 0.0f; // 0 closed, 1 open
    float ventAnimSpeed = 1.5f; // openness units per second
    bool prevMouseDown = false;
    float desiredTemp = 24.0f;
    float currentTemp = 30.0f;
    float tempDriftSpeed = 0.8f; // degrees per second
    float tempChangeStep = 1.0f;
    bool prevUpPressed = false;
    bool prevDownPressed = false;
};

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp);
void updateVent(AppState& state, float deltaTime);
void handleTemperatureInput(AppState& state, bool upPressed, bool downPressed);
void updateTemperature(AppState& state, float deltaTime);
