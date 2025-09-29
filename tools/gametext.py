#!/usr/bin/env python3
from __future__ import annotations
import argparse
import codecs
from io import BufferedReader, BufferedWriter
import math
import os
import struct
import sys
from typing import TextIO

class GameTextString:
    def __init__(self, cmd: int, string: bytes) -> None:
        self.cmd = cmd
        self.string = string

    def clone(self):
        return GameTextString(self.cmd, bytes(self.string))

class GameTextFile:
    def __init__(self, strings: list[GameTextString]) -> None:
        self.strings = strings

    def clone(self):
        return GameTextFile([s.clone() for s in self.strings])

class GameTextLang:
    def __init__(self, files: list[GameTextFile | None]) -> None:
        self.files = files

    def clone(self):
        return GameTextLang([None if f == None else f.clone() for f in self.files])

class GameText:
    def __init__(self, langs: list[GameTextLang | None]) -> None:
        self.langs = langs
    
    def apply_patch(self, other: GameText):
        for lang_idx, lang in enumerate(other.langs):
            if lang_idx >= len(self.langs):
                # New lang
                if lang == None:
                    self.langs.append(None)
                else:
                    self.langs.append(lang.clone())
                continue

            if lang == None:
                # Lang already exists, patch doesn't define it
                continue

            our_lang = self.langs[lang_idx]
            if our_lang == None:
                # Lang not defined in base, but patch defines it
                self.langs[lang_idx] = lang.clone()
                continue

            for file_idx, file in enumerate(lang.files):
                if file_idx >= len(our_lang.files):
                    # New file
                    if file == None:
                        our_lang.files.append(None)
                    else:
                        our_lang.files.append(file.clone())
                    continue

                if file == None:
                    # File already exists, patch doesn't define it
                    continue

                # Overwrite with patch version
                our_lang.files[file_idx] = file.clone()
        
        self.sync_langs()
    
    def sync_langs(self):
        # Pad all langs to the same number of files
        num_files = 0
        for lang in self.langs:
            if lang != None:
                num_files_in_lang = len(lang.files)
                num_files = max(num_files, num_files_in_lang)
        for lang in self.langs:
            if lang != None:
                for _ in range(num_files - len(lang.files)):
                    lang.files.append(None)


class GameTextBinParser:
    def __init__(self) -> None:
        pass

    def parse(self, tab: BufferedReader, bin: BufferedReader):
        langs: list[GameTextLang | None] = []

        (language_count, file_count) = struct.unpack(">HH", tab.read(4))

        for lang_idx in range(language_count):
            files: list[GameTextFile | None] = []

            lang_offset = struct.unpack(">I", tab.read(4))[0]

            file_str_counts: list[int] = []
            for file_idx in range(file_count):
                # HACK: for original dinomod gametext tab
                if len(tab.peek(1)) == 0:
                    print('BAD FILE STR COUNTS', file_idx, hex(tab.tell()))
                    file_str_counts.append(0)
                    continue
                file_str_counts.append(struct.unpack(">B", tab.read(1))[0])

            file_sizes: list[int] = []
            for file_idx in range(file_count):
                # HACK: for original dinomod gametext tab
                if len(tab.peek(1)) == 0:
                    print('BAD FILE STR SIZES', file_idx, hex(tab.tell()))
                    file_sizes.append(0)
                    continue
                file_sizes.append(struct.unpack(">H", tab.read(2))[0])

            file_offsets: list[int] = []
            for file_idx in range(file_count):
                # HACK: for original dinomod gametext tab
                if len(tab.peek(1)) == 0:
                    print('BAD FILE STR OFFSETS', file_idx, hex(tab.tell()))
                    file_offsets.append(0 if len(file_offsets) == 0 else file_offsets[len(file_offsets) - 1])
                    continue
                file_offsets.append(struct.unpack(">H", tab.read(2))[0] * 2)
            
            for file_idx in range(file_count):
                if file_sizes[file_idx] == 0:
                    files.append(GameTextFile([]))
                    continue

                strings: list[GameTextString] = []

                bin.seek(lang_offset + file_offsets[file_idx], os.SEEK_SET)

                num_strs = file_str_counts[file_idx]

                commands: list[int] = []
                for str_idx in range(num_strs):
                    cmd = struct.unpack(">h", bin.read(2))[0]
                    commands.append(cmd)

                strs_bytes = bin.read(file_sizes[file_idx] - (num_strs * 2))
                strs = self.__read_strings(strs_bytes, num_strs)

                for str_idx in range(num_strs):
                    cmd = commands[str_idx]
                    string = strs[str_idx]

                    strings.append(GameTextString(cmd, string))

                files.append(GameTextFile(strings))
            
            langs.append(GameTextLang(files))
        
        return GameText(langs)

    def __read_strings(self, data: bytes, count: int):
        strs: list[bytes] = []

        start = 0
        for i in range(len(data)):
            if data[i] == 0:
                strs.append(data[start:i])
                start = i + 1
                if len(strs) == count:
                    break

        assert(len(strs) == count)

        return strs

