#include "FOVController.h"

#include "Config.h"
#include "Input.h"

#include <array>
#include <cmath>

// Some RE/* headers pull in the real <Windows.h> (see Hooks.cpp), which
// defines min/max as object-like macros shadowing std::min/std::max.
#undef min
#undef max

// Skyrim has no re-readable ini Setting for first-person FOV, so this writes
// RE::PlayerCamera::worldFOV directly. The first-person viewmodel (arms +
// weapon) is rendered separately using its own firstPersonFOV member, so
// both are driven through the same transition to zoom as one picture.
namespace FOVController {
namespace {
bool g_active = false;
bool g_hotkeyWasDown = false;
// Raw physical press edge for Config::ToggleMode - g_hotkeyWasDown above
// tracks the post-toggle effective state instead.
bool g_rawHotkeyWasDown = false;
// Flips on each raw press while ToggleMode is on; reset to false when it's
// off, so switching modes mid-session can't leave a stale "on" state.
bool g_toggleActive = false;
float g_baseFOVDeg = 90.0f;
float g_fromFOVDeg = 90.0f;
float g_toFOVDeg = 90.0f;
float g_baseFirstPersonFOVDeg = 90.0f;
float g_fromFirstPersonFOVDeg = 90.0f;
float g_toFirstPersonFOVDeg = 90.0f;
float g_transitionT = 1.0f;
float g_baseSensitivity = 0.0f;
float g_baseGamepadSensitivity = 0.0f;

// Config::ZoomFOV as adjusted by the mouse wheel or
// Config::LiveZoomBoostButton this hold, within [MinZoomFOV, ZoomFOV] -
// kept separate from Config::ZoomFOV so scrolling never writes back to
// the ini/MCM value. g_toFOVDeg chases this via SmoothDamp (see Update())
// rather than jumping straight to it.
float g_scrollZoomFOV = 60.0f;
// SmoothDamp's persisted velocity state for the chase above.
float g_scrollFOVVelocity = 0.0f;
// Edge-detects the boost button's release, so letting go starts ramping
// g_scrollZoomFOV back to ZoomFOV (unlike the wheel, which persists).
bool g_gamepadZoomBoostWasDown = false;
// True from release until g_scrollZoomFOV finishes ramping back to
// ZoomFOV at the same rate as zooming in (an instant jump would let
// SmoothDamp close it much faster than the hold-in ramp).
bool g_gamepadBoostReleaseRamping = false;

// Drives GetActiveZoomWeight() below (see FOVController.h). Ramps 0->1 on
// engagement, stays at 1 through hold/release/scroll, only resets at full
// deactivation - a host mod's a_fov isn't fed back from our last output,
// so anything short of weight 1 never converges. Fades 1->0 during
// g_fadingOut instead of an instant cutoff, so a host mod's own FOV effects,
// suppressed all hold, ease back in instead of popping.
float g_weightRampT = 0.0f;
// True once the outer FOV transition has finished on release and only the
// exported weight is still fading out - see g_weightRampT.
bool g_fadingOut = false;

// Atomic: read cross-DLL by a compat patch's own hook (CompatAPI.cpp).
std::atomic<bool> g_exportedActive{false};
std::atomic<float> g_exportedWeight{0.0f};
std::atomic<float> g_exportedTargetFOV{90.0f};

// Tracks whether SuppressWheelControls() below is currently in effect.
bool g_wheelControlsSuppressed = false;

// Shared by both functions below - kPOVSwitch/kWheelZoom, the two vanilla
// control groups the mouse wheel drives on its own.
RE::UserEvents::USER_EVENT_FLAG WheelControlFlags() noexcept {
  return static_cast<RE::UserEvents::USER_EVENT_FLAG>(
      static_cast<std::uint32_t>(RE::UserEvents::USER_EVENT_FLAG::kPOVSwitch) |
      static_cast<std::uint32_t>(RE::UserEvents::USER_EVENT_FLAG::kWheelZoom));
}

// Vanilla binds the mouse wheel to POV switching and third-person camera
// zoom, which fight with the hotkey-held zoom (e.g. scrolling out while
// zoomed in first person flips to third person) - suppressed for the hold
// via the same ControlMap API the game uses to disable controls in menus.
void SuppressWheelControls() noexcept {
  if (g_wheelControlsSuppressed) {
    return;
  }
  auto *controlMap = RE::ControlMap::GetSingleton();
  if (!controlMap) {
    return;
  }
  controlMap->ToggleControls(WheelControlFlags(), false, true);
  g_wheelControlsSuppressed = true;
}

void RestoreWheelControls() noexcept {
  if (!g_wheelControlsSuppressed) {
    return;
  }
  // Re-enables only these two flags, not a full enabledControls snapshot
  // restore - that would also re-enable anything else disabled during the
  // hold (e.g. movement, if a menu opened mid-hold).
  if (auto *controlMap = RE::ControlMap::GetSingleton()) {
    controlMap->ToggleControls(WheelControlFlags(), true, true);
  }
  g_wheelControlsSuppressed = false;
}

// Smoothstep: zero velocity at both ends, so the transition never jolts.
float SmoothStep(float a_t) noexcept {
  const float t = std::clamp(a_t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// Critically-damped spring follow (Game Programming Gems 4's SmoothDamp) -
// unlike a from/to/duration transition it has no "restart", so a target
// that keeps moving (wheel notch mid-chase, gamepad ramp) never jolts.
// a_velocity is the follower's own persisted state, updated in place.
float SmoothDamp(float a_current, float a_target, float &a_velocity,
                 float a_smoothTime, float a_dt) noexcept {
  const float dt = std::max(a_dt, 1e-5f); // guard div-by-zero below
  const float smoothTime = std::max(a_smoothTime, 0.0001f);
  const float omega = 2.0f / smoothTime;
  const float x = omega * dt;
  const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
  const float change = a_current - a_target;
  const float temp = (a_velocity + omega * change) * dt;
  a_velocity = (a_velocity - omega * temp) * exp;
  float result = a_target + (change + temp) * exp;
  // Prevent overshoot past the target reversing direction.
  if ((a_target - a_current > 0.0f) == (result > a_target)) {
    result = a_target;
    a_velocity = (result - a_target) / dt;
  }
  return result;
}

// Ini/pref Setting collections key settings as "Name:Section" internally,
// not the bare key alone.
RE::Setting *FindSetting(const char *a_name) noexcept {
  if (auto *prefs = RE::INIPrefSettingCollection::GetSingleton()) {
    if (auto *setting = prefs->GetSetting(a_name)) {
      return setting;
    }
  }
  if (auto *ini = RE::INISettingCollection::GetSingleton()) {
    if (auto *setting = ini->GetSetting(a_name)) {
      return setting;
    }
  }
  return nullptr;
}

// Reads/writes "fMouseHeadingSensitivity:Controls" (SkyrimPrefs.ini,
// [Controls]). Scaling this alongside the FOV keeps mouse-look feeling
// consistent while zoomed: without it, the same physical mouse movement
// sweeps a much larger angle on screen once the FOV (and thus
// degrees-per-pixel) shrinks, making aim twitchy at low zoom FOVs.
RE::Setting *GetMouseSensitivitySetting() noexcept {
  static RE::Setting *cached = nullptr;
  if (!cached) {
    cached = FindSetting("fMouseHeadingSensitivity:Controls");
    if (cached) {
      SKSE::log::info("SkyZoom: mouse sensitivity setting found ({:X}), "
                      "current value={:.4f}",
                      reinterpret_cast<std::uintptr_t>(cached),
                      cached->GetFloat());
    }
  }
  return cached;
}

// Gamepad counterpart of the setting above - GamepadButton can itself
// trigger the zoom, so controller look needs the same compensation.
RE::Setting *GetGamepadSensitivitySetting() noexcept {
  static RE::Setting *cached = nullptr;
  if (!cached) {
    cached = FindSetting("fGamePadHeadingSensitivity:Controls");
    if (cached) {
      SKSE::log::info("SkyZoom: gamepad sensitivity setting found ({:X}), "
                      "current value={:.4f}",
                      reinterpret_cast<std::uintptr_t>(cached),
                      cached->GetFloat());
    }
  }
  return cached;
}

// Menus that take over input without tripping GameIsPaused() or
// ControlMap::IsMovementControlsEnabled() (e.g. Crafting does neither).
constexpr std::array<std::string_view, 19> kBlockingMenuNames = {
    RE::BookMenu::MENU_NAME,        RE::CraftingMenu::MENU_NAME,
    RE::ContainerMenu::MENU_NAME,   RE::BarterMenu::MENU_NAME,
    RE::TweenMenu::MENU_NAME,       RE::FavoritesMenu::MENU_NAME,
    RE::GiftMenu::MENU_NAME,        RE::LevelUpMenu::MENU_NAME,
    RE::LockpickingMenu::MENU_NAME, RE::RaceSexMenu::MENU_NAME,
    RE::SleepWaitMenu::MENU_NAME,   RE::TrainingMenu::MENU_NAME,
    RE::MapMenu::MENU_NAME,         RE::MagicMenu::MENU_NAME,
    RE::InventoryMenu::MENU_NAME,   RE::JournalMenu::MENU_NAME,
    RE::StatsMenu::MENU_NAME,       RE::Console::MENU_NAME,
    RE::MessageBoxMenu::MENU_NAME,
};

bool IsBlockingMenuOpen(RE::UI *a_ui) noexcept {
  for (const auto &name : kBlockingMenuNames) {
    if (a_ui->IsMenuOpen(name)) {
      return true;
    }
  }
  return false;
}
} // namespace

// Shared with Input.cpp's TriggerSink - it also needs to know when a menu
// has taken over input, so it doesn't suppress LT/RT's vanilla function
// (e.g. the Inventory/Barter/Container "hold to compare" prompt) just
// because the weapon happens to be sheathed while browsing a menu. Dialogue
// is deliberately excluded here too, matching Update()'s own menuOpen -
// TriggerSink doesn't need a dialogue carve-out, but there's no harm in it
// following the same definition of "blocking" as the zoom hotkey does.
bool IsMenuOpen() noexcept {
  auto *ui = RE::UI::GetSingleton();
  if (!ui || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
    return false;
  }
  auto *controlMap = RE::ControlMap::GetSingleton();
  const bool controlsDisabled =
      controlMap && !controlMap->IsMovementControlsEnabled();
  return controlsDisabled || ui->GameIsPaused() || IsBlockingMenuOpen(ui);
}

void Update() {
  const auto now = std::chrono::steady_clock::now();
  static auto lastTime = now;
  float dt = std::chrono::duration<float>(now - lastTime).count();
  lastTime = now;
  dt = std::clamp(dt, 0.0f, 0.1f); // guard against spikes (loading, alt-tab)

  // Drained every frame regardless of zoom state, so wheel scrolls made
  // while not zooming never carry over as a jump next time the hotkey is
  // pressed. The gamepad boost button uses
  // Input::IsGamepadZoomBoostDown() instead (continuous held state, not
  // discrete notches).
  const std::int32_t scrollSteps = Input::ConsumeScrollSteps();

  auto *playerCamera = RE::PlayerCamera::GetSingleton();
  const bool inFirstPerson = playerCamera && playerCamera->currentState &&
                             playerCamera->IsInFirstPerson();
  const bool inThirdPerson = playerCamera && playerCamera->currentState &&
                             playerCamera->IsInThirdPerson();
  const bool inZoomableView =
      (Config::ActiveViewMode.load() != Config::kThirdPersonOnly &&
       inFirstPerson) ||
      (Config::ActiveViewMode.load() != Config::kFirstPersonOnly &&
       inThirdPerson);

  // A blocking menu is up - three checks OR'd together, since no single one
  // covers every menu (see kBlockingMenuNames above). Dialogue trips these
  // too but is excluded and handled separately below.
  auto *ui = RE::UI::GetSingleton();
  const bool dialogueOpen = ui && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
  auto *controlMap = RE::ControlMap::GetSingleton();
  const bool controlsDisabled =
      controlMap && !controlMap->IsMovementControlsEnabled();
  const bool gamePaused = ui && ui->GameIsPaused();
  const bool namedMenuOpen = ui && IsBlockingMenuOpen(ui);
  const bool menuOpen =
      !dialogueOpen && (controlsDisabled || gamePaused || namedMenuOpen);

  // Own MCM toggle (some players want to zoom mid-conversation), and eases
  // out via hotkeyDown below instead of the hard reset, since unlike the
  // menus above it doesn't cover the 3D view.
  const bool dialogueZoomAllowed =
      dialogueOpen && Config::AllowZoomDuringDialogue.load();
  const bool dialogueBlocksZoom =
      dialogueOpen && !Config::AllowZoomDuringDialogue.load();

  // Recon-tool intent, except during an allowed conversation where weapon
  // state shouldn't matter.
  auto *player = RE::PlayerCharacter::GetSingleton();
  auto *actorState = player ? player->AsActorState() : nullptr;
  const bool weaponBlocksZoom = !dialogueZoomAllowed &&
                                Config::RequireWeaponSheathed.load() &&
                                actorState && actorState->IsWeaponDrawn();

  // Ungated by weapon/dialogue blocking, so a toggle press still registers
  // while blocked. Frozen entirely while a menu is up or unfocused, so a
  // same-button menu press or holding through an alt-tab can't misread
  // as a fresh edge.
  const bool rawHotkeyDown = Input::IsHotkeyDown();
  if (Config::ToggleMode.load()) {
    if (!menuOpen && Input::IsGameWindowFocused()) {
      if (rawHotkeyDown && !g_rawHotkeyWasDown) {
        g_toggleActive = !g_toggleActive;
      }
      g_rawHotkeyWasDown = rawHotkeyDown;
    }
  } else {
    g_toggleActive = false;
    g_rawHotkeyWasDown = rawHotkeyDown;
  }

  const bool effectiveHotkeyDown =
      Config::ToggleMode.load() ? g_toggleActive : rawHotkeyDown;
  const bool hotkeyDown =
      !weaponBlocksZoom && !dialogueBlocksZoom && effectiveHotkeyDown;
  auto *sensSetting = GetMouseSensitivitySetting();
  auto *gamepadSensSetting = GetGamepadSensitivitySetting();

  if (!playerCamera || !inZoomableView || menuOpen) {
    // Reset here, not just on the press/release edge below, so leaving
    // first/third person mid-zoom (or opening a menu) doesn't leave
    // worldFOV stuck at an in-between value the next time one of those is
    // entered. Other camera states (horse, dragon, vanity, kill moves, ...)
    // are left alone.
    if (g_active && playerCamera) {
      playerCamera->worldFOV = g_baseFOVDeg;
      playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
      if (sensSetting) {
        sensSetting->SetFloat(g_baseSensitivity);
      }
      if (gamepadSensSetting) {
        gamepadSensSetting->SetFloat(g_baseGamepadSensitivity);
      }
    }
    g_active = false;
    g_hotkeyWasDown = false;
    g_weightRampT = 0.0f;
    g_fadingOut = false;
    g_exportedActive.store(false, std::memory_order_relaxed);
    RestoreWheelControls();
    Input::SetScrollZoomActive(false);
    return;
  }

  const bool scrollZoomActive =
      hotkeyDown && Config::EnableScrollZoomAdjust.load();
  Input::SetScrollZoomActive(scrollZoomActive);

  if (hotkeyDown != g_hotkeyWasDown) {
    // Start a new transition from wherever the current one left off, so
    // quick taps mid-transition don't jump.
    if (!g_active) {
      g_baseFOVDeg = playerCamera->worldFOV;
      g_baseFirstPersonFOVDeg = playerCamera->firstPersonFOV;
      if (sensSetting) {
        g_baseSensitivity = sensSetting->GetFloat();
      }
      if (gamepadSensSetting) {
        g_baseGamepadSensitivity = gamepadSensSetting->GetFloat();
      }
    }
    const float t = SmoothStep(g_transitionT);
    g_fromFOVDeg = g_active ? (g_fromFOVDeg + (g_toFOVDeg - g_fromFOVDeg) * t)
                            : g_baseFOVDeg;
    g_fromFirstPersonFOVDeg =
        g_active ? (g_fromFirstPersonFOVDeg +
                    (g_toFirstPersonFOVDeg - g_fromFirstPersonFOVDeg) * t)
                 : g_baseFirstPersonFOVDeg;
    if (hotkeyDown) {
      g_scrollZoomFOV = Config::ZoomFOV.load();
      g_scrollFOVVelocity = 0.0f;
      g_gamepadZoomBoostWasDown = false;
      g_gamepadBoostReleaseRamping = false;
      SuppressWheelControls();
    } else {
      RestoreWheelControls();
    }
    g_toFOVDeg = hotkeyDown ? g_scrollZoomFOV : g_baseFOVDeg;
    g_toFirstPersonFOVDeg =
        hotkeyDown ? g_scrollZoomFOV : g_baseFirstPersonFOVDeg;
    g_transitionT = 0.0f;
    g_active = true;
    // A re-press during the post-release weight fade-out (g_fadingOut)
    // should resume ramping the weight back up, not keep fading it out.
    g_fadingOut = false;
    g_hotkeyWasDown = hotkeyDown;
  }

  if (!g_active) {
    g_exportedActive.store(false, std::memory_order_relaxed);
    return;
  }

  // Shared with the scroll-adjust SmoothDamp below when
  // Config::ScrollUsesSmoothSpeed opts into it instead of kScrollSmoothTime.
  const float duration =
      std::clamp(3.0f / Config::SmoothSpeed.load(), 0.05f, 5.0f);

  // g_scrollZoomFOV is the scroll-adjusted target within [MinZoomFOV,
  // ZoomFOV] - only releasing the hotkey fully cancels the zoom. The wheel
  // moves it in instant kScrollStepDeg jumps that persist; the gamepad
  // boost button ramps it toward MinZoomFOV while held and back at the
  // same rate on release. g_toFOVDeg chases whichever target via
  // SmoothDamp rather than the outer press/release transition, which is
  // sized for the whole zoom-in sweep, not small, fast-repeating notches.
  if (scrollZoomActive) {
    constexpr float kScrollStepDeg = 5.0f;
    constexpr float kGamepadScrollDegPerSec = 60.0f;
    constexpr float kScrollSmoothTime = 0.12f;
    const float maxFOV = Config::ZoomFOV.load();
    const float minFOV = std::min(Config::MinZoomFOV.load(), maxFOV);

    if (scrollSteps != 0) {
      g_scrollZoomFOV = std::clamp(
          g_scrollZoomFOV - static_cast<float>(scrollSteps) * kScrollStepDeg,
          minFOV, maxFOV);
    }

    const bool gamepadZoomBoostDown = Input::IsGamepadZoomBoostDown();
    if (gamepadZoomBoostDown) {
      g_scrollZoomFOV = std::clamp(
          g_scrollZoomFOV - kGamepadScrollDegPerSec * dt, minFOV, maxFOV);
      g_gamepadBoostReleaseRamping = false;
    } else if (g_gamepadZoomBoostWasDown) {
      g_gamepadBoostReleaseRamping = true; // just released
    }
    if (g_gamepadBoostReleaseRamping) {
      g_scrollZoomFOV = std::clamp(
          g_scrollZoomFOV + kGamepadScrollDegPerSec * dt, minFOV, maxFOV);
      if (g_scrollZoomFOV >= maxFOV) {
        g_gamepadBoostReleaseRamping = false;
      }
    }
    g_gamepadZoomBoostWasDown = gamepadZoomBoostDown;

    const float smoothTime =
        Config::ScrollUsesSmoothSpeed.load() ? duration : kScrollSmoothTime;
    g_toFOVDeg = SmoothDamp(g_toFOVDeg, g_scrollZoomFOV, g_scrollFOVVelocity,
                            smoothTime, dt);
    g_toFirstPersonFOVDeg = g_toFOVDeg; // kept equal while active
  }

  g_transitionT = std::clamp(g_transitionT + dt / duration, 0.0f, 1.0f);
  const float t = SmoothStep(g_transitionT);

  playerCamera->worldFOV = g_fromFOVDeg + (g_toFOVDeg - g_fromFOVDeg) * t;
  playerCamera->firstPersonFOV =
      g_fromFirstPersonFOVDeg +
      (g_toFirstPersonFOVDeg - g_fromFirstPersonFOVDeg) * t;

  // Advances at the same pace as the outer transition but never resets on
  // a press/release flip (see g_weightRampT's declaration) - only clamped
  // here, not reset, so re-pressing mid-release just keeps climbing toward
  // 1 instead of restarting the ramp. Reverses once g_fadingOut - see the
  // completion check below.
  g_weightRampT = std::clamp(
      g_weightRampT + (g_fadingOut ? -dt : dt) / duration, 0.0f, 1.0f);
  const float weight = SmoothStep(g_weightRampT);
  g_exportedWeight.store(weight, std::memory_order_relaxed);
  // The live eased FOV, not g_toFOVDeg - rides our own from/to/t curve
  // exactly (in either direction) instead of jumping to the endpoint.
  g_exportedTargetFOV.store(playerCamera->worldFOV, std::memory_order_relaxed);
  g_exportedActive.store(true, std::memory_order_relaxed);

  // Scales mouse and gamepad sensitivity by the FOV ratio (raised to
  // Config::SensitivityExponent - see its declaration for why), so it rides
  // the same eased transition as the FOV itself with no separate easing
  // state needed. Gated on Config::ScaleMouseSensitivity only here (not on
  // the sampling/restore above/below) so toggling it off mid-zoom still
  // restores whatever it had last scaled to, instead of leaving it stuck.
  if (Config::ScaleMouseSensitivity.load() && g_baseFOVDeg > 0.0f) {
    const float fovRatio =
        std::clamp(playerCamera->worldFOV / g_baseFOVDeg, 0.0f, 1.0f);
    const float sensScale =
        std::pow(fovRatio, Config::SensitivityExponent.load());
    if (sensSetting) {
      sensSetting->SetFloat(g_baseSensitivity * sensScale);
    }
    if (gamepadSensSetting) {
      gamepadSensSetting->SetFloat(g_baseGamepadSensitivity * sensScale);
    }
  }

  if (!hotkeyDown && g_transitionT >= 1.0f) {
    // Snap to the sampled base so nothing drifts if this runs again later.
    playerCamera->worldFOV = g_baseFOVDeg;
    playerCamera->firstPersonFOV = g_baseFirstPersonFOVDeg;
    if (sensSetting) {
      sensSetting->SetFloat(g_baseSensitivity);
    }
    if (gamepadSensSetting) {
      gamepadSensSetting->SetFloat(g_baseGamepadSensitivity);
    }
    // The FOV transition itself is done, but a compat patch's blend weight
    // (g_weightRampT, above) keeps fading out on its own for a bit longer
    // before we fully deactivate - see its declaration.
    g_fadingOut = true;
    if (g_weightRampT <= 0.0f) {
      g_active = false;
      g_fadingOut = false;
      g_exportedActive.store(false, std::memory_order_relaxed);
    }
  }
}

bool GetActiveZoomWeight(float &a_outWeight, float &a_outTargetFOV) noexcept {
  if (!g_exportedActive.load(std::memory_order_relaxed)) {
    return false;
  }
  a_outWeight = g_exportedWeight.load(std::memory_order_relaxed);
  a_outTargetFOV = g_exportedTargetFOV.load(std::memory_order_relaxed);
  return true;
}
} // namespace FOVController
