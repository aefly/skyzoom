#include "Papyrus.h"

#include "Config.h"

#include "REX/W32/BASE.h"
#include "REX/W32/USER32.h"
#include "SKSE/InputMap.h"

#include <array>

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

std::int32_t GetHotkey(RE::StaticFunctionTag *) {
  return static_cast<std::int32_t>(VirtualKeyToMcmCode(Config::Hotkey));
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
  if (Config::GamepadButton == 0) {
    return -1;
  }
  return static_cast<std::int32_t>(
      SKSE::InputMap::GamepadMaskToKeycode(Config::GamepadButton));
}

void SetGamepadButton(RE::StaticFunctionTag *, std::int32_t a_keycode) {
  Config::GamepadButton = a_keycode < 0
                              ? 0
                              : SKSE::InputMap::GamepadKeycodeToMask(
                                    static_cast<std::uint32_t>(a_keycode));
  Config::Save();
}

float GetZoomFOV(RE::StaticFunctionTag *) { return Config::ZoomFOV; }

void SetZoomFOV(RE::StaticFunctionTag *, float a_fov) {
  Config::ZoomFOV = std::clamp(a_fov, 1.0f, 170.0f);
  Config::Save();
}

float GetSmoothSpeed(RE::StaticFunctionTag *) { return Config::SmoothSpeed; }

void SetSmoothSpeed(RE::StaticFunctionTag *, float a_speed) {
  Config::SmoothSpeed = std::clamp(a_speed, 0.1f, 60.0f);
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
  a_vm->RegisterFunction("IsKeyboardOrMouseKeycode", kClassName,
                         IsKeyboardOrMouseKeycode);
  a_vm->RegisterFunction("IsGamepadKeycode", kClassName, IsGamepadKeycode);
  a_vm->RegisterFunction("GetZoomFOV", kClassName, GetZoomFOV);
  a_vm->RegisterFunction("SetZoomFOV", kClassName, SetZoomFOV);
  a_vm->RegisterFunction("GetSmoothSpeed", kClassName, GetSmoothSpeed);
  a_vm->RegisterFunction("SetSmoothSpeed", kClassName, SetSmoothSpeed);
  a_vm->RegisterFunction("GetPluginVersion", kClassName, GetPluginVersion);

  return true;
}
} // namespace

void Register() { SKSE::GetPapyrusInterface()->Register(RegisterFunctions); }
} // namespace Papyrus