class GameTextBinWriter:
    def __init__(self) -> None:
        pass

    def write(self, gametext: GameText, tab: BufferedWriter, bin: BufferedWriter):
        assert(tab.tell() == 0)
        assert(bin.tell() == 0)
        
        gametext.sync_langs()

        lang_count = len(gametext.langs)
        file_count = 0
        for lang in gametext.langs:
            if lang != None:
                file_count = max(file_count, len(lang.files))

        tab.write(struct.pack(">HH", lang_count, file_count))

        for lang in gametext.langs:
            if lang == None:
                lang = GameTextLang([GameTextFile([]) for _ in range(file_count)])

            lang_offset = bin.tell()

            file_sizes: list[int] = []
            file_offsets: list[int] = []

            for file in lang.files:
                if file == None:
                    file = GameTextFile([])
                
                file_abs_offset = bin.tell()
                file_offsets.append(file_abs_offset - lang_offset)

                for string in file.strings:
                    bin.write(struct.pack(">h", string.cmd))
                for string in file.strings:
                    bin.write(string.string)
                    bin.write(b'\0')

                file_size = self.__align(bin.tell() - file_abs_offset, 2)
                bin.write(bytearray(max((file_abs_offset + file_size) - bin.tell(), 0)))

                file_sizes.append(file_size)
            
            tab.write(struct.pack(">I", lang_offset))

            for file in lang.files:
                tab.write(struct.pack(">B", 0 if file == None else len(file.strings)))
            for size in file_sizes:
                tab.write(struct.pack(">H", size))
            for offset in file_offsets:
                tab.write(struct.pack(">H", offset // 2))

    @staticmethod
    def __align(n: int, alignment: int) -> int:
        return math.ceil(n / alignment) * alignment

class GameTextSpecParser:
    __HEX_ALPHABET = set([c for c in "abcdefABCDEF1234567890"])

    __lang_map: dict[int, dict[int, GameTextFile]]
    __lang_idx: int
    __file_map: dict[int, GameTextFile]
    __file_idx: int
    __strings: list[GameTextString]
    __cmd: int | None
    __line: int
    __col: int
    __token: str
    __token_line: int
    __token_col: int
    __filename: str

    def __init__(self) -> None:
        pass

    def parse(self, spec: TextIO):
        self.__lang_map = {}
        self.__lang_idx = -1
        self.__file_map = {}
        self.__file_idx = -1
        self.__strings = []
        self.__cmd = None
        self.__line = 1
        self.__col = 1
        self.__token = ""
        self.__token_line = self.__line
        self.__token_col = self.__col
        self.__filename = spec.name

        in_quote = False
        escaped = False
        while c := spec.read(1):
            if c == '\n':
                self.__line += 1
                self.__col = 1
            else:
                self.__col += 1

            if c.isspace() and not in_quote:
                if len(self.__token) > 0:
                    self.__handle_token()
                    self.__token = ""
                    self.__token_line = self.__line
                    self.__token_col = self.__col
            else:
                self.__token += c

                if not escaped:
                    if c == "\"":
                        in_quote = not in_quote
                    elif c == "\\":
                        escaped = True
                else:
                    escaped = False

        if len(self.__token) > 0:
            self.__handle_token()
        self.__flush_lang()

        # Note: The number of langs/files is based off of the highest lang/file index defined,
        # with empty definitions to fill in the gaps. Additionally, all langs must have the same
        # number of files, so each lang is padded to the longest of each lang.
        num_langs = 0 if len(self.__lang_map) == 0 else (max(self.__lang_map.keys()) + 1)
        num_files = 0
        for file_map in self.__lang_map.values():
            num_files_in_lang = 0 if len(file_map) == 0 else (max(file_map.keys()) + 1)
            num_files = max(num_files, num_files_in_lang)
        langs: list[GameTextLang | None] = []
        for lang_idx in range(num_langs):
            file_map = self.__lang_map.get(lang_idx)
            if file_map == None:
                lang = None
            else:
                files: list[GameTextFile | None] = []
                for file_idx in range(num_files):
                    files.append(file_map.get(file_idx))
                lang = GameTextLang(files)
            
            langs.append(lang)

        return GameText(langs)

    def __encode_string(self, string: str):
        data = bytearray()

        i = 0
        escaped = False
        while i < len(string):
            c = string[i]
            i += 1
            if escaped or (c != "\\" and c != "#"):
                data.extend(c.encode(encoding="windows-1252", errors="strict"))
            
            if not escaped:
                if c == "\\":
                    escaped = True
                elif c == "#":
                    start = i
                    while i < len(string) and (i - start) < 4:
                        c = string[i]
                        if not c in self.__HEX_ALPHABET:
                            break
                        i += 1
                    hex_str = string[start:i]
                    try:
                        hex_num = int(hex_str, base=16)
                    except:
                        self.__token_error(f"Invalid hex escape '{hex_str}'.", offset=start)
                    if len(hex_str) == 2:
                        data.extend(struct.pack(">H", hex_num))
                    else:
                        self.__token_error(f"Invalid hex escape '{hex_str}'. Must be exactly 4 characters.", offset=start)
            else:
                escaped = False

        return data

    def __flush_string(self, string: str):
        if self.__cmd == None:
            return
        
        self.__strings.append(GameTextString(self.__cmd, self.__encode_string(string)))
        
        self.__cmd = None

    def __flush_file(self):
        if self.__file_idx == -1:
            return

        self.__flush_string("")

        self.__file_map[self.__file_idx] = GameTextFile(self.__strings)

        self.__strings = []
        self.__file_idx = -1

    def __flush_lang(self):
        if self.__lang_idx == -1:
            return
        
        self.__flush_file()

        self.__lang_map[self.__lang_idx] = self.__file_map

        self.__file_map = {}
        self.__lang_idx = -1

    def __handle_token(self):
        if self.__token.startswith("\""):
            # string
            if self.__file_idx == -1:
                self.__token_error("A file must be declared before a string.")
            if self.__cmd == None:
                self.__token_error("String must be preceded by a command.")
            string = self.__token[1:-1]
            self.__flush_string(string)
        else:
            cmd_str, *args = self.__token.split(",")
            cmd_str = cmd_str.lower()

            if cmd_str == "lang":
                # lang command
                if len(args) != 1:
                    self.__token_error(f"Expected 1 argument for 'lang' command, found {len(args)}.")
                self.__flush_lang()
                self.__lang_idx = self.__parse_int(args[0])
                if self.__lang_idx in self.__lang_map:
                    self.__token_warning(f"Language {self.__lang_idx} already exists! Previous definition will be overriden!")
            elif cmd_str == "file":
                # file command
                if self.__lang_idx == -1:
                    self.__token_error("A language must be declared before a file.")
                if len(args) != 1:
                    self.__token_error(f"Expected 1 argument for 'file' command, found {len(args)}.")
                self.__flush_file()
                self.__file_idx = self.__parse_int(args[0])
                if self.__file_idx in self.__file_map:
                    self.__token_warning(f"File {self.__file_idx} already exists! Previous definition will be overriden!")
            else:
                # numeric command
                if self.__file_idx == -1:
                    self.__token_error("A file must be declared before a command.")
                if len(args) != 0:
                    self.__token_error(f"Expected 0 arguments for '{cmd_str}' command, found {len(args)}.")
                self.__flush_string("")
                self.__cmd = self.__parse_int(cmd_str)
    
    def __token_error(self, message: str, offset=0):
        print(f"[ERR] {message}\n  at {self.__filename}, line: {self.__token_line}, column: {self.__token_col + offset}")
        sys.exit(1)

    def __token_warning(self, message: str, offset=0):
        print(f"[WARN] {message}\n  at {self.__filename}, line: {self.__token_line}, column: {self.__token_col + offset}")

    def __parse_int(self, string: str) -> int:
        try:
            return int(string, base=0)
        except:
            self.__token_error(f"Expected integer, found '{string}'.")
            raise NotImplementedError()

class GameTextSpecWriter:
    def __init__(self) -> None:
        pass

    def write(self, gametext: GameText, output: TextIO):
        # codecs.register_error("_dino", GameTextSpecWriter.__dino_codec_error_handler)
        
        first_lang = True
        for (lang_idx, lang) in enumerate(gametext.langs):
            if lang == None:
                continue
            
            if not first_lang:
                output.write("\n\n")
            first_lang = False

            output.write(f"LANG,{lang_idx}\n")
            
            for (file_idx, file) in enumerate(lang.files):
                if file is None:
                    continue
                if len(file.strings) == 0 and file_idx < (len(lang.files) - 1):
                    continue

                output.write(f"\nFILE,{file_idx}\n")

                all_zero_cmds = True
                for string in file.strings:
                    if string.cmd != 0:
                        all_zero_cmds = False

                line_start = True
                file_start = True

                for cmd_string in file.strings:
                    cmd = cmd_string.cmd
                    string = cmd_string.string

                    if not file_start and (cmd > 0 or (cmd == 0 and all_zero_cmds)):
                        if cmd != 0 or len(string) > 0:
                            output.write("\n")
                            line_start = True
                        else:
                            output.write(" ")
                    elif not line_start:
                        output.write(" ")

                    # if cmd >= -255 and cmd <= -1:
                    #     colour8 = (~cmd) & 0xFF
                    #     r = (colour8 & 0b11000000) >> 6
                    #     g = (colour8 & 0b00110000) >> 4
                    #     b = (colour8 & 0b00001100) >> 2
                    #     a = (colour8 & 0b00000011)

                    #     cmd = f"color,{r},{g},{b},{a}"
                    # elif cmd == 0:
                    #     cmd = "newline"
                    # elif cmd == -256:
                    #     cmd = "vcenter"
                    # elif cmd == -257:
                    #     cmd = "hleft"
                    # elif cmd == -258:
                    #     cmd = "a_btn"

                    if len(string) == 0:
                        output.write(str(cmd))
                    else:
                        escaped = string \
                            .replace(b"\\", b"\\\\") \
                            .replace(b"\"", b"\\\"") \
                            .replace(b",", b"\\,") \
                            .replace(b"#", b"\\#")
                        output.write(f"{cmd} \"{escaped.decode(encoding="windows-1252", errors="strict")}\"")

                    line_start = False
                    file_start = False

                if not line_start:
                    output.write("\n")

    # @staticmethod
    # def __dino_codec_error_handler(err: UnicodeDecodeError | UnicodeError):
    #     assert(isinstance(err, UnicodeDecodeError))

    #     data = err.object[err.start:err.end]

    #     return "#{}".format(data.hex().upper()), err.end

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
    parser = GameTextBinParser()
    base = parser.parse(tab1, bin1)
    targ = parser.parse(tab2, bin2)

    langs: list[GameTextLang | None] = []
    for lang_idx, lang in enumerate(targ.langs):
        if lang_idx >= len(base.langs):
            # New lang
            langs.append(None if lang == None else lang.clone())
            continue

        if lang == None:
            # Lang not in target
            if base.langs[lang_idx] != None:
                print(f"[WARN] Base gametext defines language {lang_idx} but target gametext does not!")
            langs.append(None)
            continue

        base_lang = base.langs[lang_idx]
        if base_lang == None:
            # Lang not in base but is in target
            langs.append(lang.clone())
            continue

        files: list[GameTextFile | None] = []
        for file_idx, file in enumerate(lang.files):
            if file_idx >= len(base_lang.files):
                # New file
                files.append(None if file == None else file.clone())
                continue

            if file == None:
                # File not in target
                if base_lang.files[file_idx] != None:
                    print(f"[WARN] Base gametext defines file {lang_idx}:{file_idx} but target gametext does not!")
                files.append(None)
                continue
                
            base_file = base_lang.files[file_idx]
            if base_file == None:
                # File not in base but is in target
                files.append(file.clone())
                continue

            # Determine if file changed in target
            are_files_diff = False
            if len(base_file.strings) != len(file.strings):
                are_files_diff = True
            else:
                for str_idx in range(len(file.strings)):
                    base_str = base_file.strings[str_idx]
                    targ_str = file.strings[str_idx]

                    if base_str.cmd != targ_str.cmd or base_str.string != targ_str.string:
                        are_files_diff = True
                        break
            
            if are_files_diff:
                files.append(file.clone())
            else:
                files.append(None)

        langs.append(GameTextLang(files))
    
    patch = GameText(langs)
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
