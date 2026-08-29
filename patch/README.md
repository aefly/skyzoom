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

| Patch                                             | Targets                                  | Branch                  |
| ------------------------------------------------- | ---------------------------------------- | ----------------------- |
| [Improved Camera SE](./improved-camera/README.md) | [Improved Camera SE][improved-camera-se] | `patch/improved-camera` |

[improved-camera-se]: https://www.nexusmods.com/skyrimspecialedition/mods/93962
