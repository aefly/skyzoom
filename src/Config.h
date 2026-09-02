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

// LT/RT are analog triggers, not real XINPUT_GAMEPAD::buttons bits - SKSE's
// GamepadKeycodeToMask() returns a bogus value for them, so these two bits
// (outside XInput's real 0-15 button range) stand in instead; Input.cpp
// checks the analog trigger value for them.
inline constexpr std::uint32_t kSyntheticLeftTrigger = 0x0001'0000;
inline constexpr std::uint32_t kSyntheticRightTrigger = 0x0002'0000;

// When GamepadButton above is bound to LT/RT, also neuter that trigger's
// vanilla ButtonEvent (left/right attack-block) while the weapon is
// sheathed, freeing it up to be used purely for zoom - drawing a weapon
// restores its normal function immediately. Default: off.
inline std::atomic<bool> DisableTriggerWhenSheathed = false;

// Toggles zoom on/off on press instead of requiring the hotkey held.
// Default: off (hold-to-zoom).
inline std::atomic<bool> ToggleMode = false;

inline std::atomic<float> ZoomFOV = 60.0f;
inline std::atomic<float> SmoothSpeed = 8.0f;

// Whether scrolling the mouse wheel (or holding LiveZoomBoostButton below)
// while the hotkey is held adjusts the zoom live, between MinZoomFOV and
// ZoomFOV (ZoomFOV acts as the loose-end ceiling - releasing the hotkey is
// still the only way back to the unzoomed FOV; releasing the boost button
// alone also snaps back to ZoomFOV, unlike the wheel, which persists).
// Default: on.
inline std::atomic<bool> EnableScrollZoomAdjust = true;
// Tightest FOV reachable by scrolling in while EnableScrollZoomAdjust is on.
// Clamped to ZoomFOV at use time, so a misconfigured value above it can't
// invert the range.
inline std::atomic<float> MinZoomFOV = 20.0f;
// Whether the scroll-zoom ease uses SmoothSpeed's duration instead of its
// own fixed, faster timing. Off by default - a scroll notch is a much
// smaller hop than the full press-in sweep SmoothSpeed is tuned for.
inline std::atomic<bool> ScrollUsesSmoothSpeed = false;
// XInput gamepad button for the live zoom boost.
inline std::atomic<std::uint32_t> LiveZoomBoostButton = 0x0080;

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

// Whether to scale mouse and gamepad look sensitivity down while zoomed, so
// aim doesn't feel twitchy at a narrower FOV. Default: on.
inline std::atomic<bool> ScaleMouseSensitivity = true;
// Exponent applied to the FOV ratio (currentFOV/baseFOV) when scaling
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
