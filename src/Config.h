#pragma once

#include <cstdint>

namespace Config {
inline std::uint32_t Hotkey = 0x05; // VK_XBUTTON1 (Mouse Button 4 / M4)
inline std::uint32_t GamepadButton = 0x0004; // XINPUT_GAMEPAD_DPAD_LEFT
inline float ZoomFOV = 60.0f;
inline float SmoothSpeed = 8.0f;

// Loads SkyZoom.ini next to the DLL, writing defaults on first run.
void Load();

// Writes the current values back to SkyZoom.ini - called after any MCM edit
// so the ini stays the single source of truth regardless of who wrote it.
void Save();
} // namespace Config
