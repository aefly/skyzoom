#include "Input.h"

#include "Config.h"
#include "FOVController.h"

#include "REX/W32/BASE.h"
#include "REX/W32/XINPUT.h"

// GetKeyState only reflects state via the message queue, which nothing pumps
// on our render-thread Present hook - import GetAsyncKeyState instead.
REX_W32_IMPORT(std::int16_t, GetAsyncKeyState, std::int32_t);

// GetAsyncKeyState/XInputGetState both read global device state regardless
// of which window has focus - without this check, the hotkey would still
// trigger the zoom while alt-tabbed away from the game.
REX_W32_IMPORT(REX::W32::HWND, GetForegroundWindow);

extern "C" std::uint32_t XInputGetState(std::uint32_t a_userIndex,
                                        REX::W32::XINPUT_STATE *a_state);

namespace Input {
namespace {
std::atomic<std::int32_t> g_scrollSteps{0};
std::atomic<bool> g_scrollZoomActive{false};

// Mouse wheel notches arrive as RE::ButtonEvent on the kMouse device, not as
// a pollable state - idCode 8 is wheel-up, 9 is wheel-down (see
// SKSE::InputMap::kMacro_MouseWheelOffset, which starts right after the 8
// raw mouse button idCodes).
class WheelSink final : public RE::BSTEventSink<RE::InputEvent *> {
public:
  static WheelSink *GetSingleton() {
    static WheelSink singleton;
    return &singleton;
  }

  RE::BSEventNotifyControl
  ProcessEvent(RE::InputEvent *const *a_event,
               RE::BSTEventSource<RE::InputEvent *> *) override {
    if (!a_event) {
      return RE::BSEventNotifyControl::kContinue;
    }

    const bool claimWheel = g_scrollZoomActive.load(std::memory_order_relaxed);

    for (auto *event = *a_event; event; event = event->next) {
      auto *button = event->AsButtonEvent();
      if (!button || event->GetDevice() != RE::INPUT_DEVICE::kMouse ||
          !button->IsDown()) {
        continue;
      }

      const auto idCode = button->GetIDCode();
      if (idCode == 8) {
        g_scrollSteps.fetch_add(1, std::memory_order_relaxed);
      } else if (idCode == 9) {
        g_scrollSteps.fetch_sub(1, std::memory_order_relaxed);
      } else {
        continue;
      }

      if (claimWheel) {
        // Neuters the notch so an open menu's list (not gated by
        // ControlMap's UEFlags) doesn't also scroll from it. Relies on
        // PrependEventSink below to run before MenuControls sees it.
        button->GetRuntimeData().value = 0.0f;
      }
    }

    return RE::BSEventNotifyControl::kContinue;
  }
};

std::int16_t GetAsyncKeyState(std::int32_t a_key) noexcept {
  return ::W32_IMPL_GetAsyncKeyState(a_key);
}

bool IsTriggerPastThreshold(std::uint8_t a_value) noexcept {
  return a_value > REX::W32::XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}

bool IsWeaponSheathed() noexcept {
  auto *player = RE::PlayerCharacter::GetSingleton();
  auto *actorState = player ? player->AsActorState() : nullptr;
  return actorState && !actorState->IsWeaponDrawn();
}

// Analog-trigger ButtonEvent idCodes aren't in SKSE::InputMap's unified
// keymap range (see Config::kSyntheticLeftTrigger) - the game itself uses
// these two fixed values regardless of runtime (RE::BSWin32GamepadDevice::
// Keys::kLeftTrigger/kRightTrigger).
constexpr std::uint32_t kLeftTriggerIDCode = 9;
constexpr std::uint32_t kRightTriggerIDCode = 10;

// Neuters LT/RT's own ButtonEvent while it's repurposed as the zoom hotkey
// and the weapon is sheathed, so vanilla's left/right attack-block doesn't
// also fire from the same press - drawing a weapon stops the neutering
// immediately, restoring normal trigger function. Mirrors WheelSink's
// value-zeroing approach above; SkyZoom's own hotkey read
// (IsGamepadButtonDown) polls XInputGetState directly and is unaffected.
// Skipped entirely while a menu is open (FOVController::IsMenuOpen) - the
// weapon is sheathed nearly any time a menu is up, and several vanilla
// menus (Inventory/Barter/Container) use the same trigger to show an item
// comparison tooltip, which this would otherwise silently swallow.
class TriggerSink final : public RE::BSTEventSink<RE::InputEvent *> {
public:
  static TriggerSink *GetSingleton() {
    static TriggerSink singleton;
    return &singleton;
  }

