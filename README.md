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
  <a href="#building">Building</a> •
  <a href="#license">License</a>
</p>

---

# Overview

SkyZoom is a native [SKSE][skse] plugin for Skyrim Special Edition /
Anniversary Edition.

It drives `RE::PlayerCamera::worldFOV` and `firstPersonFOV` directly — the
same members the renderer itself reads for the scene and the first-person
viewmodel (arms/weapon) respectively — rather than touching any
camera/scene-graph memory. See [How it works](#how-it-works) below.

## Features

- **Smooth ease in/out** — a smoothstep transition (zero velocity at both
  ends) instead of an abrupt cut, so the zoom never jolts when the hotkey is
  pressed, released, or tapped quickly mid-transition.
- **Keyboard/mouse hotkey** — any virtual-key code, configurable.
- **Gamepad support** — any XInput button bitmask also triggers the zoom
  (default: D-Pad Left), independently of the keyboard/mouse hotkey.
- **Config-driven** — target FOV and transition speed are both adjustable
  without recompiling.
- **Per-view toggle** — zoom in first person only, third person only, or
  both.
- **Mouse sensitivity compensation** — optionally scales mouse sensitivity
  down while zoomed, so aim doesn't get twitchy at a narrower FOV.
- **MCM menu** — if [SkyUI][skyui] is installed, all of the above are
  configurable from an in-game menu (Mod Configuration Menu) instead of
  hand-editing `SkyZoom.ini`. Changes apply immediately, no restart needed.

## Installation

1. Install [SKSE][skse] and
   [Address Library for SKSE Plugins][address-library]. Optionally, install
   [SkyUI][skyui] for the in-game MCM menu (SkyZoom works without it,
   ini-only).
2. Copy `SkyZoom.dll` and `SkyZoom.ini` into `Data\SKSE\Plugins\` (or into
   the equivalent mod folder if using a mod manager such as Mod Organizer 2).
   This alone is enough for ini-only configuration.
3. For the MCM menu (needs SkyUI, see step 1): also copy `SkyZoom.esp` into
   `Data\`, `SkyZoom_Native.pex`/`SkyZoom_MCM.pex` into `Data\Scripts\`, and
   enable `SkyZoom.esp` in your load order.
4. Launch the game via `skse64_loader.exe`.

## Usage

Hold the configured hotkey (default: Mouse Button 4 / M4 on keyboard+mouse,
D-Pad Left on an XInput gamepad) in first- or third-person view to zoom in.
Release it to ease back to the normal FOV.

## Configuration

If [SkyUI][skyui] is installed, open the **SkyZoom** page in the MCM menu
(Options → Mod Configuration) to change any of the settings below in-game,
applied immediately.

Otherwise, edit `SkyZoom.ini` next to the DLL (created with defaults on
first launch if missing) and restart the game to apply changes:

| Key                     | Default | Description                                                                                      |
| ----------------------- | ------- | ------------------------------------------------------------------------------------------------ |
| `Hotkey`                | `5`     | Virtual-key code of the hold-to-zoom key (`5` = Mouse Button 4 / M4)                             |
| `GamepadButton`         | `4`     | XInput button bitmask that also holds-to-zoom (`4` = D-Pad Left; `0` disables)                   |
| `ZoomFOV`               | `60.0`  | FOV in degrees while the hotkey is held                                                          |
| `SmoothSpeed`           | `8.0`   | Ease-in/ease-out speed for the zoom transition (higher = snappier)                               |
| `ViewMode`              | `2`     | Which view(s) the hotkey zooms in (`0` = first person only, `1` = third person only, `2` = both) |
| `ScaleMouseSensitivity` | `1`     | Scale mouse sensitivity down while zoomed (`1` = on, `0` = off)                                  |
| `SensitivityExponent`   | `2.5`   | How aggressively sensitivity is cut while zoomed (higher = more aggressive at moderate zoom)     |

## Building

### Requirements

- [XMake 3.0.0+](https://xmake.io)
- C++23 compiler: MSVC (Build Tools for Visual Studio, no full IDE required)
  or Clang-CL

### Dependencies

`lib/commonlibsse-ng` is a git submodule, tracking
[alandtse/CommonLibSSE-NG][commonlibsse-ng], which supports both the
Skyrim 1.6.1170 and 1.7.99 runtimes. Fetch it with:

```sh
git submodule update --init --recursive
```

### Build

```sh
xmake build
```

This produces `SkyZoom.dll` under `build/windows/x64/...`.

### Building the MCM menu

`dist/SkyZoom.esp` is checked into this repo (a plugin containing one Quest -
`Start Game Enabled`, no stages/aliases - with `SkyZoom_MCM.pex` attached as
its VMAD script; built once with a tool like SSEEdit and never needs
rebuilding unless the script's class name changes). Only the `.pex` files
need recompiling when `scripts/source/*.psc` changes.

Requirements:

- `PapyrusCompiler.exe`, shipped with the Creation Kit
  (`<Skyrim install>\Papyrus Compiler\PapyrusCompiler.exe`).
- The base game's Papyrus source, under
  `<Skyrim install>\Data\Scripts\Source\Source\Scripts`.
- The [SkyUI SDK][skyui-sdk] (a separate download from the SkyUI Nexus
  page), for `SKI_ConfigBase.psc` - `SkyZoom_MCM.psc` extends it, and the
  compiler needs the source, not SkyUI's shipped `.pex`, for anything it
  extends.

Compile both scripts with all three import paths and the base game's flags
file, e.g. from PowerShell:

```powershell
$compiler = "<Skyrim install>\Papyrus Compiler\PapyrusCompiler.exe"
$baseSource = "<Skyrim install>\Data\Scripts\Source\Source\Scripts"
$skyuiSdk = "<SkyUI SDK>\Scripts\Source"
$imports = "$baseSource;$skyuiSdk;scripts\source"
$flags = "$baseSource\TESV_Papyrus_Flags.flg"

& $compiler scripts\source\SkyZoom_Native.psc `
  -i="$imports" -f="$flags" -o=build\papyrus_output
& $compiler scripts\source\SkyZoom_MCM.psc `
  -i="$imports" -f="$flags" -o=build\papyrus_output
```

Then ship the resulting `Scripts/*.pex` alongside the DLL and `SkyZoom.esp`.

### Visual Studio project (optional)

```sh
xmake project -k vsxmake
```

generates a `vsxmakeXXXX/` solution if you'd rather build/debug from the
Visual Studio IDE.

## Project Structure

```txt
├── src/
│   ├── PCH.h                    Precompiled header (CommonLibSSE-NG + STL)
│   ├── Plugin.cpp                SKSE entry point (SKSEPluginLoad) + log init
│   ├── Config.h / .cpp           SkyZoom.ini load / save / defaults
│   ├── Hooks.h / .cpp            D3D11 swapchain Present hook install
│   ├── Input.h / .cpp            Keyboard + mouse + XInput hotkey polling
│   ├── FOVController.h / .cpp    Per-frame FOV transition + worldFOV write
│   └── Papyrus.h / .cpp          Native function bridge for the MCM menu
├── scripts/source/
│   ├── SkyZoom_Native.psc        Papyrus declarations for the native bridge
│   └── SkyZoom_MCM.psc           MCM menu (extends SkyUI's SKI_ConfigBase)
├── dist/
│   ├── SkyZoom.ini                Default config, shipped alongside the DLL
│   └── SkyZoom.esp                Quest hosting the MCM menu (see below)
└── lib/commonlibsse-ng/          CommonLibSSE-NG (git submodule)
```

The compiled `Scripts/*.pex` aren't checked into this repo
(`scripts/source/*.psc` is the source of truth) - see
[Building the MCM menu](#building-the-mcm-menu) below.

## How it works

A background thread polls
`RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().renderWindows[0].swapChain`
— an Address-Library-resolved pointer into the game's actual D3D11 swapchain
— until it's non-null, then vtable-patches that swapchain's `Present` for a
per-frame callback. Each frame, while the player is in first or third person
and the hotkey (or gamepad button) is held or the transition is still easing
out, `FOVController::Update()` writes smoothstep-interpolated values
straight into `RE::PlayerCamera::worldFOV` and `firstPersonFOV` (the latter
only matters in first person, but is harmless to keep in sync in third
person too), so the scene and the first-person viewmodel (arms/weapon) zoom
together as one picture. If `ScaleMouseSensitivity` is on, the same
transition also scales `fMouseHeadingSensitivity:Controls` (SkyrimPrefs.ini)
by `(currentFOV / baseFOV) ^ SensitivityExponent`, so mouse-look doesn't
feel twitchy at a narrower FOV. See the comments in `Hooks.cpp` and
`FOVController.cpp` for more detail.

## License

This project is under the [GNU General Public License v3.0](./LICENSE).

[skse]: https://skse.silverlock.org/
[address-library]: https://www.nexusmods.com/skyrimspecialedition/mods/32444
[skyui]: https://www.nexusmods.com/skyrimspecialedition/mods/12604
[skyui-sdk]: https://www.nexusmods.com/skyrimspecialedition/mods/12604?tab=files
[commonlibsse-ng]: https://github.com/alandtse/CommonLibSSE-NG
