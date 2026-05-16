#!/usr/bin/env python3
import argparse
from io import BufferedReader
import os
from pathlib import Path
import struct

SCRIPT_DIR = Path(os.path.dirname(os.path.realpath(__file__)))
BIN_DIR = SCRIPT_DIR.joinpath("../bin")

class BitTableEntry:
    def __init__(self, 
                 start_bit: int,
                 length: int,
                 is_task: bool,
                 bitstring: int,
                 task: int) -> None:
        self.start_bit = start_bit
        self.length = length
        self.is_task = is_task
        self.bitstring = bitstring
        self.task = task

def read_bittable(file: BufferedReader):
    entries: "list[BitTableEntry]" = []
    while True:
        data = file.read(4)
        if len(data) < 4:
            break

        entry_tuple = struct.unpack_from(">HBB", data)

        entries.append(BitTableEntry(
            start_bit=entry_tuple[0],
            length=entry_tuple[1] & 0b1_1111,
            is_task=(entry_tuple[1] & 0b10_0000) != 0,
            bitstring=entry_tuple[1] >> 6,
            task=entry_tuple[2] 
        ))

    return entries

def find_free_bits(entries: "list[BitTableEntry]"):
    by_bitstring: dict[int, list[BitTableEntry]] = {
        0: [],
        1: [],
        2: [],
        3: []
    }

    for entry in entries:
        by_bitstring[entry.bitstring].append(entry)

    for list in by_bitstring.values():
        list.sort(key=lambda x: x.start_bit)

    bitstring_lengths = [128 * 8, 512 * 8, 256 * 8, 256 * 8]
    for bitstring in by_bitstring.keys():
        list = by_bitstring[bitstring]
        i = 0
        bit = 0
        first = True
        while i < len(list):
            entry = list[i]
            if entry.start_bit != 0 or entry.length != 0:
                if entry.start_bit != bit:
                    free_range = (bit, entry.start_bit)
                    if first:
                        print(f"==== Bitstring {bitstring} ====")
                        first = False
                    print(f"Bits {free_range[0]} - {free_range[1] - 1}")

            bit = entry.start_bit + entry.length + 1
            i += 1
        
        bitstring_len = bitstring_lengths[bitstring]
        if bit < bitstring_len:
            if first:
                print(f"==== Bitstring {bitstring} ====")
            print(f"Bits {bit} - {bitstring_len - 1}")

def main():
    parser = argparse.ArgumentParser(description="Finds free (unused) bit ranges in BITTABLE.bin.")
    parser.add_argument("--bittable", type=str, help="Path to BITTABLE.bin", default=str(BIN_DIR.joinpath("BITTABLE.bin")))

    args = parser.parse_args()

    with open(args.bittable, "rb") as file:
        entries = read_bittable(file)
    
    print(f"Free bitstring ranges (inclusive) for {Path(args.bittable).absolute().as_posix()}:")
    find_free_bits(entries)

if __name__ == "__main__":
    main()
