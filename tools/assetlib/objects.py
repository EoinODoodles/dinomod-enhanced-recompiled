from __future__ import annotations
from io import BufferedReader, BufferedWriter
import os
from pathlib import Path
import struct

class ObjDef:
    def __init__(self, data: bytes) -> None:
        self.data = data
    
    @property
    def name(self):
        # Deal with embedded null bytes in dinomod Sabre/Krystal objdef names
        name_bytes = self.data[0x5f:0x6f]
        first_null = name_bytes.find(b'\0')
        if first_null < 0:
            return name_bytes.decode()
        else:
            return name_bytes[0:first_null].decode()

    def clone(self):
        return ObjDef(bytes(self.data))

class ObjectList:
    def __init__(self, objects: list[ObjDef | None]) -> None:
        self.objects = objects

    def apply_patch(self, other: ObjectList):
        for i, obj in enumerate(other.objects):
            if i >= len(self.objects):
                # New
                self.objects.append(obj)
                continue
            
            if obj == None:
                # Not defined in patch
                continue

            self.objects[i] = obj.clone()

    def write_binary(self, tab: BufferedWriter, bin: BufferedWriter):
        for obj in self.objects:
            tab.write(struct.pack(">I", bin.tell()))
            if obj == None:
                bin.write(bytearray(0xAC))
            else:
                bin.write(obj.data)
        tab.write(struct.pack(">II", bin.tell(), 0xFFFFFFFF))

    @staticmethod
    def from_binary(tab: BufferedReader, bin: BufferedReader):
        objects: list[ObjDef | None] = []
        
        while True:
            file_offset, next_file_offset = struct.unpack(">II", tab.read(8))
            tab.seek(-4, os.SEEK_CUR)

            if next_file_offset == 0xffffffff:
                break

            bin.seek(file_offset, os.SEEK_SET)

            file_size = next_file_offset - file_offset
            objects.append(ObjDef(bin.read(file_size)))
        
        return ObjectList(objects)
    
    @staticmethod
    def from_directory(dir: Path, exclude: set[int]=set()):
        obj_map: dict[int, ObjDef] = {}
        for p in dir.glob("*.bin"):
            # Expects filename to be something like "0010 000A name.bin"
            idx = int(p.name.split(" ")[0].lstrip("0"))
            if idx in exclude:
                continue
            with open(p, "rb") as obj_file:
                obj_map[idx] = ObjDef(obj_file.read())
        
        max_idx = 0 if len(obj_map) == 0 else (max(obj_map.keys()) + 1)
        objects: list[ObjDef | None] = []
        for i in range(max_idx):
            objects.append(obj_map.get(i))

        return ObjectList(objects)
