#pragma once

namespace FOVController {
void Update();

// True whenever a menu/pause state has taken over input - shared with
// Input.cpp's TriggerSink so it doesn't act while a menu is open (see
// Update()'s own use of this for the zoom hotkey itself).
bool IsMenuOpen() noexcept;

// For compatibility patches that need to blend toward SkyZoom's zoom rather
// than substitute it outright - a weight, not an absolute FOV, since worldFOV
// may not reflect what's really on screen.
// True while zooming/easing (writes a_outWeight 0..1 and a_outTargetFOV);
// false otherwise.
bool GetActiveZoomWeight(float &a_outWeight, float &a_outTargetFOV) noexcept;
} // namespace FOVController
