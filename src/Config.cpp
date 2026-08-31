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
// CommonLibSSE-NG has no REX::FModule (CommonLibSF-only), so resolve this
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
         "# Edit and save, then restart the game to apply changes - or use "
         "the MCM menu (if SkyUI is installed), which edits this same file "
         "and applies changes immediately.\n"
         "\n"
         "# Virtual-key code of the hold-to-zoom hotkey.\n"
         "# Default is 5 (0x05), Mouse Button 4 / M4.\n"
         "Hotkey="
      << Hotkey.load()
      << "\n"
         "\n"
         "# XInput gamepad button bitmask that also holds-to-zoom (0 = "
         "disabled).\n"
         "# Default is 4 (0x0004), D-Pad Left.\n"
         "GamepadButton="
      << GamepadButton.load()
      << "\n"
         "\n"
         "# Whether the hotkey (keyboard/mouse or gamepad above) toggles the "
         "zoom on/off instead of requiring it held down - press once to "
         "zoom in, press again to zoom out (1 = on; 0 = off, default, "
         "hold-to-zoom).\n"
         "ToggleMode="
      << (ToggleMode.load() ? 1 : 0)
      << "\n"
         "\n"
         "# FOV in degrees while the hotkey is held.\n"
         "ZoomFOV="
      << ZoomFOV.load()
      << "\n"
         "\n"
         "# Exponential smoothing rate for the zoom-in/out animation.\n"
         "# Higher = snappier, lower = more gradual.\n"
         "SmoothSpeed="
      << SmoothSpeed.load()
      << "\n"
         "\n"
         "# Whether scrolling the mouse wheel (or holding LiveZoomBoostButton "
         "below) while the hotkey is held adjusts the zoom live, between "
         "MinZoomFOV and ZoomFOV below (1 = on, default; 0 = off). ZoomFOV "
         "is the loose end of that range - releasing the hotkey is still "
         "the only way back to the unzoomed FOV; releasing the boost button "
         "alone also snaps back to ZoomFOV, unlike the wheel, which "
         "persists.\n"
         "EnableScrollZoomAdjust="
      << (EnableScrollZoomAdjust.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Tightest FOV reachable via the wheel or LiveZoomBoostButton "
         "below, while EnableScrollZoomAdjust=1 above.\n"
         "MinZoomFOV="
      << MinZoomFOV.load()
      << "\n"
         "\n"
         "# Whether the scroll-zoom ease above uses the same speed as "
         "SmoothSpeed below, instead of its own fixed, faster timing (1 = "
         "on; 0 = off, default).\n"
         "ScrollUsesSmoothSpeed="
      << (ScrollUsesSmoothSpeed.load() ? 1 : 0)
      << "\n"
         "\n"
         "# XInput gamepad button bitmask for the live zoom boost above "
         "(0 = disabled). Default is 128 (0x0080), Right Stick Click (R3) "
         "- deliberately not LB/RB, since Shout and most transform/power "
         "binds charge-and-release on those and can misfire on release if "
         "used here instead.\n"
         "LiveZoomBoostButton="
      << LiveZoomBoostButton.load()
      << "\n"
         "\n"
         "# Which view(s) the hotkey zooms in. 0 = first person only, 1 = "
         "third person only, 2 = both (default).\n"
         "ViewMode="
      << ActiveViewMode.load()
      << "\n"
         "\n"
         "# Whether the hotkey only zooms while not in a ready/fighting "
         "stance - a drawn weapon or spell counts, and so does empty-handed "
         "H2H ready stance (1 = on, default; 0 = zoom works from any stance "
         "too).\n"
         "RequireWeaponSheathed="
      << (RequireWeaponSheathed.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Whether the hotkey still zooms while talking to an NPC, even "
         "from a ready stance (overrides RequireWeaponSheathed for that "
         "conversation) (1 = on; 0 = off, default, since conversations "
         "don't pause the game).\n"
         "AllowZoomDuringDialogue="
      << (AllowZoomDuringDialogue.load() ? 1 : 0)
      << "\n"
         "\n"
         "# Whether to scale mouse and gamepad look sensitivity down while "
         "zoomed, so aim doesn't feel twitchy at a narrower FOV (1 = on, 0 "
         "= off).\n"
         "ScaleMouseSensitivity="
      << (ScaleMouseSensitivity.load() ? 1 : 0)
      << "\n"
         "\n"
         "# How aggressively sensitivity is cut while zoomed (only used\n"
         "# while ScaleMouseSensitivity=1 above). 1.0 cuts it in direct "
         "proportion\n"
         "# to the FOV ratio; higher values cut it more aggressively at "
         "moderate\n"
         "# zoom without changing the no-zoom or fully-zoomed endpoints.\n"
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
      "ViewMode={} RequireWeaponSheathed={} AllowZoomDuringDialogue={} "
      "ScaleMouseSensitivity={} SensitivityExponent={:.2f}",
      Hotkey.load(), GamepadButton.load(), ToggleMode.load(), ZoomFOV.load(),
      SmoothSpeed.load(), EnableScrollZoomAdjust.load(), MinZoomFOV.load(),
      ScrollUsesSmoothSpeed.load(), LiveZoomBoostButton.load(),
      ActiveViewMode.load(), RequireWeaponSheathed.load(),
      AllowZoomDuringDialogue.load(), ScaleMouseSensitivity.load(),
      SensitivityExponent.load());
}

void Save() {
  WriteIni(GetIniPath());

  SKSE::log::info("SkyZoom config saved: Hotkey=0x{:X} GamepadButton=0x{:X} "
                  "ToggleMode={} ZoomFOV={:.1f} SmoothSpeed={:.1f} "
                  "EnableScrollZoomAdjust={} MinZoomFOV={:.1f} "
                  "ScrollUsesSmoothSpeed={} LiveZoomBoostButton=0x{:X} "
                  "ViewMode={} RequireWeaponSheathed={} "
                  "AllowZoomDuringDialogue={} ScaleMouseSensitivity={} "
                  "SensitivityExponent={:.2f}",
                  Hotkey.load(), GamepadButton.load(), ToggleMode.load(),
                  ZoomFOV.load(), SmoothSpeed.load(),
                  EnableScrollZoomAdjust.load(), MinZoomFOV.load(),
                  ScrollUsesSmoothSpeed.load(), LiveZoomBoostButton.load(),
                  ActiveViewMode.load(), RequireWeaponSheathed.load(),
                  AllowZoomDuringDialogue.load(), ScaleMouseSensitivity.load(),
                  SensitivityExponent.load());
}
} // namespace Config
