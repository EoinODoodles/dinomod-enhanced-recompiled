from __future__ import annotations
from io import BufferedReader, BufferedWriter
import os
from pathlib import Path
import struct

class MPEGFile:
    def __init__(self, data: bytes) -> None:
        self.data = data

    def clone(self):
        return MPEGFile(bytes(self.data))

class MPEGList:
    def __init__(self, files: list[MPEGFile | None]) -> None:
        self.files = files

    def apply_patch(self, other: MPEGList):
        for i, file in enumerate(other.files):
            if i >= len(self.files):
                # New
                self.files.append(file)
                continue
            
            if file == None:
                # Not defined in patch
                continue

            self.files[i] = file.clone()

    def write_binary(self, tab: BufferedWriter, bin: BufferedWriter):
        for file in self.files:
            tab.write(struct.pack(">I", bin.tell()))
            if file == None:
                bin.write(bytearray())
            else:
                bin.write(file.data)
        tab.write(struct.pack(">I", bin.tell()))

    @staticmethod
    def from_binary(tab: BufferedReader, bin: BufferedReader):
        files: list[MPEGFile | None] = []

        tab.seek(0, os.SEEK_END)
        file_count = (tab.tell() // 4) - 1
        tab.seek(0, os.SEEK_SET)
        
        for _ in range(file_count):
            file_offset, next_file_offset = struct.unpack(">II", tab.read(8))
            tab.seek(-4, os.SEEK_CUR)

            # Note: Only dinomod's MPEG.tab ends with -1
            if next_file_offset == 0xffffffff:
                break

            bin.seek(file_offset, os.SEEK_SET)

            file_size = next_file_offset - file_offset
            files.append(MPEGFile(bin.read(file_size)))
        
        return MPEGList(files)
    
    @staticmethod
    def from_directory(dir: Path):
        mpeg_map: dict[int, MPEGFile] = {}
        for p in dir.glob("*.mp3"):
            # Expects filename to be something like "0010 000A.mp3"
            idx = int(p.name.split(" ")[0].lstrip("0"))
            with open(p, "rb") as mpeg_file:
                mpeg_map[idx] = MPEGFile(mpeg_file.read())
        
        max_idx = 0 if len(mpeg_map) == 0 else (max(mpeg_map.keys()) + 1)
        files: list[MPEGFile | None] = []
        for i in range(max_idx):
            files.append(mpeg_map.get(i))

        return MPEGList(files)
