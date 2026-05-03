# Dinomod Enhanced (Recompiled)
A work-in-progress recomp mod for shinx121's <b><a href="https://github.com/DinosaurPlanetRecomp/dino-recomp">Dinosaur Planet: Recompiled</a></b>.

The mod ports over the fan-made fixes included in <b>Dinomod Enhanced</b> - a community ROM patch for the <i>December 2000</i> prototype of Rare's unreleased game <b>Dinosaur Planet</b>. The patch is developed by multiple contibutors and fixes a large number of the prototype's bugs and progression-blockers, allowing much more of the game to be explored. All edits included were created for fun, for free, with a strict no-AI policy.

The bulk of <b>Dinomod Enhanced</b>'s changes work through direct asset file modifications (i.e. editing text, textures, models, object instance layouts, etc.), but it also contains many code patches which are created through hand-written modification of the game's MIPS assembly. Patches in this latter category are nontrivial to port, since it ideally involves recreating the edit in C (informed by the game's <a href="https://github.com/zestydevy/dinosaur-planet">WIP decompilation</a>). 

Most of the ROM patches' code fixes are now ported, but work is continuing on any remaining aspects. The aim is to eventually have parity with the latest ROM patch releases! A table of the remaining DLL edits <a href="https://wiki.dinosaurpla.net/wiki/Dinomod_Enhanced/Differences/DLLs">can be viewed here</a>.

For more information about <b>Dinomod Enhanced</b>, visit:
<ul>
<li><a href="http://www.dinosaurpla.net/">www.dinosaurpla.net (the Dinosaur Planet Fansite)</a></li>
<li><a href="https://discord.gg/yPDYXzYFb4">The Dinosaur Planet Community Discord</a></li>
</ul>

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
