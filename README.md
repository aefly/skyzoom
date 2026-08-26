<p align="center">
  <img src="./.img/banner.png" alt="Banner" />
</p>

<p align="center"><strong>Simple hold-to-zoom feature for Skyrim SE/AE</strong></p>

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

SkyZoom is a native [SKSE](https://skse.silverlock.org/) plugin for
Skyrim Special Edition / Anniversary Edition.

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
- **MCM menu** — if [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)
  is installed, all of the above are configurable from an in-game menu
  (Mod Configuration Menu) instead of hand-editing `SkyZoom.ini`. Changes
  apply immediately, no restart needed.

## Installation

1. Install [SKSE](https://skse.silverlock.org/) and
   [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444).
   Optionally, install [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)
   for the in-game MCM menu (SkyZoom works without it, ini-only).
2. Copy `SkyZoom.dll` and `SkyZoom.ini` into `Data\SKSE\Plugins\` (or into
   the equivalent mod folder if using a mod manager such as Mod Organizer 2).
   This alone is enough for ini-only configuration.
3. For the MCM menu (needs SkyUI, see step 1): also copy `SkyZoom.esp` into
   `Data\`, `SkyZoom_Native.pex`/`SkyZoom_MCM.pex` into `Data\Scripts\`, and
   enable `SkyZoom.esp` in your load order.
4. Launch the game via `skse64_loader.exe`.

## Usage

Hold the configured hotkey (default: Mouse Button 4 / M4 on keyboard+mouse,
D-Pad Left on an XInput gamepad) in first-person view to zoom in. Release it
to ease back to the normal FOV.

## Configuration

If [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604) is
installed, open the **SkyZoom** page in the MCM menu (Options → Mod
Configuration) to change any of the settings below in-game, applied
immediately.

Otherwise, edit `SkyZoom.ini` next to the DLL (created with defaults on
first launch if missing) and restart the game to apply changes:

| Key             | Default | Description                                                                    |
| --------------- | ------- | ------------------------------------------------------------------------------ |
| `Hotkey`        | `5`     | Virtual-key code of the hold-to-zoom key (`5` = Mouse Button 4 / M4)           |
| `GamepadButton` | `4`     | XInput button bitmask that also holds-to-zoom (`4` = D-Pad Left; `0` disables) |
| `ZoomFOV`       | `60.0`  | FOV in degrees while the hotkey is held                                        |
| `SmoothSpeed`   | `8.0`   | Ease-in/ease-out speed for the zoom transition (higher = snappier)             |

## Building

### Requirements

- [XMake 3.0.0+](https://xmake.io)
- C++23 compiler: MSVC (Build Tools for Visual Studio, no full IDE required)
  or Clang-CL

### Dependencies

`lib/commonlibsse-ng` is a git submodule. Fetch it with:

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

Compile `scripts/source/*.psc` against those import paths with
`PapyrusCompiler.exe`, then ship the resulting `Scripts/*.pex` alongside the
DLL and `SkyZoom.esp`.

### Visual Studio project (optional)

```sh
xmake project -k vsxmake
```

generates a `vsxmakeXXXX/` solution if you'd rather build/debug from the
Visual Studio IDE.

## Project Structure

```txt
├── src/
│   ├── PCH.h                    Precompiled header (CommonLibSSE-NG + STL includes)
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
`RE::BSGraphics::Renderer::GetRendererData()->renderWindows[0].swapChain` —
an Address-Library-resolved pointer into the game's actual D3D11 swapchain —
until it's non-null, then vtable-patches that swapchain's `Present` for a
per-frame callback. Each frame, while the player is in first person and the
hotkey (or gamepad button) is held or the transition is still easing out,
`FOVController::Update()` writes smoothstep-interpolated values straight
into `RE::PlayerCamera::worldFOV` and `firstPersonFOV`, so the scene and the
viewmodel (arms/weapon) zoom together as one picture. See the comments in
`Hooks.cpp` and `FOVController.cpp` for more detail.

## License

This project is under the [MIT License](./LICENSE).
