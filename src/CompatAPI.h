#pragma once

// Stable C ABI for compatibility patches, resolved by name via GetProcAddress.
extern "C" __declspec(dllexport) bool
SkyZoom_GetActiveZoomWeight(float *a_outWeight, float *a_outTargetFOV) noexcept;
