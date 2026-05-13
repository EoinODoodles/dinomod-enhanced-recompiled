#!/usr/bin/env python3
import argparse
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import TypeVar

from assetlib.gametext import GameTextBinParser, GameTextBinWriter, GameTextSpecParser
from assetlib.mpeg import MPEGList
from assetlib.objects import ObjectList, obj_load_id_name_map_from_bin
from assetlib.seq import ObjSeq, Curve, seqs_from_directory, seqs_dump_to_dir
from assetlib.warp import WarpTab

T = TypeVar('T')

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
PROJECT_DIR = SCRIPT_DIR.parent

class DiffTool:
    def __init__(self, a_dir: Path, b_dir: Path, diff_dir: Path) -> None:
        self.a_dir = a_dir
        self.b_dir = b_dir
        self.diff_dir = diff_dir
    
    def seq_diff(self):
        print("Diffing SEQUENCES...")
        seqs_a = seqs_from_directory(self.a_dir.joinpath("SEQUENCES"))
        seqs_b = seqs_from_directory(self.b_dir.joinpath("SEQUENCES"))

        pre_animseqs: list[Curve | None] = self.__diff_lists(seqs_a["pre_animseqs"], seqs_b["pre_animseqs"])
        objseqs: list[ObjSeq | None] = self.__diff_lists(seqs_a["objseqs"], seqs_b["objseqs"])

        for i, o in enumerate(objseqs):
            if o != None:
                print(f"SEQUENCES\\{i:04} SEQ_{i:04X}")
        
        obj_id2name = self.__get_obj_names()

        out_dir = self.diff_dir.joinpath("SEQUENCES")
        out_dir.mkdir(exist_ok=True)
        seqs_dump_to_dir(out_dir, { "objseqs": objseqs, "pre_animseqs": pre_animseqs, "post_animseqs": [] }, obj_id2name)

    def __get_obj_names(self):
        objects_tab = PROJECT_DIR.joinpath("bin/OBJECTS.tab")
        objects_bin = PROJECT_DIR.joinpath("bin/OBJECTS.bin")
        objects_idx = PROJECT_DIR.joinpath("bin/OBJINDEX.bin")

        with open(objects_tab, "rb") as objects_tab_file, \
            open(objects_bin, "rb") as objects_bin_file, \
            open(objects_idx, "rb") as objects_idx_file:
            return obj_load_id_name_map_from_bin(objects_bin_file, objects_tab_file, objects_idx_file)

    def __diff_lists(self, a: list[T], b: list[T]):
        diff: list[T | None] = []
        a_count = len(a)
        b_count = len(b)

        for i in range(b_count):
            if i < a_count and i < b_count:
                a_objseq = a[i]
                b_objseq = b[i]

                if a_objseq != b_objseq:
                    diff.append(b_objseq)
                else:
                    diff.append(None)
            elif i < b_count:
                diff.append(b[i])
            else:
                diff.append(None)
        
        return diff

def main():
    a_dir = PROJECT_DIR.joinpath("extract")
    b_dir = PROJECT_DIR.joinpath("dinomod/extract")
    diff_dir = PROJECT_DIR.joinpath("diff")

    diff_dir.mkdir(parents=True, exist_ok=True)

    difftool = DiffTool(a_dir, b_dir, diff_dir)
    difftool.seq_diff()

    print("Done.")

if __name__ == "__main__":
    main()
