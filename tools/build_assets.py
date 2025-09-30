#!/usr/bin/env python3
import argparse
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys

from assetlib.mpeg import MPEGList
from assetlib.objects import ObjectList
from assetlib.warp import WarpTab
from gametext import GameTextBinParser, GameTextBinWriter, GameTextSpecParser

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent
OUTPUT_DIR = PROJECT_DIR.joinpath("dinomod_enhanced/assets")
ASSETS_DIR = PROJECT_DIR.joinpath("assets")
BIN_DIR = PROJECT_DIR.joinpath("bin")

def apply_xdelta(old_file: Path, delta_file: Path, new_file: Path):
    xdelta: str
    if sys.platform == "win32":
        xdelta = ".\\xdelta3.exe"
    else:
        xdelta = "xdelta3"
    
    args = [xdelta, "-d", "-f", "-s", str(old_file), str(delta_file), str(new_file)]

    subprocess.check_call(args)

def build_xdelta_asset(filename: str):
    print(f"Applying xdelta for {filename}...")
    apply_xdelta(
        old_file=BIN_DIR.joinpath(filename),
        delta_file=ASSETS_DIR.joinpath(f"{filename}.xdelta"),
        new_file=OUTPUT_DIR.joinpath(filename)
    )

def build_gametext():
    print("Building GAMETEXT...")
    base_tab = BIN_DIR.joinpath("GAMETEXT.tab")
    base_bin = BIN_DIR.joinpath("GAMETEXT.bin")

    patches = [p for p in ASSETS_DIR.joinpath("GAMETEXT").rglob("*.spec")]

    with open(base_tab, "rb") as base_tab_file, \
         open(base_bin, "rb") as base_bin_file:
        gametext = GameTextBinParser().parse(base_tab_file, base_bin_file)
    
    spec_parser = GameTextSpecParser()
    for patch in patches:
        with open(patch, "r", encoding="utf-8") as spec_file:
            patch_gametext = spec_parser.parse(spec_file)
        gametext.apply_patch(patch_gametext)

    new_tab = OUTPUT_DIR.joinpath("GAMETEXT.tab")
    new_bin = OUTPUT_DIR.joinpath("GAMETEXT.bin")

    with open(new_tab, "wb") as new_tab_file, \
         open(new_bin, "wb") as new_bin_file:
        GameTextBinWriter().write(gametext, new_tab_file, new_bin_file)

def build_mpeg():
    print("Building MPEG...")
    base_tab = BIN_DIR.joinpath("MPEG.tab")
    base_bin = BIN_DIR.joinpath("MPEG.bin")

    with open(base_tab, "rb") as base_tab_file, \
         open(base_bin, "rb") as base_bin_file:
        mpeg_list = MPEGList.from_binary(base_tab_file, base_bin_file)

    patch_mpegs = MPEGList.from_directory(ASSETS_DIR.joinpath("MPEG"))
    
    mpeg_list.apply_patch(patch_mpegs)

    new_tab = OUTPUT_DIR.joinpath("MPEG.tab")
    new_bin = OUTPUT_DIR.joinpath("MPEG.bin")

    with open(new_tab, "wb") as new_tab_file, \
         open(new_bin, "wb") as new_bin_file:
        mpeg_list.write_binary(new_tab_file, new_bin_file)

def build_objects():
    print("Building OBJECTS...")
    base_tab = BIN_DIR.joinpath("OBJECTS.tab")
    base_bin = BIN_DIR.joinpath("OBJECTS.bin")

    with open(base_tab, "rb") as base_tab_file, \
         open(base_bin, "rb") as base_bin_file:
        objlist = ObjectList.from_binary(base_tab_file, base_bin_file)

    # TODO: these need the magic dust DLL to be patched, otherwise these objdef changes crash!
    exclude = set([
        1223, # MagicDustSmall
        1225, # MagicDustMid
        1227, # MagicDustLarge
        1229, # MagicDustHuge
    ])
    patch_objs = ObjectList.from_directory(ASSETS_DIR.joinpath("OBJECTS"), exclude)
    
    objlist.apply_patch(patch_objs)

    new_tab = OUTPUT_DIR.joinpath("OBJECTS.tab")
    new_bin = OUTPUT_DIR.joinpath("OBJECTS.bin")

    with open(new_tab, "wb") as new_tab_file, \
         open(new_bin, "wb") as new_bin_file:
        objlist.write_binary(new_tab_file, new_bin_file)

def build_warptab():
    print("Building WARPTAB...")
    base = BIN_DIR.joinpath("WARPTAB.bin")

    patches = [p for p in ASSETS_DIR.joinpath("WARP").rglob("*.yaml")]

    with open(base, "rb") as base_file:
        warptab = WarpTab.from_binary_file(base_file)
    
    for patch in patches:
        with open(patch, "r", encoding="utf-8") as yaml_file:
            patch_warptab = WarpTab.from_yaml_file(yaml_file)
        warptab.apply_patch(patch_warptab)
    
    new = OUTPUT_DIR.joinpath("WARPTAB.bin")

    with open(new, "wb") as new_file:
        warptab.write_binary(new_file)

def main():
    os.chdir(PROJECT_DIR)

    print(f"Output directory: {OUTPUT_DIR.absolute()}")

    OUTPUT_DIR.mkdir(exist_ok=True)

    build_xdelta_asset("AMAP.bin")
    build_xdelta_asset("AMAP.tab")

    build_xdelta_asset("ANIM.bin")
    build_xdelta_asset("ANIM.tab")

    build_xdelta_asset("ANIMCURVES.bin")
    build_xdelta_asset("ANIMCURVES.tab")

    build_xdelta_asset("AUDIO.bin")

    build_xdelta_asset("BITTABLE.bin")
    
    build_xdelta_asset("BLOCKS.bin")
    build_xdelta_asset("BLOCKS.tab")

    build_xdelta_asset("CAMACTIONS.bin")
    build_xdelta_asset("ENVFXACT.bin")
    build_xdelta_asset("FONTS.bin")

    build_gametext()

    build_xdelta_asset("GLOBALMAP.bin")

    build_xdelta_asset("HITS.bin")
    build_xdelta_asset("HITS.tab")

    build_xdelta_asset("LACTIONS.bin")

    build_xdelta_asset("MAPS.bin")
    build_xdelta_asset("MAPS.tab")

    build_xdelta_asset("MODANIM.bin")
    build_xdelta_asset("MODANIM.tab")

    build_xdelta_asset("MODELS.bin")
    build_xdelta_asset("MODELS.tab")

    build_xdelta_asset("MODLINES.bin")
    build_xdelta_asset("MODLINES.tab")

    build_mpeg()
    
    build_xdelta_asset("MUSICACTIONS.bin")

    build_objects()
    build_xdelta_asset("OBJINDEX.bin")

    build_xdelta_asset("OBJSEQ.bin")
    build_xdelta_asset("OBJSEQ.tab")
    build_xdelta_asset("OBJSEQ2CURVE.tab")

    build_xdelta_asset("TABLES.bin")

    build_xdelta_asset("TEX0.bin")
    build_xdelta_asset("TEX0.tab")
    build_xdelta_asset("TEX1.bin")
    build_xdelta_asset("TEX1.tab")

    build_warptab()

    print("Done.")

if __name__ == "__main__":
    main()
