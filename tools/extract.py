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
from assetlib.gametext import GameTextBinParser, GameTextSpecWriter
from assetlib.mpeg import MPEGList
from assetlib.objects import ObjectList
from assetlib.seq import seqs_from_bin, seqs_dump_to_dir
from assetlib.warp import WarpTab

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent

class Extractor:
    def __init__(self, extract_dir: Path, bin_dir: Path) -> None:
        self.extract_dir = extract_dir
        self.bin_dir = bin_dir

    def extract_gametext(self):
        print("Extracting GAMETEXT...")
        tab = self.bin_dir.joinpath("GAMETEXT.tab")
        bin = self.bin_dir.joinpath("GAMETEXT.bin")
        gametext_spec = self.extract_dir.joinpath("gametext.spec")

        with open(tab, "rb") as tab_file, \
            open(bin, "rb") as bin_file:
            gametext = GameTextBinParser().parse(tab_file, bin_file)
        
        with open(gametext_spec, "w", encoding="utf-8") as gametext_spec_file:
            GameTextSpecWriter().write(gametext, gametext_spec_file)

    def extract_mpeg(self):
        print("Extracting MPEG...")
        tab = self.bin_dir.joinpath("MPEG.tab")
        bin = self.bin_dir.joinpath("MPEG.bin")

        with open(tab, "rb") as tab_file, \
            open(bin, "rb") as bin_file:
            mpeg_list = MPEGList.from_binary(tab_file, bin_file)

        mpeg_dir = self.extract_dir.joinpath("MPEG")
        mpeg_dir.mkdir(exist_ok=True)

        for i, mpeg in enumerate(mpeg_list.files):
            if mpeg == None:
                continue

            mpeg_path = mpeg_dir.joinpath(f"{i:04} {i:04X}.mp3")
            with open(mpeg_path, "wb") as mpeg_file:
                mpeg_file.write(mpeg.data)

    def extract_objects(self):
        print("Extracting OBJECTS...")
        tab = self.bin_dir.joinpath("OBJECTS.tab")
        bin = self.bin_dir.joinpath("OBJECTS.bin")

        with open(tab, "rb") as tab_file, \
            open(bin, "rb") as bin_file:
            objs = ObjectList.from_binary(tab_file, bin_file)

        obj_dir = self.extract_dir.joinpath("OBJECTS")
        obj_dir.mkdir(exist_ok=True)

        for i, obj in enumerate(objs.objects):
            if obj == None:
                continue

            obj_path = obj_dir.joinpath(f"{i:04} {i:04X} {obj.name}.bin")
            with open(obj_path, "wb") as obj_file:
                obj_file.write(obj.data)

    def extract_warp(self):
        print("Extracting WARPTAB...")
        warp_yaml = self.extract_dir.joinpath("warp.yaml")
        warptab_bin = self.bin_dir.joinpath("WARPTAB.bin")

        with open(warptab_bin, "rb") as warptab_bin_file:
            warptab = WarpTab.from_binary_file(warptab_bin_file)
        
        with open(warp_yaml, "w", encoding="utf-8") as warp_yaml_file:
            warptab.write_yaml(warp_yaml_file)

    def extract_seq(self):
        print("Extracting OBJSEQ/ANIMCURVES...")
        objseq_tab = self.bin_dir.joinpath("OBJSEQ.tab")
        objseq_bin = self.bin_dir.joinpath("OBJSEQ.bin")
        objseq2curve_tab = self.bin_dir.joinpath("OBJSEQ2CURVE.tab")
        animcurves_tab = self.bin_dir.joinpath("ANIMCURVES.tab")
        animcurves_bin = self.bin_dir.joinpath("ANIMCURVES.bin")

        with open(objseq_tab, "rb") as objseq_tab_file, \
            open(objseq_bin, "rb") as objseq_bin_file, \
            open(objseq2curve_tab, "rb") as objseq2curve_tab_file, \
            open(animcurves_tab, "rb") as animcurves_tab_file, \
            open(animcurves_bin, "rb") as animcurves_bin_file:
            seqs = seqs_from_bin(
                objseq_tab_file, objseq_bin_file,
                objseq2curve_tab_file,
                animcurves_tab_file, animcurves_bin_file)

        seqs_dir = self.extract_dir.joinpath("SEQUENCES")
        seqs_dir.mkdir(exist_ok=True)

        seqs_dump_to_dir(seqs_dir, seqs)

    def extract_bin(self, rom: BufferedReader, fs: FSTab):
        self.bin_dir.mkdir(exist_ok=True)

        for i in range(len(fs.entries)):
            path = self.bin_dir.joinpath(DINO_FILENAMES[i])

            with open(path, "wb") as file:
                file.write(fs.get_file(i, rom))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--extract", type=str, help="Directory to extract to.", default="extract")
    parser.add_argument("--bin", type=str, help="The directory to dump the FST to.", default="bin")
    parser.add_argument("--rom", type=str, help="The ROM to extract.", default="baserom.z64")
    
    args = parser.parse_args()

    extract_dir = PROJECT_DIR.joinpath(args.extract)
    bin_dir = PROJECT_DIR.joinpath(args.bin)

    extract_dir.mkdir(parents=True, exist_ok=True)
    bin_dir.mkdir(parents=True, exist_ok=True)

    extractor = Extractor(extract_dir, bin_dir)

    with open(PROJECT_DIR.joinpath(args.rom), "rb") as rom:
        print(f"Extracting {args.rom}...")
        fs = FSTab.from_rom(rom)
        extractor.extract_bin(rom, fs)
    
    extractor.extract_gametext()
    extractor.extract_mpeg()
    extractor.extract_objects()
    extractor.extract_warp()
    extractor.extract_seq()

    print("Done.")

if __name__ == "__main__":
    main()
