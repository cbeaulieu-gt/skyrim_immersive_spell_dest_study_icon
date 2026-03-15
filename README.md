# DIII Dest Imm Spell Learning Icon (SKSE Plugin)

This workspace is a starter scaffold for a Skyrim SKSE plugin using CommonLibSSE-NG, with an optional Papyrus bridge.

## What is included

- SKSE plugin entrypoint and logging setup
- Messaging event hook (`kDataLoaded`)
- Papyrus registration function
- Example Papyrus script exposing a native function
- CMake + vcpkg manifest setup for CommonLibSSE-NG

## Prerequisites

- Visual Studio 2026 / VS 18 Build Tools (Desktop development with C++)
- CMake 3.21+
- vcpkg (already installed and available on your machine)
- Skyrim SE/AE with SKSE for runtime testing
- Address Library for SKSE Plugins

## Configure

```powershell
cmake --preset ninja-vcpkg-debug
# or
cmake --preset ninja-vcpkg-release
```

This project is manifest mode (`vcpkg.json`) and expects vcpkg to resolve `commonlibsse-ng`.

The preset uses `$env:VCPKG_ROOT` for `CMAKE_TOOLCHAIN_FILE`.

If your `commonlibsse-ng` port lives in the Color-Glass custom registry, set a real commit SHA in `vcpkg-configuration.json` for the registry baseline before configuring.

If you want automatic copy after build, set `SKYRIM_MODS_PATH` first:

```powershell
$env:SKYRIM_MODS_PATH = "I:/games/skyrim/mods/diii_dest_imm_spell_learning_icon"
```

## Build

```powershell
cmake --build --preset build-ninja-vcpkg-debug
# or
cmake --build --preset build-ninja-vcpkg-release
```

## VS Code IntelliSense notes

This workspace uses CMake Tools as the C/C++ configuration provider.
Use one of the configure presets (`ninja-vcpkg-debug` or `ninja-vcpkg-release`) so IntelliSense reads the same include paths and defines as the actual build.
Build artifacts are generated under `build/`.

## VS Code build tasks

Use Run Task with one of the following labels:

- Configure (Debug)
- Build (Debug)
- Configure (Release)
- Build (Release)

The plugin DLL is produced in the build output and optionally copied to:

- `SKSE/Plugins/<plugin dll>` under `SKYRIM_MODS_PATH`

## Papyrus integration notes

The sample script is in `scripts/source/DIII_SpellLearningBridge.psc` and expects a native function:

- `MarkSpellLearnedIcon(Form akSpellOrBook)`

Compile this script with your Papyrus compiler setup and include it in your mod package.

## Next implementation step

Implement game logic in `src/PapyrusBridge.cpp`:

- Resolve relevant forms safely
- Guard against null pointers and incorrect form types
- Keep heavy work out of frequently-called callbacks
- Log key transitions for field debugging

## Compatibility strategy

- Keep plugin scope narrow and avoid invasive hooks unless required
- Prefer event-driven logic and form checks over broad polling
- Validate behavior with common UI mods and HUD/icon frameworks
