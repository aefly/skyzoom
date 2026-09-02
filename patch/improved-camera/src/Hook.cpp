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
using UpdateFOV_t = void(__fastcall *)(void *, RE::PlayerCamera *);

NiCameraUpdate_t g_original = nullptr;
GetActiveZoomWeight_t g_getActiveZoomWeight = nullptr;
UpdateFOV_t g_originalUpdateFOV = nullptr;

// Same low-level FOV function IC hooks (NiCamera_Update) - blends toward
// SkyZoom's target instead of substituting outright, since IC keeps moving
// its own FOV independently (substituting snapped at zoom start/end).
void __cdecl hkNiCameraUpdate(RE::NiCamera *a_camera, float a_fov, float a_near,
                              float a_far, std::uint32_t a_screenWidth,
                              std::uint32_t a_screenHeight, std::uint8_t a_unk7,
                              std::uint8_t a_unk8, float a_fov2) {
  float weight = 0.0f;
  float targetFOV = 0.0f;
  const bool active =
      g_getActiveZoomWeight && g_getActiveZoomWeight(&weight, &targetFOV);
  if (active) {
    a_fov += (targetFOV - a_fov) * weight;
    a_fov2 += (targetFOV - a_fov2) * weight;
  }

  g_original(a_camera, a_fov, a_near, a_far, a_screenWidth, a_screenHeight,
             a_unk7, a_unk8, a_fov2);
}

// CAMERA::Manager::UpdateFOV (RVA 0x3B530) - writes worldFOV/firstPersonFOV
// directly every frame, stomping SkyZoom's write instantly. The NiCamera
// hook's blend can't reach this since it's a direct member write, not a
// call.
void __fastcall hkUpdateFOV(void *a_this, RE::PlayerCamera *a_camera) {
  g_originalUpdateFOV(a_this, a_camera);

  float weight = 0.0f;
  float targetFOV = 0.0f;
  if (!g_getActiveZoomWeight || !g_getActiveZoomWeight(&weight, &targetFOV)) {
    return;
  }

  a_camera->worldFOV += (targetFOV - a_camera->worldFOV) * weight;
  a_camera->firstPersonFOV += (targetFOV - a_camera->firstPersonFOV) * weight;
}

// Resolved by name, not linked, so this degrades to a no-op instead of
// failing to load if SkyZoom isn't present or is too old to export it.
void ResolveSkyZoomAPI() noexcept {
  auto *module = REX::W32::GetModuleHandleW(L"SkyZoom.dll");
  if (!module) {
    SKSE::log::warn("SkyZoomICPatch: SkyZoom.dll not found - "
                    "is SkyZoom installed?");
    return;
  }

  auto *proc = REX::W32::GetProcAddress(module, "SkyZoom_GetActiveZoomWeight");
  if (!proc) {
    SKSE::log::warn("SkyZoomICPatch: SkyZoom.dll found but "
                    "missing SkyZoom_GetActiveZoomWeight - update SkyZoom");
    return;
  }

  SKSE::log::info("SkyZoomICPatch: SkyZoom.dll found, bound "
                  "to SkyZoom_GetActiveZoomWeight");
  g_getActiveZoomWeight = reinterpret_cast<GetActiveZoomWeight_t>(proc);
}

void InstallNiCameraHook() {
  if (MH_Initialize() != MH_OK) {
    SKSE::log::error("SkyZoomICPatch: MH_Initialize failed");
    return;
  }

  const REL::RelocationID id{69273, 70643};
  auto *target = reinterpret_cast<void *>(id.address());
  if (!target) {
    SKSE::log::error("SkyZoomICPatch: failed to resolve NiCamera update "
                     "address - Address Library may be missing or out of date");
    return;
  }

  if (MH_CreateHook(target, reinterpret_cast<void *>(&hkNiCameraUpdate),
                    reinterpret_cast<void **>(&g_original)) != MH_OK) {
    SKSE::log::error("SkyZoomICPatch: MH_CreateHook failed for {:X}",
                     reinterpret_cast<std::uintptr_t>(target));
    return;
  }

  if (MH_EnableHook(target) != MH_OK) {
    SKSE::log::error("SkyZoomICPatch: MH_EnableHook failed");
    return;
  }

  SKSE::log::info("SkyZoomICPatch: NiCamera update hook "
                  "installed ({:X})",
                  reinterpret_cast<std::uintptr_t>(target));
}

// kUpdateFOVRVA was resolved from ImprovedCameraSE.dll PDB -
// Improved Camera isn't on Address Library, so there's no version-
// independent way to find it. Confirmed stable across the NexusMods and
// Discord builds tested so far; if a future build moves it, this hook
// install just fails (logged) rather than crashing.
void InstallUpdateFOVHook(void *a_module) {
  constexpr std::uintptr_t kUpdateFOVRVA = 0x3B530;
  auto *target = reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(a_module) + kUpdateFOVRVA);

  if (MH_CreateHook(target, reinterpret_cast<void *>(&hkUpdateFOV),
                    reinterpret_cast<void **>(&g_originalUpdateFOV)) != MH_OK) {
    SKSE::log::error("SkyZoomICPatch: MH_CreateHook failed for UpdateFOV "
                     "{:X}",
                     reinterpret_cast<std::uintptr_t>(target));
    return;
  }

  if (MH_EnableHook(target) != MH_OK) {
    SKSE::log::error("SkyZoomICPatch: MH_EnableHook failed for UpdateFOV");
    return;
  }

  SKSE::log::info("SkyZoomICPatch: UpdateFOV hook installed ({:X})",
                  reinterpret_cast<std::uintptr_t>(target));
}

// kDataLoaded, not Install()/SKSEPluginLoad, since load order against
// ImprovedCameraSE.dll/SkyZoom.dll isn't guaranteed until then. Does
// nothing at all if IC isn't installed, so a user without IC never sees an
// "update SkyZoom" warning that has nothing to do with them.
void OnMessage(SKSE::MessagingInterface::Message *a_message) {
  if (a_message->type != SKSE::MessagingInterface::kDataLoaded) {
    return;
  }

  auto *icModule = REX::W32::GetModuleHandleW(L"ImprovedCameraSE.dll");
  if (!icModule) {
    SKSE::log::info("SkyZoomICPatch: Improved Camera SE not "
                    "detected - nothing to patch, doing nothing");
    return;
  }

  ResolveSkyZoomAPI();
  InstallNiCameraHook();
  InstallUpdateFOVHook(icModule);
}
} // namespace

void Install() {
  if (auto *messaging = SKSE::GetMessagingInterface()) {
    messaging->RegisterListener(&OnMessage);
  }
}
} // namespace Hook
