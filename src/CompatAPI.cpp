#include "CompatAPI.h"

#include "FOVController.h"

extern "C" __declspec(dllexport) bool SkyZoom_GetActiveZoomWeight(
    float *a_outWeight, float *a_outTargetFOV) noexcept {
  return a_outWeight && a_outTargetFOV &&
         FOVController::GetActiveZoomWeight(*a_outWeight, *a_outTargetFOV);
}
