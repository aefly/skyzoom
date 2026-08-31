#pragma once

namespace Input {
bool IsHotkeyDown() noexcept;

// Exposed for ToggleMode's edge detection to freeze on, since
// IsHotkeyDown() always reads false while unfocused.
bool IsGameWindowFocused() noexcept;

// Registers the mouse wheel sink - call once SKSE's kInputLoaded message
// fires, so RE::BSInputDeviceManager is guaranteed to exist.
void InstallWheelSink();

// Net mouse wheel notches scrolled since the last call (positive = up/in,
// negative = down/out), then resets the count to zero. Safe to call every
// frame even while nothing is listening - the count is simply dropped.
std::int32_t ConsumeScrollSteps() noexcept;

// Gamepad counterpart of ConsumeScrollSteps() above, for
// Config::LiveZoomBoostButton - true while held. Releasing it is handled
// by the caller, not this function.
bool IsGamepadZoomBoostDown() noexcept;

// Tells the wheel sink whether scroll-to-adjust-zoom is currently claiming
// the wheel, so it can neuter a claimed notch before it also reaches an
// open menu or vanilla's own POV-switch/camera-zoom.
void SetScrollZoomActive(bool a_active) noexcept;
} // namespace Input
