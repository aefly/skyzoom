#pragma once

namespace Input {
bool IsHotkeyDown() noexcept;

// Registers the mouse wheel sink - call once SKSE's kInputLoaded message
// fires, so RE::BSInputDeviceManager is guaranteed to exist.
void InstallWheelSink();

// Net mouse wheel notches scrolled since the last call (positive = up/in,
// negative = down/out), then resets the count to zero. Safe to call every
// frame even while nothing is listening - the count is simply dropped.
std::int32_t ConsumeScrollSteps() noexcept;

// Gamepad counterpart of ConsumeScrollSteps() above, for LB/RB - continuous
// held state (+1 = RB/zoom-in, -1 = LB/zoom-out, 0 = neither/both) rather
// than discrete steps, since holding is the natural gamepad gesture.
std::int32_t GetGamepadScrollDirection() noexcept;

// Tells the wheel sink whether scroll-to-adjust-zoom is currently claiming
// the wheel, so it can neuter a claimed notch before it also reaches an
// open menu or vanilla's own POV-switch/camera-zoom.
void SetScrollZoomActive(bool a_active) noexcept;
} // namespace Input
