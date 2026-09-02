# SkyZoom / Improved Camera SE Compatibility Patch

Optional. Only needed alongside [Improved Camera SE][improved-camera-se] -
a separate DLL, depending on `SkyZoom.dll` only via a runtime
`GetProcAddress` lookup (no link-time dependency, no MinHook in SkyZoom
itself).

## Why

Improved Camera SE re-applies its own FOV every frame, later than SkyZoom's
D3D `Present` hook - it always wins, so SkyZoom's zoom appears to do
nothing. Improved Camera SE's own `bEnableOverride` toggle (FOV tab →
"Enable FOV Override") fixes this too, without the patch - use this
instead if you'd rather keep that override on for its other states
(combat, horseback, etc.) and only want SkyZoom's zoom to win while it's
active.

## How it works

Hooks the same low-level, per-frame FOV-apply function Improved Camera SE
itself hooks (address published in its own MPL-licensed source) - the last
point before the frame's FOV reaches the renderer. Reads SkyZoom's live
zoom weight via `SkyZoom_GetActiveZoomWeight` and blends toward it (not a
flat substitution, to avoid a visible snap right at the zoom's start/end).
Only the FOV fields are touched, so the two hooks chain without conflict
regardless of load order.

Improved Camera SE also writes FOV directly from a second function,
`CAMERA::Manager::UpdateFOV`, that the hook above can't see. This patch
hooks that too, at a fixed offset found via Improved Camera SE's PDB (it
isn't on Address Library, so there's no version-independent way to locate
it) - if a future build moves it, that hook just fails to install (logged)
instead of crashing.

No-ops if Improved Camera SE or SkyZoom aren't found; logs exactly which
case to `SkyZoomICPatch.log`.

## Installation

Copy `SkyZoomICPatch.dll` into `Data\SKSE\Plugins\` alongside
`SkyZoom.dll` and `ImprovedCameraSE.dll`.

## Caveats

Both hooks resolve unversioned addresses with no stable interface - the
NiCamera hook via Address Library ID, `UpdateFOV` via a fixed offset with
no version check at all (no public source to verify it against for the
current Discord build). If either shifts, that hook logs an error and does
nothing instead of misbehaving, but the underlying conflict returns until
it's updated here.

[improved-camera-se]: https://www.nexusmods.com/skyrimspecialedition/mods/93962
