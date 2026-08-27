#pragma once

#include <atomic>
#include <cstdint>

// std::atomic: read every frame on the render thread, written from the
// Papyrus VM thread whenever an MCM option changes.
namespace Config {
inline std::atomic<std::uint32_t> Hotkey =
    0x05; // VK_XBUTTON1 (Mouse Button 4 / M4)
inline std::atomic<std::uint32_t> GamepadButton =
    0x0004; // XINPUT_GAMEPAD_DPAD_LEFT
inline std::atomic<float> ZoomFOV = 60.0f;
inline std::atomic<float> SmoothSpeed = 8.0f;

enum ViewMode : std::uint32_t {
  kFirstPersonOnly = 0,
  kThirdPersonOnly = 1,
  kBoth = 2,
};
inline std::atomic<std::uint32_t> ActiveViewMode = kBoth;

// Zoom only while not in a ready/fighting stance (weapon, spell, or H2H).
// Default: on.
inline std::atomic<bool> RequireWeaponSheathed = true;

// Zoom during dialogue too, overriding RequireWeaponSheathed for that
// conversation. Default: off.
inline std::atomic<bool> AllowZoomDuringDialogue = false;

// Whether to scale mouse sensitivity down while zoomed, so aim doesn't feel
// twitchy at a narrower FOV. Default: on.
inline std::atomic<bool> ScaleMouseSensitivity = true;
// Exponent applied to the FOV ratio (currentFOV/baseFOV) when scaling mouse
// sensitivity while zoomed - 1.0 scales linearly with FOV, higher values cut
// sensitivity more aggressively at moderate zoom without changing the
// no-zoom (ratio=1) or fully-zoomed (ratio=0) endpoints. Only relevant while
// ScaleMouseSensitivity is on.
inline std::atomic<float> SensitivityExponent = 2.5f;

// Loads SkyZoom.ini next to the DLL, writing defaults on first run.
void Load();

// Writes the current values back to SkyZoom.ini - called after any MCM edit
// so the ini stays the single source of truth regardless of who wrote it.
void Save();
} // namespace Config
