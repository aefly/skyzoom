#include "Config.h"

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
      << SmoothSpeed << "\n";
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
    }
  }

  SKSE::log::info(
      "SkyZoom config: Hotkey=0x{:X} GamepadButton=0x{:X} ZoomFOV={:.1f} "
      "SmoothSpeed={:.1f}",
      Hotkey, GamepadButton, ZoomFOV, SmoothSpeed);
}

void Save() {
  WriteIni(GetIniPath());

  SKSE::log::info("SkyZoom config saved: Hotkey=0x{:X} GamepadButton=0x{:X} "
                  "ZoomFOV={:.1f} SmoothSpeed={:.1f}",
                  Hotkey, GamepadButton, ZoomFOV, SmoothSpeed);
}
} // namespace Config
