#include "Hook.h"

#include <MinHook.h>

// Some RE/* headers pull in the real <d3d11.h> (and therefore <windows.h>)
// after REX/W32/BASE.h has already been parsed - see SkyZoom's own
// Hooks.cpp/Config.cpp for the same note. Not known to bite here, but cheap
// insurance against a future include-order change.
#ifdef MAX_PATH
#undef MAX_PATH
#endif

namespace Hook {
namespace {
using NiCameraUpdate_t = void(__cdecl *)(RE::NiCamera *, float, float, float,
                                         std::uint32_t, std::uint32_t,
                                         std::uint8_t, std::uint8_t, float);
using GetActiveZoomWeight_t = bool(__cdecl *)(float *, float *);

NiCameraUpdate_t g_original = nullptr;
GetActiveZoomWeight_t g_getActiveZoomWeight = nullptr;

// Last per-frame FOV-apply call before the renderer, so it sees FirstPersonFOV's write
// regardless of order. Blend by weight, not substitute, to avoid a visible
// snap - same reasoning as that patch. No shared hook address with
// FirstPersonFOV though, so install order doesn't matter here.
void __cdecl hkNiCameraUpdate(RE::NiCamera *a_camera, float a_fov, float a_near,
                              float a_far, std::uint32_t a_screenWidth,
                              std::uint32_t a_screenHeight, std::uint8_t a_unk7,
                              std::uint8_t a_unk8, float a_fov2) {
  float weight = 0.0f;
  float targetFOV = 0.0f;
  if (g_getActiveZoomWeight && g_getActiveZoomWeight(&weight, &targetFOV)) {
    a_fov += (targetFOV - a_fov) * weight;
    a_fov2 += (targetFOV - a_fov2) * weight;
  }

  g_original(a_camera, a_fov, a_near, a_far, a_screenWidth, a_screenHeight,
             a_unk7, a_unk8, a_fov2);
}

// Resolved by name, not linked, so this degrades to a no-op instead of
// failing to load if SkyZoom isn't present or is too old to export it.
void ResolveSkyZoomAPI() noexcept {
  auto *module = REX::W32::GetModuleHandleW(L"SkyZoom.dll");
  if (!module) {
    SKSE::log::warn("SkyZoomFPFOVPatch: SkyZoom.dll not found - "
                    "is SkyZoom installed?");
    return;
  }

  auto *proc = REX::W32::GetProcAddress(module, "SkyZoom_GetActiveZoomWeight");
  if (!proc) {
    SKSE::log::warn("SkyZoomFPFOVPatch: SkyZoom.dll found but "
                    "missing SkyZoom_GetActiveZoomWeight - update SkyZoom");
    return;
  }

  SKSE::log::info("SkyZoomFPFOVPatch: SkyZoom.dll found, bound "
                  "to SkyZoom_GetActiveZoomWeight");
  g_getActiveZoomWeight = reinterpret_cast<GetActiveZoomWeight_t>(proc);
}

void InstallNiCameraHook() {
  if (MH_Initialize() != MH_OK) {
    SKSE::log::error("SkyZoomFPFOVPatch: MH_Initialize failed");
    return;
  }

  const REL::RelocationID id{69273, 70643};
  auto *target = reinterpret_cast<void *>(id.address());
  if (!target) {
    SKSE::log::error("SkyZoomFPFOVPatch: failed to resolve NiCamera update "
                     "address - Address Library may be missing or out of date");
    return;
  }

  if (MH_CreateHook(target, reinterpret_cast<void *>(&hkNiCameraUpdate),
                    reinterpret_cast<void **>(&g_original)) != MH_OK) {
    SKSE::log::error("SkyZoomFPFOVPatch: MH_CreateHook failed for {:X}",
                     reinterpret_cast<std::uintptr_t>(target));
    return;
  }

  if (MH_EnableHook(target) != MH_OK) {
    SKSE::log::error("SkyZoomFPFOVPatch: MH_EnableHook failed");
    return;
  }

  SKSE::log::info("SkyZoomFPFOVPatch: NiCamera update hook "
                  "installed ({:X})",
                  reinterpret_cast<std::uintptr_t>(target));
}

// kDataLoaded, not Install()/SKSEPluginLoad, since load order against
// FirstPersonFOV.dll/SkyZoom.dll isn't guaranteed until then. Does nothing
// at all if FirstPersonFOV isn't installed, so a user without it never sees
// an "update SkyZoom" warning that has nothing to do with them.
void OnMessage(SKSE::MessagingInterface::Message *a_message) {
  if (a_message->type != SKSE::MessagingInterface::kDataLoaded) {
    return;
  }

  if (!REX::W32::GetModuleHandleW(L"FirstPersonFOV.dll")) {
    SKSE::log::info("SkyZoomFPFOVPatch: FirstPersonFOV not "
                    "detected - nothing to patch, doing nothing");
    return;
  }

  ResolveSkyZoomAPI();
  InstallNiCameraHook();
}
} // namespace

void Install() {
  if (auto *messaging = SKSE::GetMessagingInterface()) {
    messaging->RegisterListener(&OnMessage);
  }
}
} // namespace Hook
