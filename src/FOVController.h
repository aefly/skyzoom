#pragma once

namespace FOVController {
void Update();

// For compatibility patches that need to blend toward SkyZoom's zoom rather
// than substitute it outright - a weight, not an absolute FOV, since worldFOV
// may not reflect what's really on screen.
// True while zooming/easing (writes a_outWeight 0..1 and a_outTargetFOV);
// false otherwise.
bool GetActiveZoomWeight(float &a_outWeight, float &a_outTargetFOV) noexcept;
} // namespace FOVController
