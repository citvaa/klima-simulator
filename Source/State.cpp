#include "../Header/State.h"

#include <algorithm>
#include <cmath>

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp)
{
    if (mouseDown && !state.prevMouseDown)
    {
        float dx = static_cast<float>(mouseX) - lamp.x;
        float dy = static_cast<float>(mouseY) - lamp.y;
        float distSq = dx * dx + dy * dy;
        if (distSq <= lamp.radius * lamp.radius && !state.lockedByFullBowl)
        {
            state.isOn = !state.isOn;
        }
    }

    state.prevMouseDown = mouseDown;
}

void updateVent(AppState& state, float deltaTime)
{
    float targetOpenness = state.isOn && !state.lockedByFullBowl ? 1.0f : 0.0f;
    if (state.ventOpenness < targetOpenness)
    {
        state.ventOpenness = std::min(targetOpenness, state.ventOpenness + state.ventAnimSpeed * deltaTime);
    }
    else if (state.ventOpenness > targetOpenness)
    {
        state.ventOpenness = std::max(targetOpenness, state.ventOpenness - state.ventAnimSpeed * deltaTime);
    }
}

void handleTemperatureInput(AppState& state, bool upPressed, bool downPressed)
{
    bool upEdge = upPressed && !state.prevUpPressed;
    bool downEdge = downPressed && !state.prevDownPressed;

    if (upEdge)
    {
        state.desiredTemp += state.tempChangeStep;
    }
    if (downEdge)
    {
        state.desiredTemp -= state.tempChangeStep;
    }

    state.desiredTemp = std::clamp(state.desiredTemp, -10.0f, 40.0f);

    state.prevUpPressed = upPressed;
    state.prevDownPressed = downPressed;
}

void updateTemperature(AppState& state, float deltaTime)
{
    if (!state.isOn || state.lockedByFullBowl) return;

    float diff = state.desiredTemp - state.currentTemp;
    float step = state.tempDriftSpeed * deltaTime;

    if (std::fabs(diff) <= step)
    {
        state.currentTemp = state.desiredTemp;
    }
    else
    {
        state.currentTemp += (diff > 0.0f ? step : -step);
    }
}
