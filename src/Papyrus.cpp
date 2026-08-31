#include "Papyrus.h"

#include "Config.h"

#include "REX/W32/BASE.h"
#include "REX/W32/USER32.h"
#include "SKSE/InputMap.h"

#include <array>

// Some RE/* headers pull in the real <d3d11.h> (and therefore <windows.h>)
// after REX/W32/BASE.h has already been parsed, so its "no real Windows.h"
// guard doesn't catch it. That leaves these as object-like macros, shadowing
// the REX::W32:: qualified names below - undef them first.
#undef VK_LBUTTON
#undef VK_RBUTTON
#undef VK_MBUTTON
#undef VK_XBUTTON1
#undef VK_XBUTTON2

// MCM's AddKeyMapOption reports DirectX scan codes, not the Windows
// virtual-key codes Config::Hotkey and Input.cpp's GetAsyncKeyState use -
// every value crossing that boundary goes through this conversion.
REX_W32_IMPORT(std::uint32_t, MapVirtualKeyW, std::uint32_t, std::uint32_t);

namespace Papyrus {
namespace {
constexpr std::uint32_t kMapvkVkToVscEx = 4;
constexpr std::uint32_t kMapvkVscToVkEx = 3;

std::uint32_t VirtualKeyToScanCode(std::uint32_t a_vk) noexcept {
  return ::W32_IMPL_MapVirtualKeyW(a_vk, kMapvkVkToVscEx);
}

std::uint32_t ScanCodeToVirtualKey(std::uint32_t a_scanCode) noexcept {
  return ::W32_IMPL_MapVirtualKeyW(a_scanCode, kMapvkVscToVkEx);
}

// AddKeyMapOption also reports mouse buttons as SKSE::InputMap's offset codes
// 256 (Left) .. 260 (XButton2) - codes 261-265 (extra buttons/wheel) have no
// Windows VK equivalent GetAsyncKeyState can poll, so they map to 0.
constexpr std::uint32_t kMouseButtonMcmOffset =
    SKSE::InputMap::kMacro_MouseButtonOffset;
constexpr std::array<std::uint32_t, 5> kMouseButtonVKs = {
    REX::W32::VK_LBUTTON, REX::W32::VK_RBUTTON, REX::W32::VK_MBUTTON,
    REX::W32::VK_XBUTTON1, REX::W32::VK_XBUTTON2};

std::uint32_t McmCodeToVirtualKey(std::uint32_t a_mcmCode) noexcept {
  if (a_mcmCode < SKSE::InputMap::kMacro_NumKeyboardKeys) {
    return ScanCodeToVirtualKey(a_mcmCode);
  }
  const auto mouseIndex = a_mcmCode - kMouseButtonMcmOffset;
  return mouseIndex < kMouseButtonVKs.size() ? kMouseButtonVKs[mouseIndex] : 0;
}

std::uint32_t VirtualKeyToMcmCode(std::uint32_t a_vk) noexcept {
  for (std::size_t i = 0; i < kMouseButtonVKs.size(); ++i) {
    if (a_vk != 0 && kMouseButtonVKs[i] == a_vk) {
      return static_cast<std::uint32_t>(kMouseButtonMcmOffset + i);
    }
  }
  return VirtualKeyToScanCode(a_vk);
}

// SKSE's GamepadKeycodeToMask()/GamepadMaskToKeycode() don't round-trip
// LT/RT (see Config::kSyntheticLeftTrigger) - these wrap them to use our
// two dedicated bits instead.
std::uint32_t GamepadKeycodeToMask(std::uint32_t a_keycode) noexcept {
  if (a_keycode == SKSE::InputMap::kGamepadButtonOffset_LT) {
    return Config::kSyntheticLeftTrigger;
  }
  if (a_keycode == SKSE::InputMap::kGamepadButtonOffset_RT) {
    return Config::kSyntheticRightTrigger;
  }
  return SKSE::InputMap::GamepadKeycodeToMask(a_keycode);
}

std::uint32_t GamepadMaskToKeycode(std::uint32_t a_mask) noexcept {
  if (a_mask == Config::kSyntheticLeftTrigger) {
    return SKSE::InputMap::kGamepadButtonOffset_LT;
  }
  if (a_mask == Config::kSyntheticRightTrigger) {
    return SKSE::InputMap::kGamepadButtonOffset_RT;
  }
  return SKSE::InputMap::GamepadMaskToKeycode(a_mask);
}

std::int32_t GetHotkey(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(VirtualKeyToMcmCode(Config::Hotkey.load()));
}

void SetHotkey(RE::StaticFunctionTag *, std::int32_t a_mcmCode) {
  Config::Hotkey = McmCodeToVirtualKey(static_cast<std::uint32_t>(a_mcmCode));
  Config::Save();
}

// AddKeyMapOption accepts input from any device, so the MCM script uses
// these to reject a code from the wrong device instead of storing it.
bool IsKeyboardOrMouseKeycode(RE::StaticFunctionTag *, std::int32_t a_keycode) {
  return a_keycode >= 0 &&
         McmCodeToVirtualKey(static_cast<std::uint32_t>(a_keycode)) != 0;
}

bool IsGamepadKeycode(RE::StaticFunctionTag *, std::int32_t a_keycode) {
  return a_keycode >=
             static_cast<std::int32_t>(SKSE::InputMap::kMacro_GamepadOffset) &&
         a_keycode < static_cast<std::int32_t>(SKSE::InputMap::kMaxMacros);
}

std::int32_t GetGamepadButton(RE::StaticFunctionTag *) {
  const auto mask = Config::GamepadButton.load();
  if (mask == 0) {
    return -1;
  }
  return static_cast<std::int32_t>(GamepadMaskToKeycode(mask));
}

void SetGamepadButton(RE::StaticFunctionTag *, std::int32_t a_keycode) {
  Config::GamepadButton =
      a_keycode < 0
          ? 0
          : GamepadKeycodeToMask(static_cast<std::uint32_t>(a_keycode));
  Config::Save();
}

bool GetToggleMode(RE::StaticFunctionTag *) { return Config::ToggleMode.load(); }

void SetToggleMode(RE::StaticFunctionTag *, bool a_toggle) {
  Config::ToggleMode = a_toggle;
  Config::Save();
}

std::int32_t GetLiveZoomBoostButton(RE::StaticFunctionTag *) {
  const auto mask = Config::LiveZoomBoostButton.load();
  if (mask == 0) {
    return -1;
  }
  return static_cast<std::int32_t>(GamepadMaskToKeycode(mask));
}

void SetLiveZoomBoostButton(RE::StaticFunctionTag *, std::int32_t a_keycode) {
  Config::LiveZoomBoostButton =
      a_keycode < 0
          ? 0
          : GamepadKeycodeToMask(static_cast<std::uint32_t>(a_keycode));
  Config::Save();
}

// Default hotkey/gamepad button as MCM codes, for the MCM's per-option
// "reset to default" gesture - reuses the same conversions GetHotkey/
// GetGamepadButton do, rather than hardcoding a separate MCM code.
std::int32_t GetDefaultHotkey(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(VirtualKeyToMcmCode(0x05));
}

std::int32_t GetDefaultGamepadButton(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(
      SKSE::InputMap::GamepadMaskToKeycode(0x0004));
}

std::int32_t GetDefaultLiveZoomBoostButton(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(
      SKSE::InputMap::GamepadMaskToKeycode(0x0080));
}

float GetZoomFOV(RE::StaticFunctionTag *) { return Config::ZoomFOV.load(); }

void SetZoomFOV(RE::StaticFunctionTag *, float a_fov) {
  Config::ZoomFOV = std::clamp(a_fov, 1.0f, 170.0f);
  Config::Save();
}

float GetSmoothSpeed(RE::StaticFunctionTag *) {
  return Config::SmoothSpeed.load();
}

void SetSmoothSpeed(RE::StaticFunctionTag *, float a_speed) {
  Config::SmoothSpeed = std::clamp(a_speed, 0.1f, 60.0f);
  Config::Save();
}

bool GetEnableScrollZoomAdjust(RE::StaticFunctionTag *) {
  return Config::EnableScrollZoomAdjust.load();
}

void SetEnableScrollZoomAdjust(RE::StaticFunctionTag *, bool a_enable) {
  Config::EnableScrollZoomAdjust = a_enable;
  Config::Save();
}

float GetMinZoomFOV(RE::StaticFunctionTag *) {
  return Config::MinZoomFOV.load();
}

void SetMinZoomFOV(RE::StaticFunctionTag *, float a_fov) {
  Config::MinZoomFOV = std::clamp(a_fov, 1.0f, 170.0f);
  Config::Save();
}

bool GetScrollUsesSmoothSpeed(RE::StaticFunctionTag *) {
  return Config::ScrollUsesSmoothSpeed.load();
}

void SetScrollUsesSmoothSpeed(RE::StaticFunctionTag *, bool a_use) {
  Config::ScrollUsesSmoothSpeed = a_use;
  Config::Save();
}

std::int32_t GetViewMode(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(Config::ActiveViewMode.load());
}

void SetViewMode(RE::StaticFunctionTag *, std::int32_t a_viewMode) {
  Config::ActiveViewMode = static_cast<std::uint32_t>(std::clamp(
      a_viewMode, static_cast<std::int32_t>(Config::kFirstPersonOnly),
      static_cast<std::int32_t>(Config::kBoth)));
  Config::Save();
}

bool GetRequireWeaponSheathed(RE::StaticFunctionTag *) {
  return Config::RequireWeaponSheathed.load();
}

void SetRequireWeaponSheathed(RE::StaticFunctionTag *, bool a_require) {
  Config::RequireWeaponSheathed = a_require;
  Config::Save();
}

bool GetAllowZoomDuringDialogue(RE::StaticFunctionTag *) {
  return Config::AllowZoomDuringDialogue.load();
}

void SetAllowZoomDuringDialogue(RE::StaticFunctionTag *, bool a_allow) {
  Config::AllowZoomDuringDialogue = a_allow;
  Config::Save();
}

bool GetScaleMouseSensitivity(RE::StaticFunctionTag *) {
  return Config::ScaleMouseSensitivity.load();
}

void SetScaleMouseSensitivity(RE::StaticFunctionTag *, bool a_scale) {
  Config::ScaleMouseSensitivity = a_scale;
  Config::Save();
}

float GetSensitivityExponent(RE::StaticFunctionTag *) {
  return Config::SensitivityExponent.load();
}

void SetSensitivityExponent(RE::StaticFunctionTag *, float a_exponent) {
  Config::SensitivityExponent = std::clamp(a_exponent, 0.1f, 10.0f);
  Config::Save();
}

// Read from the plugin declaration rather than hardcoded, so it can't drift
// out of sync with the actual build.
std::string GetPluginVersion(RE::StaticFunctionTag *) {
  const auto *decl = SKSE::PluginDeclaration::GetSingleton();
  return decl ? decl->GetVersion().string(".") : std::string{};
}

bool RegisterFunctions(RE::BSScript::IVirtualMachine *a_vm) {
  constexpr auto kClassName = "SkyZoom_Native";

  a_vm->RegisterFunction("GetHotkey", kClassName, GetHotkey);
  a_vm->RegisterFunction("SetHotkey", kClassName, SetHotkey);
  a_vm->RegisterFunction("GetGamepadButton", kClassName, GetGamepadButton);
  a_vm->RegisterFunction("SetGamepadButton", kClassName, SetGamepadButton);
  a_vm->RegisterFunction("GetToggleMode", kClassName, GetToggleMode);
  a_vm->RegisterFunction("SetToggleMode", kClassName, SetToggleMode);
  a_vm->RegisterFunction("GetDefaultHotkey", kClassName, GetDefaultHotkey);
  a_vm->RegisterFunction("GetDefaultGamepadButton", kClassName,
                         GetDefaultGamepadButton);
  a_vm->RegisterFunction("IsKeyboardOrMouseKeycode", kClassName,
                         IsKeyboardOrMouseKeycode);
  a_vm->RegisterFunction("IsGamepadKeycode", kClassName, IsGamepadKeycode);
  a_vm->RegisterFunction("GetZoomFOV", kClassName, GetZoomFOV);
  a_vm->RegisterFunction("SetZoomFOV", kClassName, SetZoomFOV);
  a_vm->RegisterFunction("GetSmoothSpeed", kClassName, GetSmoothSpeed);
  a_vm->RegisterFunction("SetSmoothSpeed", kClassName, SetSmoothSpeed);
  a_vm->RegisterFunction("GetEnableScrollZoomAdjust", kClassName,
                         GetEnableScrollZoomAdjust);
  a_vm->RegisterFunction("SetEnableScrollZoomAdjust", kClassName,
                         SetEnableScrollZoomAdjust);
  a_vm->RegisterFunction("GetMinZoomFOV", kClassName, GetMinZoomFOV);
  a_vm->RegisterFunction("SetMinZoomFOV", kClassName, SetMinZoomFOV);
  a_vm->RegisterFunction("GetScrollUsesSmoothSpeed", kClassName,
                         GetScrollUsesSmoothSpeed);
  a_vm->RegisterFunction("SetScrollUsesSmoothSpeed", kClassName,
                         SetScrollUsesSmoothSpeed);
  a_vm->RegisterFunction("GetLiveZoomBoostButton", kClassName,
                         GetLiveZoomBoostButton);
  a_vm->RegisterFunction("SetLiveZoomBoostButton", kClassName,
                         SetLiveZoomBoostButton);
  a_vm->RegisterFunction("GetDefaultLiveZoomBoostButton", kClassName,
                         GetDefaultLiveZoomBoostButton);
  a_vm->RegisterFunction("GetViewMode", kClassName, GetViewMode);
  a_vm->RegisterFunction("SetViewMode", kClassName, SetViewMode);
  a_vm->RegisterFunction("GetRequireWeaponSheathed", kClassName,
                         GetRequireWeaponSheathed);
  a_vm->RegisterFunction("SetRequireWeaponSheathed", kClassName,
                         SetRequireWeaponSheathed);
  a_vm->RegisterFunction("GetAllowZoomDuringDialogue", kClassName,
                         GetAllowZoomDuringDialogue);
  a_vm->RegisterFunction("SetAllowZoomDuringDialogue", kClassName,
                         SetAllowZoomDuringDialogue);
  a_vm->RegisterFunction("GetScaleMouseSensitivity", kClassName,
                         GetScaleMouseSensitivity);
  a_vm->RegisterFunction("SetScaleMouseSensitivity", kClassName,
                         SetScaleMouseSensitivity);
  a_vm->RegisterFunction("GetSensitivityExponent", kClassName,
                         GetSensitivityExponent);
  a_vm->RegisterFunction("SetSensitivityExponent", kClassName,
                         SetSensitivityExponent);
  a_vm->RegisterFunction("GetPluginVersion", kClassName, GetPluginVersion);

  return true;
}
} // namespace

void Register() { SKSE::GetPapyrusInterface()->Register(RegisterFunctions); }
} // namespace Papyrus
