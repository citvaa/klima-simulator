#include "../Header/State.h"

#include <algorithm>

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
