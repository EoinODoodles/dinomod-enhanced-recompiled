#!/usr/bin/env python3
import argparse
import glob
from io import BufferedReader
import os
from pathlib import Path
import shutil
import subprocess
import sys

from assetlib.fs import FSTab, DINO_FILENAMES
from assetlib.mpeg import MPEGList
from assetlib.objects import ObjectList
from assetlib.warp import WarpTab
from gametext import GameTextBinParser, GameTextSpecWriter

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent
EXTRACT_DIR = PROJECT_DIR.joinpath("extract")
BIN_DIR = PROJECT_DIR.joinpath("bin")

def extract_gametext():
    print("Extracting GAMETEXT...")
    tab = BIN_DIR.joinpath("GAMETEXT.tab")
    bin = BIN_DIR.joinpath("GAMETEXT.bin")
    gametext_spec = EXTRACT_DIR.joinpath("gametext.spec")

    with open(tab, "rb") as tab_file, \
         open(bin, "rb") as bin_file:
        gametext = GameTextBinParser().parse(tab_file, bin_file)
    
    with open(gametext_spec, "w") as gametext_spec_file:
        GameTextSpecWriter().write(gametext, gametext_spec_file)

def extract_mpeg():
    print("Extracting MPEG...")
    tab = BIN_DIR.joinpath("MPEG.tab")
    bin = BIN_DIR.joinpath("MPEG.bin")

    with open(tab, "rb") as tab_file, \
         open(bin, "rb") as bin_file:
        mpeg_list = MPEGList.from_binary(tab_file, bin_file)

    mpeg_dir = EXTRACT_DIR.joinpath("MPEG")
    mpeg_dir.mkdir(exist_ok=True)

    for i, mpeg in enumerate(mpeg_list.files):
        if mpeg == None:
            continue

        mpeg_path = mpeg_dir.joinpath(f"{i:04} {i:04X}.mp3")
        with open(mpeg_path, "wb") as mpeg_file:
            mpeg_file.write(mpeg.data)

def extract_objects():
    print("Extracting OBJECTS...")
    tab = BIN_DIR.joinpath("OBJECTS.tab")
    bin = BIN_DIR.joinpath("OBJECTS.bin")

    with open(tab, "rb") as tab_file, \
         open(bin, "rb") as bin_file:
        objs = ObjectList.from_binary(tab_file, bin_file)

    obj_dir = EXTRACT_DIR.joinpath("OBJECTS")
    obj_dir.mkdir(exist_ok=True)

    for i, obj in enumerate(objs.objects):
        if obj == None:
            continue

        obj_path = obj_dir.joinpath(f"{i:04} {i:04X} {obj.name}.bin")
        with open(obj_path, "wb") as obj_file:
            obj_file.write(obj.data)

def extract_warp():
    print("Extracting WARPTAB...")
    warp_yaml = EXTRACT_DIR.joinpath("warp.yaml")
    warptab_bin = BIN_DIR.joinpath("WARPTAB.bin")

    with open(warptab_bin, "rb") as warptab_bin_file:
        warptab = WarpTab.from_binary_file(warptab_bin_file)
    
    with open(warp_yaml, "w") as warp_yaml_file:
        warptab.write_yaml(warp_yaml_file)

def extract_bin(rom: BufferedReader, fs: FSTab):
    print("Extracting baserom...")
    BIN_DIR.mkdir(exist_ok=True)

    for i in range(len(fs.entries)):
        path = BIN_DIR.joinpath(DINO_FILENAMES[i])

        with open(path, "wb") as file:
            file.write(fs.get_file(i, rom))

def main():
    EXTRACT_DIR.mkdir(exist_ok=True)

    with open(PROJECT_DIR.joinpath("baserom.z64"), "rb") as rom:
        fs = FSTab.from_rom(rom)
        extract_bin(rom, fs)
    
    extract_gametext()
    extract_mpeg()
    extract_objects()
    extract_warp()

    print("Done.")

if __name__ == "__main__":
    main()
