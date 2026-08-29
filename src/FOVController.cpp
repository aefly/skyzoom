#include "FOVController.h"

#include "Config.h"
#include "Input.h"

#include <array>
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

// Same from/to/t transition as above, in 0..1 instead of degrees - for
// GetActiveZoomWeight() below, so a compat patch can blend rather than
// substitute (see FOVController.h).
float g_fromWeight = 0.0f;
float g_toWeight = 0.0f;

// Atomic: read cross-DLL by a compat patch's own hook (CompatAPI.cpp).
std::atomic<bool> g_exportedActive{false};
std::atomic<float> g_exportedWeight{0.0f};
std::atomic<float> g_exportedTargetFOV{90.0f};

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

  const bool hotkeyDown =
      !weaponBlocksZoom && !dialogueBlocksZoom && Input::IsHotkeyDown();
  auto *sensSetting = GetMouseSensitivitySetting();

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
    }
    g_active = false;
    g_hotkeyWasDown = false;
    g_exportedActive.store(false, std::memory_order_relaxed);
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
    g_toFOVDeg = hotkeyDown ? Config::ZoomFOV.load() : g_baseFOVDeg;
    g_toFirstPersonFOVDeg =
        hotkeyDown ? Config::ZoomFOV.load() : g_baseFirstPersonFOVDeg;
    g_fromWeight =
        g_active ? (g_fromWeight + (g_toWeight - g_fromWeight) * t) : 0.0f;
    g_toWeight = hotkeyDown ? 1.0f : 0.0f;
    g_transitionT = 0.0f;
    g_active = true;
    g_hotkeyWasDown = hotkeyDown;
  }

  if (!g_active) {
    g_exportedActive.store(false, std::memory_order_relaxed);
    return;
  }

  const float duration =
      std::clamp(3.0f / Config::SmoothSpeed.load(), 0.05f, 5.0f);
  g_transitionT = std::clamp(g_transitionT + dt / duration, 0.0f, 1.0f);
  const float t = SmoothStep(g_transitionT);

  playerCamera->worldFOV = g_fromFOVDeg + (g_toFOVDeg - g_fromFOVDeg) * t;
  playerCamera->firstPersonFOV =
      g_fromFirstPersonFOVDeg +
      (g_toFirstPersonFOVDeg - g_fromFirstPersonFOVDeg) * t;

  const float weight = g_fromWeight + (g_toWeight - g_fromWeight) * t;
  g_exportedWeight.store(weight, std::memory_order_relaxed);
  g_exportedTargetFOV.store(Config::ZoomFOV.load(), std::memory_order_relaxed);
  g_exportedActive.store(true, std::memory_order_relaxed);

  // Scales sensitivity by the FOV ratio (raised to Config::SensitivityExponent
  // - see its declaration for why), so it rides the same eased transition as
  // the FOV itself with no separate easing state needed. Gated on
  // Config::ScaleMouseSensitivity only here (not on the sampling/restore
  // above/below) so toggling it off mid-zoom still restores whatever it had
  // last scaled to, instead of leaving it stuck.
  if (sensSetting && Config::ScaleMouseSensitivity.load() &&
      g_baseFOVDeg > 0.0f) {
    const float fovRatio =
        std::clamp(playerCamera->worldFOV / g_baseFOVDeg, 0.0f, 1.0f);
    const float sensScale =
        std::pow(fovRatio, Config::SensitivityExponent.load());
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
    g_exportedActive.store(false, std::memory_order_relaxed);
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
