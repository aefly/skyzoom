# SkyZoom / FirstPersonFOVSKSE Compatibility Patch

Optional. Only needed alongside [FirstPersonFOVSKSE][first-person-fov-se] -
a separate DLL, depending on `SkyZoom.dll` only via a runtime
`GetProcAddress` lookup (no link-time dependency, no MinHook in SkyZoom
itself).

## Why

FirstPersonFOVSKSE re-applies its own FOV every frame, later than SkyZoom's
D3D `Present` hook - it always wins, so SkyZoom's zoom appears to do
nothing. FirstPersonFOVSKSE's own "Enable FOV Override" toggle (needs
SKSEMenuFramework) fixes this too, without the patch - use this instead if
you'd rather keep that override on and only want SkyZoom's zoom to win
while it's active.

## How it works

Hooks the same low-level, per-frame FOV-apply call the [Improved Camera SE
patch](../improved-camera/README.md) hooks - the last point before the
frame's FOV reaches the renderer, regardless of who wrote it upstream.
Reads SkyZoom's live zoom weight via `SkyZoom_GetActiveZoomWeight` and
blends toward it (not a flat substitution, to avoid a visible snap right at
the zoom's start/end). No-ops if FirstPersonFOV or SkyZoom aren't found;
logs exactly which case to `SkyZoomFPFOVPatch.log`.

## Installation

Copy `SkyZoomFPFOVPatch.dll` into `Data\SKSE\Plugins\` alongside
`SkyZoom.dll` and `FirstPersonFOV.dll`.

## Caveats

Hooks an internal, unversioned Skyrim function via its Address Library ID -
same risk as the Improved Camera SE patch's hook on that function. If a
future update shifts it, this patch logs an error and does nothing instead
of misbehaving.

[first-person-fov-se]: https://www.nexusmods.com/skyrimspecialedition/mods/172417
