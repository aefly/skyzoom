#include "FOVController.h"

#include "Config.h"
#include "Input.h"

// Skyrim has no re-readable ini Setting for first-person FOV, so this writes
// RE::PlayerCamera::worldFOV directly. The first-person viewmodel (arms +
// weapon) is rendered separately using its own firstPersonFOV member, so
// both are driven through the same transition to zoom as one picture.
namespace FOVController {
namespace {
bool g_active = false;
bool g_hotkeyWasDown = false;
float g_baseFOVDeg = 90.0f;
float g_fromFOVDeg = 90.0f;
float g_toFOVDeg = 90.0f;
float g_baseFirstPersonFOVDeg = 90.0f;
float g_fromFirstPersonFOVDeg = 90.0f;
float g_toFirstPersonFOVDeg = 90.0f;
float g_transitionT = 1.0f;

// Smoothstep: zero velocity at both ends, so the transition never jolts.
float SmoothStep(float a_t) noexcept {
  const float t = std::clamp(a_t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
} // namespace

void Update() {
  const auto now = std::chrono::steady_clock::now();
  static auto lastTime = now;
  float dt = std::chrono::duration<float>(now - lastTime).count();
  lastTime = now;
  dt = std::clamp(dt, 0.0f, 0.1f); // guard against spikes (loading, alt-tab)

  auto *playerCamera = RE::PlayerCamera::GetSingleton();
  const bool firstPerson = playerCamera && playerCamera->currentState &&
                           playerCamera->IsInFirstPerson();
  const bool hotkeyDown = Input::IsHotkeyDown();

  if (!playerCamera || !firstPerson) {
    // Reset here, not just on the press/release edge below, so leaving
    // first person mid-zoom doesn't leave worldFOV stuck at an in-between
    // value the next time first person is entered.
    if (g_active && playerCamera) {
      playerCamera->worldFOV = g_baseFOVDeg;
      playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
    }
    g_active = false;
    g_hotkeyWasDown = false;
    return;
  }

  if (hotkeyDown != g_hotkeyWasDown) {
    // Start a new transition from wherever the current one left off, so
    // quick taps mid-transition don't jump.
    if (!g_active) {
      g_baseFOVDeg = playerCamera->worldFOV;
      g_baseFirstPersonFOVDeg = playerCamera->firstPersonFOV;
    }
    const float t = SmoothStep(g_transitionT);
    g_fromFOVDeg = g_active ? (g_fromFOVDeg + (g_toFOVDeg - g_fromFOVDeg) * t)
                            : g_baseFOVDeg;
    g_fromFirstPersonFOVDeg =
        g_active ? (g_fromFirstPersonFOVDeg +
                    (g_toFirstPersonFOVDeg - g_fromFirstPersonFOVDeg) * t)
                 : g_baseFirstPersonFOVDeg;
    g_toFOVDeg = hotkeyDown ? Config::ZoomFOV : g_baseFOVDeg;
    g_toFirstPersonFOVDeg =
        hotkeyDown ? Config::ZoomFOV : g_baseFirstPersonFOVDeg;
    g_transitionT = 0.0f;
    g_active = true;
    g_hotkeyWasDown = hotkeyDown;
  }

  if (!g_active) {
    return;
  }

  const float duration = std::clamp(3.0f / Config::SmoothSpeed, 0.05f, 5.0f);
  g_transitionT = std::clamp(g_transitionT + dt / duration, 0.0f, 1.0f);
  const float t = SmoothStep(g_transitionT);

  playerCamera->worldFOV = g_fromFOVDeg + (g_toFOVDeg - g_fromFOVDeg) * t;
  playerCamera->firstPersonFOV =
      g_fromFirstPersonFOVDeg +
      (g_toFirstPersonFOVDeg - g_fromFirstPersonFOVDeg) * t;

  if (!hotkeyDown && g_transitionT >= 1.0f) {
    // Snap to the sampled base so nothing drifts if this runs again later.
    playerCamera->worldFOV = g_baseFOVDeg;
    playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
    g_active = false;
  }
}
} // namespace FOVController
