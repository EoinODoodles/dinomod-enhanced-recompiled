#!/usr/bin/env python3
from __future__ import annotations
import argparse
from io import BufferedReader, BufferedWriter
from typing import TextIO

from assetlib.gametext import *

def bin2spec(tab: BufferedReader, bin: BufferedReader, output: TextIO):
    gametext = GameTextBinParser().parse(tab, bin)
    GameTextSpecWriter().write(gametext, output)

def spec2bin(spec: list[TextIO], tab: BufferedWriter, bin: BufferedWriter):
    gametexts = [GameTextSpecParser().parse(s) for s in spec]
    
    gametext = gametexts[0]
    for i in range(1, len(gametexts)):
        gametext.apply_patch(gametexts[i])
    
    GameTextBinWriter().write(gametext, tab, bin)

def mkpatch(tab1: BufferedReader, bin1: BufferedReader, tab2: BufferedReader, bin2: BufferedReader, output: TextIO):
    patch = gametext_mkpatch(tab1, bin1, tab2, bin2)
    GameTextSpecWriter().write(patch, output)

def main():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(dest="command", required=True)

    bin2spec_cmd = subparsers.add_parser("bin2spec", help="Convert GAMETEXT.bin/tab to gametext.spec.")
    bin2spec_cmd.add_argument("--tab", type=argparse.FileType("rb"), help="The GAMETEXT.tab file to read.", required=True)
    bin2spec_cmd.add_argument("--bin", type=argparse.FileType("rb"), help="The GAMETEXT.bin file to read.", required=True)
    bin2spec_cmd.add_argument("-o", "--output", type=argparse.FileType("w", encoding="utf-8"), help="The spec file to write.", required=True)

    spec2bin_cmd = subparsers.add_parser("spec2bin", help="Convert gametext.spec files to GAMETEXT.bin/tab.")
    spec2bin_cmd.add_argument("spec", nargs="+", action="extend", type=argparse.FileType("r", encoding="utf-8"), 
                              help="The spec files to read. Later files will be merged onto earlier files.")
    spec2bin_cmd.add_argument("--tab", type=argparse.FileType("wb"), help="The GAMETEXT.tab file to write.", required=True)
    spec2bin_cmd.add_argument("--bin", type=argparse.FileType("wb"), help="The GAMETEXT.bin file to write.", required=True)

    mkpatch_cmd = subparsers.add_parser("mkpatch", help="Compute a patch gametext.spec file to go from one GAMETEXT.bin/tab pair to another.")
    mkpatch_cmd.add_argument("--tab1", type=argparse.FileType("rb"), help="The base GAMETEXT.tab file.", required=True)
    mkpatch_cmd.add_argument("--bin1", type=argparse.FileType("rb"), help="The base GAMETEXT.bin file.", required=True)
    mkpatch_cmd.add_argument("--tab2", type=argparse.FileType("rb"), help="The target GAMETEXT.tab file.", required=True)
    mkpatch_cmd.add_argument("--bin2", type=argparse.FileType("rb"), help="The target GAMETEXT.bin file.", required=True)
    mkpatch_cmd.add_argument("-o", "--output", type=argparse.FileType("w", encoding="utf-8"), help="The patch spec file to write.", required=True)

    args, _ = parser.parse_known_args()
    cmd = args.command

    if cmd == "bin2spec":
        bin2spec(args.tab, args.bin, args.output)
    elif cmd == "spec2bin":
        spec2bin(args.spec, args.tab, args.bin)
    elif cmd == "mkpatch":
        mkpatch(args.tab1, args.bin1, args.tab2, args.bin2, args.output)

if __name__ == "__main__":
    main()
