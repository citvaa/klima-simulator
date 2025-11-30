#include "../Header/State.h"

#include <algorithm>
#include <cmath>

void handlePowerToggle(AppState& state, double mouseX, double mouseY, bool mouseDown, const CircleShape& lamp)
{
    // Toggle AC on lamp click; ignore if locked by full bowl.
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
    // Animate vent toward open/closed target.
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
    // Edge-detect arrow keys and clamp desired temp.
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

    if (state.desiredTemp < -10.0f) state.desiredTemp = -10.0f;
    if (state.desiredTemp > 40.0f) state.desiredTemp = 40.0f;

    state.prevUpPressed = upPressed;
    state.prevDownPressed = downPressed;
}

void updateTemperature(AppState& state, float deltaTime)
{
    // Drift measured temp toward desired while AC is active.
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

void updateWater(AppState& state, float deltaTime, bool spacePressed)
{
    // Fill bowl over time while AC runs; Space drains and unlocks.
    bool spaceEdge = spacePressed && !state.prevSpacePressed;
    if (spaceEdge)
    {
        state.waterLevel = 0.0f;
        state.lockedByFullBowl = false;
    }

    if (state.isOn && !state.lockedByFullBowl)
    {
        state.waterAccum += deltaTime;
        while (state.waterAccum >= 1.0f)
        {
            state.waterAccum -= 1.0f;
            state.waterLevel += state.waterFillPerSecond;
        }
    }

    if (state.waterLevel > 1.0f)
    {
        state.waterLevel = 1.0f;
    }

    if (state.waterLevel >= 1.0f)
    {
        state.isOn = false;
        state.lockedByFullBowl = true;
    }

    state.prevSpacePressed = spacePressed;
}
