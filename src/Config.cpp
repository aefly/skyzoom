#include "Config.h"

// Some RE/* headers pull in the real <d3d11.h> (and therefore <windows.h>)
// after REX/W32/BASE.h has already been parsed, so its "no real Windows.h"
// guard doesn't catch it. That leaves MAX_PATH as an object-like macro,
// shadowing REX::W32::MAX_PATH - undef it so the qualified name resolves.
#ifdef MAX_PATH
#undef MAX_PATH
#endif

namespace Config {
namespace {
// CommonLibSSE-NG has no REX::FModule, so resolve this
// DLL's own path via GetCurrentModule() instead.
std::filesystem::path GetIniPath() {
  wchar_t buffer[REX::W32::MAX_PATH]{};
  const auto module = REX::W32::GetCurrentModule();
  REX::W32::GetModuleFileNameW(module, buffer,
                               static_cast<std::uint32_t>(std::size(buffer)));
  return std::filesystem::path(buffer).parent_path() / "SkyZoom.ini";
}

// Shared by the first-run default write and Save() - just with current
// values instead of hardcoded ones. Keep in sync with dist/SkyZoom.ini.
void WriteIni(const std::filesystem::path &a_path) {
  std::ofstream out(a_path);
  out << "# SkyZoom configuration\n"
         "# Edit and save, then restart the game to apply changes.\n"
         "\n"
         "# Keyboard/Mouse key used to activate zoom\n"
         "# Default: 5 = Mouse Button 4 (M4)\n"
         "Hotkey="
      << Hotkey.load()
      << "\n"
         "\n"
         "# Gamepad button used to activate zoom (Xbox gamepads only)\n"
         "# Default: 4 = D-Pad Left\n"
         "# 0 = disabled\n"
         "GamepadButton="
      << GamepadButton.load()
      << "\n"
         "\n"
         "# Zoom mode\n"
         "# 0 = hold the button to zoom\n"
         "# 1 = press once to zoom, press again to stop\n"
         "ToggleMode="
      << (ToggleMode.load() ? 1 : 0)
      << "\n"
         "\n"
         "# FOV when zooming\n"
         "# Lower number = more zoom\n"
         "ZoomFOV="
      << ZoomFOV.load()
      << "\n"
         "\n"
         "# Zoom speed\n"
         "# Higher = faster zoom\n"
         "SmoothSpeed="
      << SmoothSpeed.load()
      << "\n"
         "\n"
         "# Allows extra zoom to be changed with the mouse wheel or\n"
         "# dedicated gamepad button\n"
         "# 1 = enabled\n"
         "# 0 = disabled\n"
         "EnableScrollZoomAdjust="
      << (EnableScrollZoomAdjust.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Tightest zoom reachable with the mouse wheel or\n"
         "# LiveZoomBoostButton below\n"
         "# Lower number = more zoom\n"
         "MinZoomFOV="
      << MinZoomFOV.load()
      << "\n"
         "\n"
         "# Uses SmoothSpeed for mouse wheel/gamepad live zoom adjust\n"
         "# 1 = enabled\n"
         "# 0 = disabled\n"
         "ScrollUsesSmoothSpeed="
      << (ScrollUsesSmoothSpeed.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Gamepad button used for extra zoom\n"
         "# Only works when Hotkey is held and EnableScrollZoomAdjust\n"
         "# is enabled\n"
         "# Default: 128 = Right Stick Button (RSB)\n"
         "# 0 = disabled\n"
         "LiveZoomBoostButton="
      << LiveZoomBoostButton.load()
      << "\n"
         "\n"
         "# When GamepadButton is LT/RT, disables its normal\n"
         "# function while zooming with the weapon sheathed\n"
         "# 1 = enabled\n"
         "# 0 = disabled\n"
         "DisableTriggerWhenSheathed="
      << (DisableTriggerWhenSheathed.load() ? 1 : 0)
      << "\n"
         "\n"
         "# View where zoom works\n"
         "# 0 = first person\n"
         "# 1 = third person\n"
         "# 2 = both\n"
         "ViewMode="
      << ActiveViewMode.load()
      << "\n"
         "\n"
         "# Zoom only works when the weapon is sheathed\n"
         "# 1 = yes\n"
         "# 0 = zoom works at any time\n"
         "RequireWeaponSheathed="
      << (RequireWeaponSheathed.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Allows zoom during NPC dialogue\n"
         "# 1 = enabled\n"
         "# 0 = disabled\n"
         "AllowZoomDuringDialogue="
      << (AllowZoomDuringDialogue.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Reduces mouse and gamepad sensitivity while zoomed\n"
         "# 1 = enabled\n"
         "# 0 = disabled\n"
         "ScaleMouseSensitivity="
      << (ScaleMouseSensitivity.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Controls how much sensitivity is reduced\n"
         "# Higher number = lower sensitivity while zoomed\n"
         "SensitivityExponent="
      << SensitivityExponent.load() << "\n";
}

std::string Trim(std::string a_str) {
  const auto first = a_str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = a_str.find_last_not_of(" \t\r\n");
  return a_str.substr(first, last - first + 1);
}
} // namespace

void Load() {
  const auto path = GetIniPath();

  if (!std::filesystem::exists(path)) {
    WriteIni(path);
  }

  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    line = Trim(line);
    if (line.empty() || line.front() == '#' || line.front() == ';') {
      continue;
    }

    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }

    const auto key = Trim(line.substr(0, eq));
    const auto val = Trim(line.substr(eq + 1));

    if (key == "Hotkey") {
      Hotkey =
          static_cast<std::uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
    } else if (key == "GamepadButton") {
      GamepadButton =
          static_cast<std::uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
    } else if (key == "ToggleMode") {
      ToggleMode = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "ZoomFOV") {
      ZoomFOV = std::clamp(std::strtof(val.c_str(), nullptr), 1.0f, 170.0f);
    } else if (key == "SmoothSpeed") {
      SmoothSpeed = std::clamp(std::strtof(val.c_str(), nullptr), 0.1f, 60.0f);
    } else if (key == "EnableScrollZoomAdjust") {
      EnableScrollZoomAdjust = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "MinZoomFOV") {
      MinZoomFOV = std::clamp(std::strtof(val.c_str(), nullptr), 1.0f, 170.0f);
    } else if (key == "ScrollUsesSmoothSpeed") {
      ScrollUsesSmoothSpeed = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "LiveZoomBoostButton") {
      LiveZoomBoostButton =
          static_cast<std::uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
    } else if (key == "DisableTriggerWhenSheathed") {
      DisableTriggerWhenSheathed = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "ViewMode") {
      ActiveViewMode = std::clamp(
          static_cast<std::uint32_t>(std::strtoul(val.c_str(), nullptr, 10)),
          static_cast<std::uint32_t>(kFirstPersonOnly),
          static_cast<std::uint32_t>(kBoth));
    } else if (key == "RequireWeaponSheathed") {
      RequireWeaponSheathed = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "AllowZoomDuringDialogue") {
      AllowZoomDuringDialogue = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "ScaleMouseSensitivity") {
      ScaleMouseSensitivity = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "SensitivityExponent") {
      SensitivityExponent =
          std::clamp(std::strtof(val.c_str(), nullptr), 0.1f, 10.0f);
    }
  }

  SKSE::log::info(
      "SkyZoom config: Hotkey=0x{:X} GamepadButton=0x{:X} ToggleMode={} "
      "ZoomFOV={:.1f} SmoothSpeed={:.1f} EnableScrollZoomAdjust={} "
      "MinZoomFOV={:.1f} ScrollUsesSmoothSpeed={} LiveZoomBoostButton=0x{:X} "
      "DisableTriggerWhenSheathed={} ViewMode={} RequireWeaponSheathed={} "
      "AllowZoomDuringDialogue={} ScaleMouseSensitivity={} "
      "SensitivityExponent={:.2f}",
      Hotkey.load(), GamepadButton.load(), ToggleMode.load(), ZoomFOV.load(),
      SmoothSpeed.load(), EnableScrollZoomAdjust.load(), MinZoomFOV.load(),
      ScrollUsesSmoothSpeed.load(), LiveZoomBoostButton.load(),
      DisableTriggerWhenSheathed.load(), ActiveViewMode.load(),
      RequireWeaponSheathed.load(), AllowZoomDuringDialogue.load(),
      ScaleMouseSensitivity.load(), SensitivityExponent.load());
}

void Save() {
  WriteIni(GetIniPath());

  SKSE::log::info("SkyZoom config saved: Hotkey=0x{:X} GamepadButton=0x{:X} "
                  "ToggleMode={} ZoomFOV={:.1f} SmoothSpeed={:.1f} "
                  "EnableScrollZoomAdjust={} MinZoomFOV={:.1f} "
                  "ScrollUsesSmoothSpeed={} LiveZoomBoostButton=0x{:X} "
                  "DisableTriggerWhenSheathed={} ViewMode={} "
                  "RequireWeaponSheathed={} AllowZoomDuringDialogue={} "
                  "ScaleMouseSensitivity={} SensitivityExponent={:.2f}",
                  Hotkey.load(), GamepadButton.load(), ToggleMode.load(),
                  ZoomFOV.load(), SmoothSpeed.load(),
                  EnableScrollZoomAdjust.load(), MinZoomFOV.load(),
                  ScrollUsesSmoothSpeed.load(), LiveZoomBoostButton.load(),
                  DisableTriggerWhenSheathed.load(), ActiveViewMode.load(),
                  RequireWeaponSheathed.load(), AllowZoomDuringDialogue.load(),
                  ScaleMouseSensitivity.load(), SensitivityExponent.load());
}
} // namespace Config
