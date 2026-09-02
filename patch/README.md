# SkyZoom Patches

This directory holds SkyZoom's optional compatibility patches, makes sense to
install if you also have the specific mod it targets.
None of them are required for SkyZoom itself to work. What form a patch
takes (an SKSE plugin DLL, an ESP tweak, an ini, ...) depends entirely on
what the targeted mod needs.

Each patch lives in its own `patch/<name>/` directory here,
versioned independently of SkyZoom and of each other via its own xmake
target (`set_default(false)`, so a plain `xmake build` never builds it) -
see each patch's own README for what it does, why it exists, and how to
build/install it.

## Patches

| Patch                                         | Targets                                   | Build                           |
| --------------------------------------------- | ----------------------------------------- | ------------------------------- |
| [Improved Camera SE][improved-camera-readme]  | [Improved Camera SE][improved-camera-se]  | `xmake build SkyZoomICPatch`    |
| [FirstPersonFOVSKSE][first-person-fov-readme] | [FirstPersonFOVSKSE][first-person-fov-se] | `xmake build SkyZoomFPFOVPatch` |

[improved-camera-readme]: improved-camera/README.md
[improved-camera-se]: https://www.nexusmods.com/skyrimspecialedition/mods/93962
[first-person-fov-readme]: first-person-fov/README.md
[first-person-fov-se]: https://www.nexusmods.com/skyrimspecialedition/mods/172417
