#!/usr/bin/env python3
import argparse
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys

from assetlib.warp import WarpTab
from gametext import GameTextBinParser, GameTextSpecWriter

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent
ASSETS_DIR = PROJECT_DIR.joinpath("assets")
EXTRACT_DIR = PROJECT_DIR.joinpath("extract")
DECOMP_DIR = PROJECT_DIR.joinpath("lib/dino-recomp-decomp-bridge/dinosaur-planet")
DECOMP_ASSETS_DIR = DECOMP_DIR.joinpath("bin/assets")

def extract_gametext():
    tab = DECOMP_ASSETS_DIR.joinpath("GAMETEXT_tab.bin")
    bin = DECOMP_ASSETS_DIR.joinpath("GAMETEXT.bin")
    gametext_spec = EXTRACT_DIR.joinpath("gametext.spec")

    with open(tab, "rb") as tab_file, \
         open(bin, "rb") as bin_file:
        gametext = GameTextBinParser().parse(tab_file, bin_file)
    
    with open(gametext_spec, "w") as gametext_spec_file:
        GameTextSpecWriter().write(gametext, gametext_spec_file)

def extract_warp():
    warp_yaml = EXTRACT_DIR.joinpath("warp.yaml")
    warptab_bin = DECOMP_ASSETS_DIR.joinpath("WARPTAB.bin")

    with open(warptab_bin, "rb") as warptab_bin_file:
        warptab = WarpTab.from_binary_file(warptab_bin_file)
    
    with open(warp_yaml, "w") as warp_yaml_file:
        warptab.write_yaml(warp_yaml_file)

def main():
    EXTRACT_DIR.mkdir(exist_ok=True)
    
    extract_gametext()
    extract_warp()

if __name__ == "__main__":
    main()