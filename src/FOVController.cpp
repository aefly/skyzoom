#include "FOVController.h"

#include "Config.h"
#include "Input.h"

#include <cmath>

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
float g_baseSensitivity = 0.0f;

// Smoothstep: zero velocity at both ends, so the transition never jolts.
float SmoothStep(float a_t) noexcept {
  const float t = std::clamp(a_t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// Reads/writes "fMouseHeadingSensitivity:Controls" (SkyrimPrefs.ini,
// [Controls]) - Skyrim's ini/pref Setting collections key settings as
// "Name:Section" internally (matching the ini's [Section] header), not the
// bare key alone; see e.g. CommonLibSSE-NG's own
// SKSE/Translation.cpp:GetSetting("sLanguage:General"). Scaling this
// alongside the FOV keeps mouse-look feeling consistent while zoomed:
// without it, the same physical mouse movement sweeps a much larger angle
// on screen once the FOV (and thus degrees-per-pixel) shrinks, making aim
// twitchy at low zoom FOVs.
RE::Setting *GetMouseSensitivitySetting() noexcept {
  static RE::Setting *cached = nullptr;
  if (cached) {
    return cached;
  }

  constexpr auto kSettingName = "fMouseHeadingSensitivity:Controls";

  if (auto *prefs = RE::INIPrefSettingCollection::GetSingleton()) {
    if (auto *setting = prefs->GetSetting(kSettingName)) {
      cached = setting;
    }
  }

  if (!cached) {
    if (auto *ini = RE::INISettingCollection::GetSingleton()) {
      if (auto *setting = ini->GetSetting(kSettingName)) {
        cached = setting;
      }
    }
  }

  if (cached) {
    SKSE::log::info(
        "SkyZoom: mouse sensitivity setting found ({:X}), current value={:.4f}",
        reinterpret_cast<std::uintptr_t>(cached), cached->GetFloat());
  }

  return cached;
}
} // namespace

void Update() {
  const auto now = std::chrono::steady_clock::now();
  static auto lastTime = now;
  float dt = std::chrono::duration<float>(now - lastTime).count();
  lastTime = now;
  dt = std::clamp(dt, 0.0f, 0.1f); // guard against spikes (loading, alt-tab)

  auto *playerCamera = RE::PlayerCamera::GetSingleton();
  const bool inFirstPerson = playerCamera && playerCamera->currentState &&
                             playerCamera->IsInFirstPerson();
  const bool inThirdPerson = playerCamera && playerCamera->currentState &&
                             playerCamera->IsInThirdPerson();
  const bool inZoomableView =
      (Config::ActiveViewMode != Config::kThirdPersonOnly && inFirstPerson) ||
      (Config::ActiveViewMode != Config::kFirstPersonOnly && inThirdPerson);
  const bool hotkeyDown = Input::IsHotkeyDown();
  auto *sensSetting = GetMouseSensitivitySetting();

  if (!playerCamera || !inZoomableView) {
    // Reset here, not just on the press/release edge below, so leaving
    // first/third person mid-zoom doesn't leave worldFOV stuck at an
    // in-between value the next time one of those is entered. Other camera
    // states (horse, dragon, vanity, kill moves, ...) are left alone.
    if (g_active && playerCamera) {
      playerCamera->worldFOV = g_baseFOVDeg;
      playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
      if (sensSetting) {
        sensSetting->SetFloat(g_baseSensitivity);
      }
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
      if (sensSetting) {
        g_baseSensitivity = sensSetting->GetFloat();
      }
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

  // Scales sensitivity by the FOV ratio (raised to Config::SensitivityExponent
  // - see its declaration for why), so it rides the same eased transition as
  // the FOV itself with no separate easing state needed. Gated on
  // Config::ScaleMouseSensitivity only here (not on the sampling/restore
  // above/below) so toggling it off mid-zoom still restores whatever it had
  // last scaled to, instead of leaving it stuck.
  if (sensSetting && Config::ScaleMouseSensitivity && g_baseFOVDeg > 0.0f) {
    const float fovRatio =
        std::clamp(playerCamera->worldFOV / g_baseFOVDeg, 0.0f, 1.0f);
    const float sensScale = std::pow(fovRatio, Config::SensitivityExponent);
    sensSetting->SetFloat(g_baseSensitivity * sensScale);
  }

  if (!hotkeyDown && g_transitionT >= 1.0f) {
    // Snap to the sampled base so nothing drifts if this runs again later.
    playerCamera->worldFOV = g_baseFOVDeg;
    playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
    if (sensSetting) {
      sensSetting->SetFloat(g_baseSensitivity);
    }
    g_active = false;
  }
}
} // namespace FOVController
