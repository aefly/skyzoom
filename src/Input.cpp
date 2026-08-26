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

  if ((GetAsyncKeyState(static_cast<std::int32_t>(Config::Hotkey)) & 0x8000) !=
      0) {
    return true;
  }

  return IsGamepadButtonDown(Config::GamepadButton);
}
} // namespace Input
