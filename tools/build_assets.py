#!/usr/bin/env python3
import argparse
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys

from assetlib.warp import WarpTab
from gametext import GameTextBinParser, GameTextBinWriter, GameTextSpecParser

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent
BUILD_DIR = PROJECT_DIR.joinpath("build")
ASSETS_DIR = PROJECT_DIR.joinpath("assets")
DECOMP_DIR = PROJECT_DIR.joinpath("lib/dino-recomp-decomp-bridge/dinosaur-planet")
DECOMP_ASSETS_DIR = DECOMP_DIR.joinpath("bin/assets")

def build_gametext():
    base_tab = DECOMP_ASSETS_DIR.joinpath("GAMETEXT_tab.bin")
    base_bin = DECOMP_ASSETS_DIR.joinpath("GAMETEXT.bin")

    patches = [p for p in ASSETS_DIR.joinpath("GAMETEXT").rglob("*.spec")]

    with open(base_tab, "rb") as base_tab_file, \
         open(base_bin, "rb") as base_bin_file:
        gametext = GameTextBinParser().parse(base_tab_file, base_bin_file)
    
    spec_parser = GameTextSpecParser()
    for patch in patches:
        with open(patch, "r") as spec_file:
            patch_gametext = spec_parser.parse(spec_file)
        gametext.apply_patch(patch_gametext)

    new_tab = BUILD_DIR.joinpath("GAMETEXT.tab")
    new_bin = BUILD_DIR.joinpath("GAMETEXT.bin")

    with open(new_tab, "wb") as new_tab_file, \
         open(new_bin, "wb") as new_bin_file:
        GameTextBinWriter().write(gametext, new_tab_file, new_bin_file)

def build_warptab():
    base = DECOMP_ASSETS_DIR.joinpath("WARPTAB.bin")

    patches = [p for p in ASSETS_DIR.joinpath("WARP").rglob("*.yaml")]

    with open(base, "rb") as base_file:
        warptab = WarpTab.from_binary_file(base_file)
    
    for patch in patches:
        with open(patch, "r") as yaml_file:
            patch_warptab = WarpTab.from_yaml_file(yaml_file)
        warptab.apply_patch(patch_warptab)
    
    new = BUILD_DIR.joinpath("WARPTAB.bin")

    with open(new, "wb") as new_file:
        warptab.write_binary(new_file)

def main():
    BUILD_DIR.mkdir(exist_ok=True)
    build_gametext()
    build_warptab()

if __name__ == "__main__":
    main()
