# Dinomod Enhanced

Enhancements and fixes for the December 2000 prototype of Dinosaur Planet.

This mod ports over (and expands on) the fan-made community ROM patch of the same name. The patches are developed by multiple contributors and fix a large number of the prototype's bugs and progression blockers, allowing much more of the game to be explored and even completed!

All edits included were created for fun, for free, with a strict no-AI policy.

For more information about Dinomod Enhanced, visit:
- [www.dinosaurpla.net (the Dinosaur Planet Fansite)](http://www.dinosaurpla.net/)
- [The Dinosaur Planet Community Discord](https://discord.gg/yPDYXzYFb4)

### Status
The recomp mod features nearly full parity with the latest ROM patch release. All patches relating to progression have been fully ported. A table of the remaining DLL edits [can be viewed here](https://wiki.dinosaurpla.net/wiki/Dinomod_Enhanced/Differences/DLLs).

This mod also includes many new bug fixes and enhancements not currently found in the ROM patch. The goal is to eventually keep the two projects in sync.

## Building

### Assets
1. Place an unmodified Dinosaur Planet ROM into the root of this repository as `baserom.z64` (MD5: `49f7bb346ade39d1915c22e090ffd748`).
2. Install `xdelta3`.
    - For Windows users, place `xdelta3.exe` into the root of this repository.
    - For Linux users, `xdelta3` must be on your `PATH`.
3. Install Python dependencies.
    - (optional) Create a virtual environment: `python -m venv .venv`
    - Install: `pip install -r requirements.txt`
4. Extract vanilla assets from ROM.
    - `python tools/extract.py`
5. Build patched assets.
    - `python tools/build_assets.py`
6. The directory `dinomod_enhanced/assets` will now contain all patched asset files.

### Mod
Patched assets [must be built first](#assets)!

See [BUILDING.md](BUILDING.md) for instructions on compiling the recomp mod.

## Extracting Dinomod (WIP)
The dinomod ROM can be extracted and diff'd by running the following:

```
mkdir dinomod
python tools/extract.py --extract dinomod/extract --bin dinomod/bin --rom Dinosaur_Planet_2025-01-01\ Cosmetic\ Text\ \(Nightly\).z64
python tools/diff.py
```

The directory `dinomod` will contain the extracted FST binaries as well as the individual extracted files just like the vanilla assets under `extract`.
After running `diff.py`, the directory `diff` will contain individual asset files that changed from vanilla to dinomod.
