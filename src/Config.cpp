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
      << Hotkey
      << "\n"
         "\n"
         "# XInput gamepad button bitmask that also holds-to-zoom (0 = "
         "disabled).\n"
         "# Default is 4 (0x0004), D-Pad Left.\n"
         "GamepadButton="
      << GamepadButton
      << "\n"
         "\n"
         "# FOV in degrees while the hotkey is held.\n"
         "ZoomFOV="
      << ZoomFOV
      << "\n"
         "\n"
         "# Exponential smoothing rate for the zoom-in/out animation.\n"
         "# Higher = snappier, lower = more gradual.\n"
         "SmoothSpeed="
      << SmoothSpeed
      << "\n"
         "\n"
         "# Which view(s) the hotkey zooms in. 0 = first person only, 1 = "
         "third person only, 2 = both (default).\n"
         "ViewMode="
      << ActiveViewMode
      << "\n"
         "\n"
         "# Whether to scale mouse sensitivity down while zoomed, so aim "
         "doesn't feel twitchy at a narrower FOV (1 = on, 0 = off).\n"
         "ScaleMouseSensitivity="
      << (ScaleMouseSensitivity ? 1 : 0)
      << "\n"
         "\n"
         "# How aggressively mouse sensitivity is cut while zoomed (only "
         "used\n"
         "# while ScaleMouseSensitivity=1 above). 1.0 cuts it in direct "
         "proportion\n"
         "# to the FOV ratio; higher values cut it more aggressively at "
         "moderate\n"
         "# zoom without changing the no-zoom or fully-zoomed endpoints.\n"
         "SensitivityExponent="
      << SensitivityExponent << "\n";
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
    } else if (key == "ZoomFOV") {
      ZoomFOV = std::clamp(std::strtof(val.c_str(), nullptr), 1.0f, 170.0f);
    } else if (key == "SmoothSpeed") {
      SmoothSpeed = std::clamp(std::strtof(val.c_str(), nullptr), 0.1f, 60.0f);
    } else if (key == "ViewMode") {
      ActiveViewMode = std::clamp(
          static_cast<std::uint32_t>(std::strtoul(val.c_str(), nullptr, 10)),
          static_cast<std::uint32_t>(kFirstPersonOnly),
          static_cast<std::uint32_t>(kBoth));
    } else if (key == "ScaleMouseSensitivity") {
      ScaleMouseSensitivity = std::strtoul(val.c_str(), nullptr, 10) != 0;
    } else if (key == "SensitivityExponent") {
      SensitivityExponent =
          std::clamp(std::strtof(val.c_str(), nullptr), 0.1f, 10.0f);
    }
  }

  SKSE::log::info(
      "SkyZoom config: Hotkey=0x{:X} GamepadButton=0x{:X} ZoomFOV={:.1f} "
      "SmoothSpeed={:.1f} ViewMode={} ScaleMouseSensitivity={} "
      "SensitivityExponent={:.2f}",
      Hotkey, GamepadButton, ZoomFOV, SmoothSpeed, ActiveViewMode,
      ScaleMouseSensitivity, SensitivityExponent);
}

void Save() {
  WriteIni(GetIniPath());

  SKSE::log::info(
      "SkyZoom config saved: Hotkey=0x{:X} GamepadButton=0x{:X} "
      "ZoomFOV={:.1f} SmoothSpeed={:.1f} ViewMode={} ScaleMouseSensitivity={} "
      "SensitivityExponent={:.2f}",
      Hotkey, GamepadButton, ZoomFOV, SmoothSpeed, ActiveViewMode,
      ScaleMouseSensitivity, SensitivityExponent);
}
} // namespace Config
