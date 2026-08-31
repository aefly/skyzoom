#include "Input.h"

#include "Config.h"

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

bool IsGameWindowFocused() noexcept {
  auto *renderer = RE::BSGraphics::Renderer::GetSingleton();
  if (!renderer) {
    return true; // renderer not up yet; nothing to compare against
  }
  const auto gameHwnd = renderer->GetRuntimeData().renderWindows[0].hWnd;
  return gameHwnd != nullptr && ::W32_IMPL_GetForegroundWindow() == gameHwnd;
}

bool IsGamepadButtonDown(std::uint32_t a_buttonMask) noexcept {
  if (a_buttonMask == 0) {
    return false;
  }

  for (std::uint32_t user = 0; user < 4; ++user) {
    REX::W32::XINPUT_STATE state{};
    if (XInputGetState(user, &state) != 0) {
      continue;
    }

    auto downBits =
        static_cast<std::uint32_t>(state.gamepad.buttons) & a_buttonMask;
    constexpr auto kLeftThumb =
        static_cast<std::uint32_t>(REX::W32::XINPUT_GAMEPAD_LEFT_THUMB);

    if ((downBits & kLeftThumb) != 0) {
      // Left Stick is also the default Sprint button, which only engages
      // with the stick pushed - so a centered-stick click still counts as
      // the zoom hotkey, but a pushed one doesn't (no "is sprinting" flag
      // exists to check directly).
      const auto lx = static_cast<float>(state.gamepad.thumbLX);
      const auto ly = static_cast<float>(state.gamepad.thumbLY);
      constexpr float kDeadzone = REX::W32::XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
      if (lx * lx + ly * ly > kDeadzone * kDeadzone) {
        downBits &= ~kLeftThumb;
      }
    }

    if (downBits != 0) {
      return true;
    }
  }

  return false;
}

} // namespace

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

// LB/RB are the gamepad equivalent of scroll-to-adjust-zoom (D-Pad was the
// first choice but conflicts with Skyrim's own favorites quick-swap).
std::int32_t GetGamepadScrollDirection() noexcept {
  // Excludes whichever button is the zoom hotkey itself (a valid MCM
  // remap) - otherwise holding the hotkey would also continuously ramp
  // the FOV, with no way to just hold at ZoomFOV.
  const auto hotkeyMask = Config::GamepadButton.load();
  const bool leftIsHotkey =
      (hotkeyMask &
       static_cast<std::uint32_t>(REX::W32::XINPUT_GAMEPAD_LEFT_SHOULDER)) != 0;
  const bool rightIsHotkey =
      (hotkeyMask & static_cast<std::uint32_t>(
                        REX::W32::XINPUT_GAMEPAD_RIGHT_SHOULDER)) != 0;

  for (std::uint32_t user = 0; user < 4; ++user) {
    REX::W32::XINPUT_STATE state{};
    if (XInputGetState(user, &state) != 0) {
      continue;
    }

    const auto buttons = static_cast<std::uint32_t>(state.gamepad.buttons);
    const bool leftDown =
        !leftIsHotkey &&
        (buttons & static_cast<std::uint32_t>(
                       REX::W32::XINPUT_GAMEPAD_LEFT_SHOULDER)) != 0;
    const bool rightDown =
        !rightIsHotkey &&
        (buttons & static_cast<std::uint32_t>(
                       REX::W32::XINPUT_GAMEPAD_RIGHT_SHOULDER)) != 0;

    if (rightDown && !leftDown) {
      return 1; // zoom in
    }
    if (leftDown && !rightDown) {
      return -1; // zoom out
    }
  }

  return 0;
}

void InstallWheelSink() {
  if (auto *manager = RE::BSInputDeviceManager::GetSingleton()) {
    // Prepended, not appended - WheelSink::ProcessEvent must run before
    // MenuControls/PlayerControls (already-registered sinks by this point)
    // so it can neuter a claimed notch before they react to it.
    manager->PrependEventSink(WheelSink::GetSingleton());
  }
}

std::int32_t ConsumeScrollSteps() noexcept {
  return g_scrollSteps.exchange(0, std::memory_order_relaxed);
}

void SetScrollZoomActive(bool a_active) noexcept {
  g_scrollZoomActive.store(a_active, std::memory_order_relaxed);
}
} // namespace Input
