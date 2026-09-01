# SkyZoom / FirstPersonFOVSKSE compatibility patch

Optional. Only install this alongside `SkyZoom.dll` if you also have
[FirstPersonFOVSKSE][first-person-fov-se] installed. Deliberately a
separate DLL - `SkyZoom.dll` itself has no MinHook dependency and no
FirstPersonFOVSKSE-specific code; this patch depends on `SkyZoom.dll` at
runtime (by name, via `GetProcAddress` - not link-time), not the other way
around.

## Why this exists

FirstPersonFOVSKSE re-applies its own FOV to
`RE::PlayerCamera::worldFOV`/`firstPersonFOV` every frame, from its own
per-frame camera hook (a `call`-site patch on `PlayerCamera::Update`, not a
detour - see its own `FOVPatch.h`). That hook runs earlier in the frame
than SkyZoom's (SkyZoom writes from a D3D `Present` hook, which is late
enough that the write only takes effect starting the _next_ frame - by
which point FirstPersonFOVSKSE has already forced its own value back in).
The two mods fight over the same fields and FirstPersonFOVSKSE always wins,
so SkyZoom's zoom appears to do nothing.

FirstPersonFOVSKSE does have a "Enable FOV Override" toggle in its own
in-game menu (requires SKSEMenuFramework) and a matching
`bEnableFOVOverride` in `FirstPersonFOV.ini`. Turning that off resolves the
conflict without this patch. This patch exists for the case where you'd
rather keep FirstPersonFOVSKSE's FOV override on (for its hands/world FOV
split and third-person override) and only want SkyZoom's zoom specifically
to win while it's active.

## How it works

Hooks the same low-level, per-frame FOV-application function the [Improved
Camera SE patch](../improved-camera/README.md) hooks (address published in
its MPL-licensed source, `ImprovedCamera/source/skyrimse/Hooks.cpp`) - the
last point before that frame's FOV reaches the renderer, regardless of
which mod wrote the value upstream. It reads SkyZoom's live zoom progress
through `SkyZoom_GetActiveZoomWeight`, an exported function resolved by
name via `GetProcAddress` (not linked against `SkyZoom.dll`, so this patch
degrades to a no-op (logged) rather than failing to load if SkyZoom isn't
present), and _blends_ the FOV that reached this point toward SkyZoom's
target by that weight (0 = untouched, 1 = fully SkyZoom's target), rather
than substituting SkyZoom's value outright.

Blending instead of substituting matters because FirstPersonFOVSKSE can
keep moving its own desired FOV around independently of SkyZoom (e.g. its
dialogue/inventory exit lerp) - substituting a flat value the whole time
SkyZoom is "active" would hide that mid-zoom (SkyZoom's value dominates
deep into the blend either way) but produce a visible jump right as the
zoom started or finished, where the blend weight is near 0 and
FirstPersonFOVSKSE's own live value should still be showing through almost
entirely.

Unlike the Improved Camera SE patch, this one doesn't hook the same address
FirstPersonFOVSKSE itself patches (that's a separate, earlier function) -
it only reads the _result_ of FirstPersonFOVSKSE's write, downstream. So
there's no hook chaining to worry about and install order against
FirstPersonFOVSKSE doesn't matter.

All of the above only happens if FirstPersonFOVSKSE is actually detected
(`FirstPersonFOV.dll` loaded) - if it isn't, this patch hooks nothing at
all and does nothing, so it's harmless to have installed by mistake without
it. This check (and the `SkyZoom.dll` lookup) runs once, on SKSE's
`kDataLoaded` message, and logs exactly one of these to
`SkyZoomFPFOVPatch.log`:

- `FirstPersonFOV not detected - nothing to patch, doing nothing`
- `SkyZoom.dll not found - is SkyZoom installed?`
- `SkyZoom.dll found but missing SkyZoom_GetActiveZoomWeight - update SkyZoom`
- `SkyZoom.dll found, bound to SkyZoom_GetActiveZoomWeight` (the working case)

## Installation

Copy `SkyZoomFPFOVPatch.dll` into `Data\SKSE\Plugins\` alongside
`SkyZoom.dll` and `FirstPersonFOV.dll`.

## Caveats

This hooks an internal, unversioned Skyrim function via its Address Library
ID rather than any documented/stable interface - the same risk profile the
Improved Camera SE patch's hook on that function carries. If a future
Skyrim/Address Library update shifts that ID, this patch will fail to
resolve the hook address and log an error (`SkyZoomFPFOVPatch.log` next to
SkyZoom's own log) rather than misbehave, but the underlying conflict with
FirstPersonFOVSKSE would return until it's updated.

[first-person-fov-se]: https://www.nexusmods.com/skyrimspecialedition/mods/172417