  RE::BSEventNotifyControl
  ProcessEvent(RE::InputEvent *const *a_event,
               RE::BSTEventSource<RE::InputEvent *> *) override {
    if (!a_event || !Config::DisableTriggerWhenSheathed.load()) {
      return RE::BSEventNotifyControl::kContinue;
    }

    const auto gamepadButton = Config::GamepadButton.load();
    const bool suppressLeft =
        (gamepadButton & Config::kSyntheticLeftTrigger) != 0;
    const bool suppressRight =
        (gamepadButton & Config::kSyntheticRightTrigger) != 0;
    if (!suppressLeft && !suppressRight) {
      return RE::BSEventNotifyControl::kContinue;
    }

    if (FOVController::IsMenuOpen()) {
      return RE::BSEventNotifyControl::kContinue;
    }

    bool checkedSheathed = false;
    bool sheathed = false;

    for (auto *event = *a_event; event; event = event->next) {
      auto *button = event->AsButtonEvent();
      if (!button || event->GetDevice() != RE::INPUT_DEVICE::kGamepad) {
        continue;
      }

      const auto idCode = button->GetIDCode();
      const bool isTargetTrigger =
          (suppressLeft && idCode == kLeftTriggerIDCode) ||
          (suppressRight && idCode == kRightTriggerIDCode);
      if (!isTargetTrigger) {
        continue;
      }

      // Cached across the loop, not hoisted above it - avoids querying the
      // player singleton at all when nothing in this event batch matches.
      if (!checkedSheathed) {
        sheathed = IsWeaponSheathed();
        checkedSheathed = true;
      }
      if (sheathed) {
        button->GetRuntimeData().value = 0.0f;
      }
    }

    return RE::BSEventNotifyControl::kContinue;
  }
};

bool IsGamepadButtonDown(std::uint32_t a_buttonMask) noexcept {
  if (a_buttonMask == 0) {
    return false;
  }

  // LT/RT aren't real button bits (see Config::kSyntheticLeftTrigger) -
  // strip them before the bitmask check and test the trigger bytes instead.
  const auto realButtonMask = a_buttonMask & ~(Config::kSyntheticLeftTrigger |
                                               Config::kSyntheticRightTrigger);

  for (std::uint32_t user = 0; user < 4; ++user) {
    REX::W32::XINPUT_STATE state{};
    if (XInputGetState(user, &state) != 0) {
      continue;
    }

    if ((a_buttonMask & Config::kSyntheticLeftTrigger) != 0 &&
        IsTriggerPastThreshold(state.gamepad.leftTrigger)) {
      return true;
    }
    if ((a_buttonMask & Config::kSyntheticRightTrigger) != 0 &&
        IsTriggerPastThreshold(state.gamepad.rightTrigger)) {
      return true;
    }

    const auto downBits =
        static_cast<std::uint32_t>(state.gamepad.buttons) & realButtonMask;

    if (downBits != 0) {
      return true;
    }
  }

  return false;
}

} // namespace

bool IsGameWindowFocused() noexcept {
  auto *renderer = RE::BSGraphics::Renderer::GetSingleton();
  if (!renderer) {
    return true; // renderer not up yet; nothing to compare against
  }
  const auto gameHwnd = renderer->GetRuntimeData().renderWindows[0].hWnd;
  return gameHwnd != nullptr && ::W32_IMPL_GetForegroundWindow() == gameHwnd;
}

bool IsHotkeyDown() noexcept {
  if (!IsGameWindowFocused()) {
    return false;
  }

  if ((GetAsyncKeyState(static_cast<std::int32_t>(Config::Hotkey.load())) &
       0x8000) != 0) {
    return true;
  }

  return IsGamepadButtonDown(Config::GamepadButton.load());
}

bool IsGamepadZoomBoostDown() noexcept {
  const auto boostMask = Config::LiveZoomBoostButton.load();
  if (boostMask == 0) {
    return false;
  }

  // Excludes the boost button when it's also bound as the zoom hotkey
  // itself (a valid MCM remap) - otherwise holding the hotkey would also
  // continuously ramp the FOV, with no way to just hold at ZoomFOV.
  if ((Config::GamepadButton.load() & boostMask) != 0) {
    return false;
  }

  for (std::uint32_t user = 0; user < 4; ++user) {
    REX::W32::XINPUT_STATE state{};
    if (XInputGetState(user, &state) != 0) {
      continue;
    }

    if (boostMask == Config::kSyntheticLeftTrigger) {
      if (IsTriggerPastThreshold(state.gamepad.leftTrigger)) {
        return true;
      }
      continue;
    }
    if (boostMask == Config::kSyntheticRightTrigger) {
      if (IsTriggerPastThreshold(state.gamepad.rightTrigger)) {
        return true;
      }
      continue;
    }

    const auto buttons = static_cast<std::uint32_t>(state.gamepad.buttons);
    if ((buttons & boostMask) != 0) {
      return true;
    }
  }

  return false;
}

void InstallWheelSink() {
  if (auto *manager = RE::BSInputDeviceManager::GetSingleton()) {
    // Prepended, not appended - WheelSink::ProcessEvent must run before
    // MenuControls/PlayerControls (already-registered sinks by this point)
    // so it can neuter a claimed notch before they react to it.
    manager->PrependEventSink(WheelSink::GetSingleton());
  }
}

void InstallTriggerSink() {
  if (auto *manager = RE::BSInputDeviceManager::GetSingleton()) {
    // Prepended for the same reason as WheelSink above - must run before
    // PlayerControls sees the trigger's ButtonEvent.
    manager->PrependEventSink(TriggerSink::GetSingleton());
  }
}

std::int32_t ConsumeScrollSteps() noexcept {
  return g_scrollSteps.exchange(0, std::memory_order_relaxed);
}

void SetScrollZoomActive(bool a_active) noexcept {
  g_scrollZoomActive.store(a_active, std::memory_order_relaxed);
}
} // namespace Input
