<p align="center">
  <img src="./.img/banner.png" alt="Banner" />
</p>

<p align="center">
  <strong>Simple hold-to-zoom feature for Skyrim SE/AE</strong>
</p>

<p align="center">
  <a href="#overview">Overview</a> •
  <a href="#features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#usage">Usage</a> •
  <a href="#configuration">Configuration</a> •
  <a href="#compatibility">Compatibility</a> •
  <a href="#building">Building</a> •
  <a href="#license">License</a>
</p>

---

# Overview

SkyZoom is a native [SKSE][skse] plugin for Skyrim SE/AE. It drives
`worldFOV` and `firstPersonFOV` directly, rather than touching the
camera/scene-graph.

## Features

- Smooth ease in/out (smoothstep, no jolt)
- Keyboard/mouse + gamepad (XInput) hotkeys, independent of each other
- Optional toggle mode (default: hold-to-zoom)
- Live zoom adjust via scroll wheel or a rebindable gamepad button
- Optional sensitivity compensation while zoomed
- Per-view toggle (first person / third person / both)
- Config via ini, or in-game MCM menu with [SkyUI][skyui]

## Installation

1. Install [SKSE][skse] and [Address Library][address-library]. Optionally
   [SkyUI][skyui] for the MCM menu.
2. Copy `SkyZoom.dll` and `SkyZoom.ini` into `Data\SKSE\Plugins\`.
3. For the MCM menu: also copy `SkyZoom.esp` into `Data\` and
   `Scripts/*.pex` into `Data\Scripts\`, then enable `SkyZoom.esp`.
4. Launch via `skse64_loader.exe`.

## Usage

Hold the hotkey (default: Mouse Button 4 / D-Pad Left) to zoom in; release
to ease back out. With `ToggleMode` on, press again instead of releasing.

## Configuration

With SkyUI: MCM menu → **SkyZoom**, applied instantly. Without it, edit
`SkyZoom.ini` and restart.

| Key                          | Default | Description                                             |
| ---------------------------- | ------- | ------------------------------------------------------- |
| `Hotkey`                     | `5`     | Hold-to-zoom key (`5` = M4)                             |
| `GamepadButton`              | `4`     | XInput hold-to-zoom button (`0` disables)               |
| `ToggleMode`                 | `0`     | Toggle instead of hold                                  |
| `ZoomFOV`                    | `60.0`  | FOV while zoomed                                        |
| `SmoothSpeed`                | `8.0`   | Ease speed (higher = snappier)                          |
| `EnableScrollZoomAdjust`     | `1`     | Live-adjust zoom via wheel/boost button                 |
| `MinZoomFOV`                 | `20.0`  | Tightest FOV reachable while adjusting                  |
| `ScrollUsesSmoothSpeed`      | `0`     | Use `SmoothSpeed` timing for scroll-zoom ease           |
| `LiveZoomBoostButton`        | `128`   | Gamepad button for live zoom boost (`0` disables)       |
| `DisableTriggerWhenSheathed` | `0`     | Free an LT/RT `GamepadButton` from block while sheathed |
| `ViewMode`                   | `2`     | `0`=1st person, `1`=3rd person, `2`=both                |
| `RequireWeaponSheathed`      | `1`     | Only zoom outside ready/fighting stance                 |
| `AllowZoomDuringDialogue`    | `0`     | Allow zoom during dialogue even if readied              |
| `ScaleMouseSensitivity`      | `1`     | Reduce look sensitivity while zoomed                    |
| `SensitivityExponent`        | `2.5`   | Aggressiveness of sensitivity cut                       |

## Compatibility

See [patch/README.md](./patch/README.md) for compatibility patches.

## Building

**Requirements:** [XMake 3.0.0+](https://xmake.io), C++23 compiler (MSVC or Clang-CL)

```sh
git submodule update --init --recursive
xmake build SkyZoom
```

### MCM Menu

`dist/SkyZoom.esp` is prebuilt and checked in. Only `scripts/source/*.psc`
needs recompiling on change, via `PapyrusCompiler.exe` (Creation Kit).

Requirements:

- The base game's Papyrus source, under
  `<Skyrim install>\Data\Scripts\Source\Source\Scripts`. The Creation Kit
  drops it as `<Skyrim install>\Data\Scripts.zip` without extracting it -
  unzip it yourself, and put its `Source\Scripts\` folder at
  `Source\Source\Scripts` above. Needed even to resolve `Quest`, the root
  of `SKI_ConfigBase`'s own extends chain.
- The [SkyUI SDK][skyui], for `SKI_ConfigBase.psc` source - `SkyZoom_MCM.psc`
  extends it, and the compiler needs source, not SkyUI's shipped `.pex`.

```powershell
$compiler = "<Skyrim install>\Papyrus Compiler\PapyrusCompiler.exe"
$baseSource = "<Skyrim install>\Data\Scripts\Source\Source\Scripts"
$imports = "$baseSource;<SkyUI SDK>\Scripts\Source;scripts\source"
$flags = "$baseSource\TESV_Papyrus_Flags.flg"

& $compiler scripts\source\SkyZoom_Native.psc -i="$imports" -f="$flags" -o=build\papyrus_output
& $compiler scripts\source\SkyZoom_MCM.psc -i="$imports" -f="$flags" -o=build\papyrus_output
```

Ship the resulting `Scripts/*.pex` alongside the DLL and `SkyZoom.esp`.

## Project Structure

```txt
├── src/
│   ├── PCH.h                    Precompiled header (CommonLibSSE-NG + STL)
│   ├── Plugin.cpp                SKSE entry point (SKSEPluginLoad) + log init
│   ├── Config.h / .cpp           SkyZoom.ini load / save / defaults
│   ├── Hooks.h / .cpp            D3D11 swapchain Present hook install
│   ├── Input.h / .cpp            Keyboard + mouse + XInput hotkey polling
│   ├── FOVController.h / .cpp    Per-frame FOV transition + worldFOV write
│   ├── CompatAPI.h / .cpp        Exported zoom-weight query for compat patch
│   └── Papyrus.h / .cpp          Native function bridge for the MCM menu
├── scripts/source/
│   ├── SkyZoom_Native.psc        Papyrus declarations for the native bridge
│   └── SkyZoom_MCM.psc           MCM menu (extends SkyUI's SKI_ConfigBase)
├── dist/
│   ├── SkyZoom.ini                Default config, shipped alongside the DLL
│   └── SkyZoom.esp                Quest hosting the MCM menu (see below)
├── patch/README.md               Compatibility patch index - code for each
│                                  one lives on its own `patch/<name>` branch
└── lib/commonlibsse-ng/          CommonLibSSE-NG (git submodule)
```

## License

This project is under the [GNU General Public License v3.0](./LICENSE).

[skse]: https://skse.silverlock.org/
[address-library]: https://www.nexusmods.com/skyrimspecialedition/mods/32444
[skyui]: https://www.nexusmods.com/skyrimspecialedition/mods/12604
