# SkyZoom / Improved Camera SE compatibility patch

Optional. Only install this alongside `SkyZoom.dll` if you also have
[Improved Camera SE][improved-camera-se] installed. Deliberately a separate
DLL - `SkyZoom.dll` itself has no MinHook dependency and no Improved
Camera SE-specific code; this patch depends on `SkyZoom.dll` at runtime (by
name, via `GetProcAddress` - not link-time), not the other way around.

## Why this exists

Improved Camera SE re-applies its own FOV to
`RE::PlayerCamera::worldFOV`/`firstPersonFOV` every frame, from its own
per-frame camera hook. That hook runs earlier in the frame than SkyZoom's
(SkyZoom writes from a D3D `Present` hook, which is late enough that the
write only takes effect starting the _next_ frame - by which point Improved
Camera SE has already forced its own value back in). The two mods fight over
the same fields and Improved Camera SE always wins, so SkyZoom's zoom
appears to do nothing.

Improved Camera SE does have a `bEnableOverride` toggle for its own FOV
control (its in-game menu, FOV tab → "Enable FOV Override" - the field
persists the same way regardless of whether your build stores config as ini
or json). Turning that off resolves the conflict without this patch. This
patch exists for the case where you'd rather keep Improved Camera SE's FOV
override on for its other states (combat, horseback, etc.) and only want
SkyZoom's zoom specifically to win while it's active.

## How it works

Hooks the same low-level, per-frame FOV-application function Improved
Camera SE itself hooks (address published in its own MPL-licensed source,
`ImprovedCamera/source/skyrimse/Hooks.cpp`) - the last point before that
frame's FOV reaches the renderer. It reads SkyZoom's live zoom progress
through `SkyZoom_GetActiveZoomWeight`, an exported function resolved by
name via `GetProcAddress` (not linked against `SkyZoom.dll`, so this patch
degrades to a no-op (logged) rather than failing to load if SkyZoom isn't
present), and _blends_ the FOV Improved Camera SE was about to apply this
frame toward SkyZoom's target by that weight (0 = untouched, 1 = fully
SkyZoom's target), rather than substituting SkyZoom's value outright.

Blending instead of substituting matters because Improved Camera SE can
keep moving its own desired FOV around independently of SkyZoom (e.g. some
movement-driven effect) - substituting a flat value the whole time SkyZoom
is "active" hid that while mid-zoom (SkyZoom's value dominates deep into
the blend either way) but produced a visible jump right as the zoom started
or finished, where the blend weight is near 0 and Improved Camera SE's own
live value should still be showing through almost entirely.

Only the FOV parameters are touched; Improved Camera SE's own hook on the
same function only touches near-distance, so the two hooks chain without
conflict regardless of load order.

All of the above only happens if Improved Camera SE is actually detected
(`ImprovedCameraSE.dll` loaded) - if it isn't, this patch hooks nothing at
all and does nothing, so it's harmless to have installed by mistake without
IC. This check (and the `SkyZoom.dll` lookup) runs once, on SKSE's
`kDataLoaded` message, and logs exactly one of these to
`SkyZoomICPatch.log`:

- `Improved Camera SE not detected - nothing to patch, doing nothing`
- `SkyZoom.dll not found - is SkyZoom installed?`
- `SkyZoom.dll found but missing SkyZoom_GetActiveZoomWeight - update SkyZoom`
- `SkyZoom.dll found, bound to SkyZoom_GetActiveZoomWeight` (the working case)

## Installation

Copy `SkyZoomICPatch.dll` into `Data\SKSE\Plugins\` alongside
`SkyZoom.dll` and `ImprovedCameraSE.dll`.

## Caveats

This hooks an internal, unversioned Skyrim function via its Address Library
ID rather than any documented/stable interface - the same risk profile
Improved Camera SE's own hook on that function carries. If a future
Skyrim/Address Library update shifts that ID, this patch will fail to
resolve the hook address and log an error (`SkyZoomICPatch.log`
next to SkyZoom's own log) rather than misbehave, but the underlying
conflict with Improved Camera SE would return until it's updated.

[improved-camera-se]: https://www.nexusmods.com/skyrimspecialedition/mods/93962
