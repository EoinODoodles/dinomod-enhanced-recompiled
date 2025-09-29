from enum import Enum
from io import BufferedReader, BytesIO
import os
import struct

class DinoFile(Enum):
    AUDIO_TAB = 0
    AUDIO_BIN = 1
    SFX_TAB = 2
    SFX_BIN = 3
    AMBIENT_TAB = 4
    AMBIENT_BIN = 5
    MUSIC_TAB = 6
    MUSIC_BIN = 7
    MPEG_TAB = 8
    MPEG_BIN = 9
    MUSICACTIONS_BIN = 10
    CAMACTIONS_BIN = 11
    LACTIONS_BIN = 12
    ANIMCURVES_BIN = 13
    ANIMCURVES_TAB = 14
    OBJSEQ2CURVE_TAB = 15
    FONTS_BIN = 16
    CACHEFON_BIN = 17
    CACHEFON2_BIN = 18
    GAMETEXT_BIN = 19
    GAMETEXT_TAB = 20
    GLOBALMAP_BIN = 21
    TABLES_BIN = 22
    TABLES_TAB = 23
    SCREENS_BIN = 24
    SCREENS_TAB = 25
    VOXMAP_BIN = 26
    VOXMAP_TAB = 27
    TEXPRE_TAB = 28
    TEXPRE_BIN = 29
    WARPTAB_BIN = 30
    MAPS_BIN = 31
    MAPS_TAB = 32
    MAPINFO_BIN = 33
    MAPSETUP_IND = 34
    MAPSETUP_TAB = 35
    TEX1_BIN = 36
    TEX1_TAB = 37
    TEXTABLE_BIN = 38
    TEX0_BIN = 39
    TEX0_TAB = 40
    BLOCKS_BIN = 41
    BLOCKS_TAB = 42
    TRKBLK_BIN = 43
    HITS_BIN = 44
    HITS_TAB = 45
    MODELS_TAB = 46
    MODELS_BIN = 47
    MODELIND_BIN = 48
    MODANIM_TAB = 49
    MODANIM_BIN = 50
    ANIM_TAB = 51
    ANIM_BIN = 52
    AMAP_TAB = 53
    AMAP_BIN = 54
    BITTABLE_BIN = 55
    WEAPONDATA_BIN = 56
    VOXOBJ_TAB = 57
    VOXOBJ_BIN = 58
    MODLINES_BIN = 59
    MODLINES_TAB = 60
    SAVEGAME_BIN = 61
    SAVEGAME_TAB = 62
    OBJSEQ_BIN = 63
    OBJSEQ_TAB = 64
    OBJECTS_TAB = 65
    OBJECTS_BIN = 66
    OBJINDEX_BIN = 67
    OBJEVENT_BIN = 68
    OBJHITS_BIN = 69
    DLLS_BIN = 70
    DLLS_TAB = 71
    DLLSIMPORTTAB_BIN = 72
    ENVFXACT_BIN = 73

DINO_FILENAMES = [
    "AUDIO.tab", # 00
    "AUDIO.bin", # 01
    "SFX.tab", # 02
    "SFX.bin", # 03
    "AMBIENT.tab", # 04
    "AMBIENT.bin", # 05
    "MUSIC.tab", # 06
    "MUSIC.bin", # 07
    "MPEG.tab", # 08
    "MPEG.bin", # 09
    "MUSICACTIONS.bin", # 0A
    "CAMACTIONS.bin", # 0B
    "LACTIONS.bin", # 0C
    "ANIMCURVES.bin", # 0D
    "ANIMCURVES.tab", # 0E
    "OBJSEQ2CURVE.tab", # 0F
    "FONTS.bin", # 10
    "CACHEFON.bin", # 11
    "CACHEFON2.bin", # 12
    "GAMETEXT.bin", # 13
    "GAMETEXT.tab", # 14
    "GLOBALMAP.bin", # 15
    "TABLES.bin", # 16
    "TABLES.tab", # 17
    "SCREENS.bin", # 18
    "SCREENS.tab", # 19
    "VOXMAP.bin", # 1A
    "VOXMAP.tab", # 1B
    "TEXPRE.tab", # 1C
    "TEXPRE.bin", # 1D
    "WARPTAB.bin", # 1E
    "MAPS.bin", # 1F
    "MAPS.tab", # 20
    "MAPINFO.bin", # 21
    "MAPSETUP_IND", # 22
    "MAPSETUP.tab", # 23
    "TEX1.bin", # 24
    "TEX1.tab", # 25
    "TEXTABLE.bin", # 26
    "TEX0.bin", # 27
    "TEX0.tab", # 28
    "BLOCKS.bin", # 29
    "BLOCKS.tab", # 2A
    "TRKBLK.bin", # 2B
    "HITS.bin", # 2C
    "HITS.tab", # 2D
    "MODELS.tab", # 2E
    "MODELS.bin", # 2F
    "MODELIND.bin", # 30
    "MODANIM.tab", # 31
    "MODANIM.bin", # 32
    "ANIM.tab", # 33
    "ANIM.bin", # 34
    "AMAP.tab", # 35
    "AMAP.bin", # 36
    "BITTABLE.bin", # 37
    "WEAPONDATA.bin", # 38
    "VOXOBJ.tab", # 39
    "VOXOBJ.bin", # 3A
    "MODLINES.bin", # 3B
    "MODLINES.tab", # 3C
    "SAVEGAME.bin", # 3D
    "SAVEGAME.tab", # 3E
    "OBJSEQ.bin", # 3F
    "OBJSEQ.tab", # 40
    "OBJECTS.tab", # 41
    "OBJECTS.bin", # 42
    "OBJINDEX.bin", # 43
    "OBJEVENT.bin", # 44
    "OBJHITS.bin", # 45
    "DLLS.bin", # 46
    "DLLS.tab", # 47
    "DLLSIMPORTTAB.bin", # 48
    "ENVFXACT.bin", # 49
]

FS_TAB_ROM_OFFSET = 0xA4970
FS_ROM_OFFSET = 0xA4AA0

class FSTabEntry:
    def __init__(self, offset: int, size: int) -> None:
        self.offset = offset
        self.size = size

class FSTab:
    def __init__(self, entries: list[FSTabEntry]) -> None:
        self.entries = entries

    def get_file(self, file: int | DinoFile, rom: BufferedReader, fs_offset=FS_ROM_OFFSET) -> bytes:
        if isinstance(file, DinoFile):
            file = file.value

        if file < 0 or file >= len(self.entries):
            return bytes()
        
        entry = self.entries[file]
        rom.seek(fs_offset + entry.offset, os.SEEK_SET)

        return rom.read(entry.size)
    
    @staticmethod
    def from_rom(rom: BufferedReader, offset=FS_TAB_ROM_OFFSET):
        if offset >= 0:
            rom.seek(offset, os.SEEK_SET)

        entries: list[FSTabEntry] = []
        num_entries = struct.unpack(">I", rom.read(4))[0]
        for _ in range(num_entries):
            offset, next_offset = struct.unpack(">II", rom.read(8))
            rom.seek(-4, os.SEEK_CUR)

            size = next_offset - offset

            entries.append(FSTabEntry(offset, size))

        return FSTab(entries)
