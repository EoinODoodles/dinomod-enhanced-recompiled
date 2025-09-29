from __future__ import annotations
from io import BufferedReader, BufferedWriter
import os
import struct
from typing import Any, TextIO
import pylibyaml
import yaml

class WarpTabEntry:
    def __init__(self, x: float, y: float, z: float, layer: int) -> None:
        self.x = x
        self.y = y
        self.z = z
        self.layer = layer

    def clone(self):
        return WarpTabEntry(self.x, self.y, self.z, self.layer)

class WarpTab:
    def __init__(self, entries: list[WarpTabEntry | None]) -> None:
        self.entries = entries

    def apply_patch(self, other: WarpTab):
        for i, entry in enumerate(other.entries):
            if i >= len(self.entries):
                # New
                self.entries.append(None if entry == None else entry.clone())
                continue
            
            if entry == None:
                # Not defined by patch
                continue

            self.entries[i] = entry

    def write_yaml(self, output: TextIO):
        data = []
        for i, entry in enumerate(self.entries):
            if entry == None:
                continue
            data.append({
                'index': i,
                'x': entry.x,
                'y': entry.y,
                'z': entry.z,
                'layer': entry.layer
            })
        
        yaml.safe_dump(data, output, sort_keys=False)
    
    def write_binary(self, output: BufferedWriter):
        for i, entry in enumerate(self.entries):
            if entry == None:
                entry = WarpTabEntry(0, 0, 0, 0)
            
            output.write(struct.pack(">fffI", entry.x, entry.y, entry.z, entry.layer))

    @staticmethod
    def from_yaml_file(stream: Any):
        tab_yaml = yaml.safe_load(stream)
        entry_map: dict[int, WarpTabEntry] = {}
        
        for entry_yaml in tab_yaml:
            index = entry_yaml['index']
            x = float(entry_yaml['x'])
            y = float(entry_yaml['y'])
            z = float(entry_yaml['z'])
            layer = entry_yaml['layer']

            entry_map[index] = WarpTabEntry(x, y, z, layer)
        
        num_entries = 0 if len(entry_map) == 0 else (max(entry_map.keys()) + 1)
        entries: list[WarpTabEntry | None] = []
        for i in range(num_entries):
            entries.append(entry_map.get(i))

        return WarpTab(entries)

    @staticmethod
    def from_binary_file(file: BufferedReader):
        assert(file.tell() == 0)

        file.seek(0, os.SEEK_END)
        num_entries = file.tell() // 0x10
        file.seek(0, os.SEEK_SET)

        entries: list[WarpTabEntry | None] = []
        for i in range(num_entries):
            data = struct.unpack(">fffI", file.read(0x10))
            entries.append(WarpTabEntry(data[0], data[1], data[2], data[3]))
        
        return WarpTab(entries)
