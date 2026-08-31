# SkyZoom patches

This directory holds SkyZoom's optional compatibility patches - each one
only makes sense to install if you also have the specific mod it targets.
None of them are required for SkyZoom itself to work. What form a patch
takes (an SKSE plugin DLL, an ESP tweak, an ini, ...) depends entirely on
what the targeted mod needs; nothing here assumes DLL.

Each patch lives on its own branch (`patch/<name>`), versioned
independently of SkyZoom and of each other - see each patch's own README
for what it does, why it exists, and how to build/install it.

## Patches

| Patch                                         | Targets                                   | Branch                     |
| --------------------------------------------- | ----------------------------------------- | -------------------------- |
| [Improved Camera SE][improved-camera-readme]  | [Improved Camera SE][improved-camera-se]  | `patch/improved-camera`    |
| [FirstPersonFOVSKSE][first-person-fov-readme] | [FirstPersonFOVSKSE][first-person-fov-se] | `patch/FirstPersonFOVSKSE` |

Each README link above points at that patch's own branch, not this one -
that's where its actual code lives.

[improved-camera-readme]: https://github.com/aefly/skyzoom/blob/patch/improved-camera/patch/improved-camera/README.md
[improved-camera-se]: https://www.nexusmods.com/skyrimspecialedition/mods/93962
[first-person-fov-readme]: https://github.com/aefly/skyzoom/blob/patch/FirstPersonFOVSKSE/patch/first-person-fov/README.md
[first-person-fov-se]: https://www.nexusmods.com/skyrimspecialedition/mods/172417
