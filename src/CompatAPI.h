#pragma once

// Stable C ABI for compatibility patches, resolved by name via
// GetProcAddress rather than linked - so they load independently of us.
extern "C" __declspec(dllexport) bool SkyZoom_GetActiveZoomWeight(
    float *a_outWeight, float *a_outTargetFOV) noexcept;
