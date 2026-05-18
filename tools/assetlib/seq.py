from __future__ import annotations
from copy import deepcopy
from io import BufferedReader, BufferedWriter
import os
from pathlib import Path
import struct
from typing import Any, TextIO, TypedDict
import pylibyaml
import yaml

from .objects import ObjSeqObjInfo

class Keyframe(TypedDict):
    value: float
    interp_type: int
    weight: int
    channel: int
    time: int

class Curve(TypedDict):
    events: list[bytes]
    keyframes: list[Keyframe]

class Actor(TypedDict):
    uid: int
    settings: int
    objectID: int

class ObjSeq(TypedDict):
    actors: list[Actor]
    curves: list[Curve]

class Sequences(TypedDict):
    pre_animseqs: list[Curve | None]
    post_animseqs: list[Curve | None]
    objseqs: list[ObjSeq | None]

event_names = {
    -1: "SETDURATION",
    0: "SETTIME",
    1: "MOVEMODE",
    2: "ANIM",
    3: "OVERRIDE",
    4: "VTXANIM",
    5: "SOFTWARE",
    6: "SFX",
    7: "GROUND_MODE",
    8: "TUNE",
    9: "ANGLE_MODE",
    10: "LOOK_AT",
    11: "CODE",
    12: "SPEECH",
    13: "ENVFX",
    14: "STORYBOARD",
    15: "SFX_WITH_DURATION",
    127: "PARAM"
}

code_event_names = {
    1: "JUMPTOTIME",
    2: "SET",
    3: "COUNTER_ADD",
    4: "PAUSE",
    5: "CONTINUE",
    6: "6",
    7: "MESSAGE",
    8: "DECISION",
    9: "JUMPTARGET",
    10: "JUMPTOLABEL"
}

channel_names = {
    0: "HEAD_ROTATE_Z",
    1: "HEAD_ROTATE_X",
    2: "HEAD_ROTATE_Y",
    3: "OPACITY",
    4: "DAY_TIME",
    5: "SCALE",
    6: "ROTATE_Z",
    7: "ROTATE_Y",
    8: "ROTATE_X",
    9: "ANIM_SPEED",
    10: "ANIM_BLEND_SPEED",
    11: "TRANSLATE_Z",
    12: "TRANSLATE_Y",
    13: "TRANSLATE_X",
    14: "FIELD_OF_VIEW",
    15: "EYE_X",
    16: "EYE_Y",
    17: "JAW",
    18: "SOUND_VOLUME"
}

envfx_names = {
    0: "SET_MUSIC",
    2: "APPLY",
    3: "PARTFX",
    4: "4",
    5: "PROJGFX",
    6: "WARP",
    7: "SFX",
    8: "BLINK",
    9: "SCREEN_FX",
    10: "SUBTITLES",
    11: "SET_BIT",
    12: "CLEAR_BIT",
    13: "CMDMENU_BUTTON_OVERRIDE",
    14: "EYELID_R",
    15: "EYELID_L",
}

code_event_6_names = {
    0: "0",
    2: "2",
    5: "5",
    6: "6",
    7: "CAMERA_SHAKE",
    9: "9",
    10: "COUNTUP_TIMER",
    11: "COUNTDOWN_TIMER",
    12: "COUNTDOWN_TIMER_SFX",
    13: "SFX_STOP",
    14: "14",
    15: "15",
    16: "16",
    18: "TOGGLE_LETTERBOX",
    19: "ENABLE_LETTERBOX",
    20: "STATIC_CAMERA",
    23: "SET_MODEL",
    24: "24",
    25: "25",
    26: "NORMAL_CAMERA",
    27: "ENABLE_OBJ_GROUP",
    28: "DISABLE_OBJ_GROUP",
    29: "SET_ACT",
    30: "DISABLE_LETTERBOX",
    31: "RESTART_CLEAR",
    32: "RESTART_GOTO",
    33: "33",
    34: "34",
    35: "CHECKPOINT",
    36: "CHECKPOINT_NO_LOCATION",
    37: "TOGGLE_PLAYER_CONTROL"
}

code_set_types = {
    0: "MESSAGE",
    1: "COUNTER",
    3: "ANIMCOUNT1",
    4: "ANIMCOUNT2",
    5: "FLAGS",
    6: "BIT"
}

def signed16(x: int):
    if x > 0x8000:
        return x - 0x10000
    else:
        return x

def unsigned16(x: int):
    if x < 0:
        return x + 0x10000
    else:
        return x

def signed8(x: int):
    if x >= 0x80:
        return x - 0x100
    else:
        return x

def unsigned8(x: int):
    if x < 0:
        return x + 0x100
    else:
        return x

def __parse_event(event: bytes):
    evt_type, delay, params = struct.unpack(">bBh", event)
    return evt_type, delay, params

def __pack_event(evt_type: int, delay: int, params: int):
    return struct.pack(">bBH", signed8(evt_type), unsigned8(delay), unsigned16(params))

def __parse_code_event(event: bytes):
    raw = struct.unpack(">i", event)[0]
    evt_type = raw & 0x3F
    val1 = (raw >> 6) & 0x3FF
    val2 = (raw >> 0x10) & 0xFFFF
    return evt_type, val1, val2

def __pack_code_event(evt_type: int, val1: int, val2: int):
    raw = ((val2 & 0xFFFF) << 0x10) | ((val1 & 0x3FF) << 6) | (evt_type & 0x3F)
    return struct.pack(">I", raw)

def seqs_apply_patch(base: Sequences, patch: Sequences):
    __apply_patch(base["pre_animseqs"], patch["pre_animseqs"])
    __apply_patch(base["objseqs"], patch["objseqs"])
    # __apply_patch(base["post_animseqs"], patch["post_animseqs"])

def __apply_patch(base: list, patch: list):
    for i, element in enumerate(patch):
        if i >= len(base):
            # New
            base.append(element)
            continue
        
        if element == None:
            # Not defined in patch
            continue

        base[i] = deepcopy(element)

def seqs_write_bin(
        seqs: Sequences,
        objseq_tab: BufferedWriter,
        objseq_bin: BufferedWriter,
        objseq2curve_tab: BufferedWriter, 
        animcurves_tab: BufferedWriter,
        animcurves_bin: BufferedWriter):
    animcurve_idx = 0

    for curve in seqs["pre_animseqs"]:
        __write_anim_curve(curve, animcurves_tab, animcurves_bin)
        animcurve_idx += 1

    last_objseq_with_actors = 0
    for i, seq in enumerate(seqs["objseqs"]):
        if seq != None and len(seq["actors"]) != 0:
            last_objseq_with_actors = i

    for i, seq in enumerate(seqs["objseqs"]):
        if (seq != None and len(seq["actors"]) > 0) or i <= last_objseq_with_actors:
            objseq_tab.write(struct.pack(">H", objseq_bin.tell() // 8))
            if seq != None:
                for actor in seq["actors"]:
                    objseq_bin.write(struct.pack(">IHH", actor["uid"], actor["settings"], unsigned16(actor["objectID"])))

        objseq2curve_tab.write(struct.pack(">H", animcurve_idx))
        if seq != None:
            for curve in seq["curves"]:
                __write_anim_curve(curve, animcurves_tab, animcurves_bin)
                animcurve_idx += 1

    objseq_tab.write(struct.pack(">HH", objseq_bin.tell() // 8, 0xFFFF))
    objseq2curve_tab.write(struct.pack(">HH", animcurve_idx, 0xFFFF))

    for curve in seqs["post_animseqs"]:
        __write_anim_curve(curve, animcurves_tab, animcurves_bin)
        animcurve_idx += 1
    
    animcurves_tab.write(struct.pack(">II", 0xFFFFFFFF, 0xFFFFFFFF))

def __write_anim_curve(curve: Curve | None, animcurves_tab: BufferedWriter, animcurves_bin: BufferedWriter):
    bin_offset = animcurves_bin.tell()

    if curve == None:
        animcurves_tab.write(struct.pack(">HHI", 0, 0, bin_offset))
        return

    file_size = (len(curve["events"]) * 4) + (len(curve["keyframes"]) * 8)

    animcurves_tab.write(struct.pack(">HHI", file_size, len(curve["events"]), bin_offset))

    for event in curve["events"]:
        assert len(event) == 4
        animcurves_bin.write(event)
    for kf in curve["keyframes"]:
        interp = ((kf["weight"] & 0b111111) << 2) | (kf["interp_type"] & 0b11)
        animcurves_bin.write(struct.pack(">fbbh", kf["value"], signed8(interp), kf["channel"], signed16(kf["time"])))

def seqs_from_bin(
        objseq_tab: BufferedReader,
        objseq_bin: BufferedReader,
        objseq2curve_tab: BufferedReader, 
        animcurves_tab: BufferedReader,
        animcurves_bin: BufferedReader) -> Sequences:
    objseq_tab.seek(0, os.SEEK_END)
    objseq_count = (objseq_tab.tell() // 2) - 2
    objseq_tab.seek(0)

    objseq2curve_count = 0
    while True:
        idx = struct.unpack(">H", objseq2curve_tab.read(2))[0]
        if idx == 0xFFFF:
            break
        objseq2curve_count += 1
    objseq2curve_count -= 1
    objseq2curve_tab.seek(0)

    animcurves_tab.seek(0, os.SEEK_END)
    animcurves_count = (animcurves_tab.tell() // 8) - 1
    animcurves_tab.seek(0)

    objseq_curves_start = struct.unpack(">H", objseq2curve_tab.read(2))[0]
    objseq2curve_tab.seek(0)
    standalone_curves_count = objseq_curves_start

    pre_animseqs: list[Curve | None] = []
    for _ in range(standalone_curves_count):
        pre_animseqs.append(__read_next_anim_curve(animcurves_tab, animcurves_bin))
    
    seqs: list[ObjSeq | None] = []
    for i in range(objseq2curve_count):
        actors: list[Actor] = []

        if i < objseq_count:
            start_offset, end_offset = struct.unpack(">HH", objseq_tab.read(4))
            objseq_tab.seek(-2, os.SEEK_CUR)  
            num_actors = end_offset - start_offset
            start_offset *= 8

            objseq_bin.seek(start_offset)

            for _ in range(num_actors):
                uid, settings, objectID = struct.unpack(">IHH", objseq_bin.read(8))

                actors.append({ "uid": uid, "settings": settings, "objectID": objectID })

        curves_start_idx, curves_end_idx = struct.unpack(">HH", objseq2curve_tab.read(4))
        objseq2curve_tab.seek(-2, os.SEEK_CUR)
        num_curves = curves_end_idx - curves_start_idx

        animcurves_tab.seek(curves_start_idx * 8)

        curves: list[Curve] = []
        for _ in range(num_curves):
            curves.append(__read_next_anim_curve(animcurves_tab, animcurves_bin))

        seqs.append({ "actors": actors, "curves": curves })
    
    animcurve_idx = animcurves_tab.tell() // 8
    post_animseqs: list[Curve | None] = []
    for _ in range(animcurves_count - animcurve_idx):
        post_animseqs.append(__read_next_anim_curve(animcurves_tab, animcurves_bin))

    return { "objseqs": seqs, "pre_animseqs": pre_animseqs, "post_animseqs": post_animseqs }

def __read_next_anim_curve(animcurves_tab: BufferedReader, animcurves_bin: BufferedReader) -> Curve:
    file_size, event_count, bin_offset = struct.unpack(">HHI", animcurves_tab.read(8))

    animcurves_bin.seek(bin_offset)

    events: list[bytes] = []
    for _ in range(event_count):
        evt = animcurves_bin.read(4)
        events.append(evt)
    
    keyframes: list[Keyframe] = []
    keyframe_count = (file_size - (event_count * 4)) // 8
    for _ in range(keyframe_count):
        value, interp, channel, time = struct.unpack(">fbbh", animcurves_bin.read(8))
        interp_type = interp & 0b11
        weight = (interp >> 2) & 0b111111
        keyframes.append({ "value": value, "interp_type": interp_type, "weight": weight, "channel": channel, "time": time })

    return { "events": events, "keyframes": keyframes }

def __write_events_yaml(events: list[bytes], output: TextIO, indent: str):
    time_offset = 0
    k = 0
    num_events = len(events)
    while k < num_events:
        event = events[k]
        evt_type, delay, params = __parse_event(event)
        evt_name = event_names.get(evt_type, "<unknown>")
        comment = f"{evt_name} @ {time_offset}"
        if evt_type == 13: # ENVFX
            envfx_type = (params >> 0xC) & 0xF
            envfx_val = params & 0xFFF
            comment += f" ({envfx_names.get(envfx_type, '<unknown>')}, 0x{envfx_val:X})"

        if evt_type == -1 or evt_type == 0 or evt_type == 11:
            output.write(f"{indent}- {{ type: {evt_type}, delay: {delay}, params: {params} }} # {comment}\n")
        else:
            output.write(f"{indent}- {{ type: {evt_type}, delay: {delay}, params: 0x{unsigned16(params):X} }} # {comment}\n")
        k += 1

        if evt_type == 0: # settime event
            time_offset = params
        elif evt_type != 15: # sfx_with_duration event
            time_offset += delay

        if evt_type == 11: # code event
            j = 0
            while j < params and k < num_events:
                code_evt = events[k]
                code_evt_type, val1, val2 = __parse_code_event(code_evt)
                
                code_evt_name = code_event_names.get(code_evt_type, "<unknown>")
                comment = code_evt_name
                if code_evt_type == 2:
                    comment += f" ({code_set_types.get(val1, 'unknown')})"
                elif code_evt_type == 6:
                    evt_6_type = val1 & 0xFF
                    comment += f" ({code_event_6_names.get(evt_6_type, '<unknown>')})"
                output.write(f"{indent}- {{ code_type: {code_evt_type}, val1: 0x{val1:X}, val2: 0x{val2:X} }} # {comment}\n")

                k += 1
                j += 1

def obj_seq_write_yaml(seq_id: int, seq: ObjSeq, output: TextIO, obj_info: ObjSeqObjInfo):
    output.write("actors:\n")
    for i, actor in enumerate(seq["actors"]):
        output.write(f"# Actor {i}\n")
        output.write(f"- uid: 0x{actor['uid']:X}\n")
        output.write(f"  settings: 0x{actor['settings']:X}\n")
        comment: str | None = None
        if actor["objectID"] == 0xFFFF:
            comment = "Self"

            self_id_set = obj_info.seq_to_id.get(seq_id)
            if self_id_set != None:
                names: list[str] = []
                for self_id in self_id_set:
                    self_name = obj_info.id_to_name.get(self_id)
                    if self_name == None:
                        names.append(f"0x{self_id:X}")
                    else:
                        names.append(self_name)
                comment += f" ({", ".join(names)})"
        elif actor["objectID"] == 0xFFFE:
            comment = "Camera"
        else:
            obj_name = obj_info.id_to_name.get(actor["objectID"])
            if obj_name != None:
                comment = obj_name
        if comment != None:
            output.write(f"  objectID: 0x{actor['objectID']:X} # {comment}\n")
        else:
            output.write(f"  objectID: 0x{actor['objectID']:X}\n")
    
    output.write("\ncurves:\n")
    for i, curve in enumerate(seq["curves"]):
        output.write(f"# Curve {i}\n")
        output.write("- events:\n")
        __write_events_yaml(curve["events"], output, "  ")

        output.write("  keyframes:\n")
        last_channel = -1
        for kf in curve["keyframes"]:
            if kf['channel'] != last_channel:
                last_channel = kf['channel']
                output.write(f"    # {channel_names.get(kf['channel'], '<unknown>')}\n")
            output.write(f"  - {{ channel: {kf['channel']}, time: {kf['time']}, value: {kf['value']}, interp: {kf['interp_type']}, weight: {kf['weight']} }}\n")

def anim_curve_write_yaml(curve: Curve, output: TextIO):
    output.write("events:\n")
    __write_events_yaml(curve["events"], output, "")
    output.write("keyframes:\n")
    for kf in curve["keyframes"]:
        output.write(f"- {{ channel: {kf['channel']}, time: {kf['time']}, value: {kf['value']}, interp: {kf['interp_type']}, weight: {kf['weight']} }}\n")

def obj_seq_from_yaml(seq_yaml: Any) -> ObjSeq:
    actors: list[Actor] = []
    if seq_yaml["actors"] != None:
        for actor_yaml in seq_yaml["actors"]:
            if "unk5" in actor_yaml:
                # Old format
                actors.append({
                    "uid": int(actor_yaml["uid"]),
                    "settings": (int(actor_yaml["settings"]) << 8) | int(actor_yaml["unk5"]),
                    "objectID": int(actor_yaml["objectID"])
                })
            else:
                actors.append({
                    "uid": int(actor_yaml["uid"]),
                    "settings": int(actor_yaml["settings"]),
                    "objectID": int(actor_yaml["objectID"])
                })

    curves: list[Curve] = []
    if seq_yaml["curves"] != None:
        for curve_yaml in seq_yaml["curves"]:
            curves.append(anim_curve_from_yaml(curve_yaml))

    return { "actors": actors, "curves": curves }

def anim_curve_from_yaml(curve_yaml: Any) -> Curve:
    events: list[bytes] = []
    if curve_yaml["events"] != None:
        for event_yaml in curve_yaml["events"]:
            if "code_type" in event_yaml:
                events.append(__pack_code_event(
                    int(event_yaml["code_type"]),
                    int(event_yaml["val1"]),
                    int(event_yaml["val2"])
                ))
            else:
                events.append(__pack_event(
                    int(event_yaml["type"]),
                    int(event_yaml["delay"]),
                    int(event_yaml["params"])
                ))
        
    keyframes: list[Keyframe] = []
    if curve_yaml["keyframes"] != None:
        for kf_yaml in curve_yaml["keyframes"]:
            keyframes.append({
                "value": float(kf_yaml["value"]),
                "interp_type": int(kf_yaml["interp"]),
                "weight": int(kf_yaml["weight"]),
                "channel": int(kf_yaml["channel"]),
                "time": int(kf_yaml["time"])
            })
    
    return { "events": events, "keyframes": keyframes }

def seqs_dump_to_dir(dir: Path, seqs: Sequences, obj_info: ObjSeqObjInfo):
    animseqs_dir = dir.joinpath("animseqs")
    
    if len(seqs["pre_animseqs"]) > 0:
        animseqs_dir.mkdir(exist_ok=True)
        pre_dir = animseqs_dir.joinpath("pre")
        pre_dir.mkdir(exist_ok=True)

        for i, curve in enumerate(seqs["pre_animseqs"]):
            if curve == None:
                continue
            filename = f"{i:04} {i:04X}.yaml"
            with open(pre_dir.joinpath(filename), "w", encoding="utf-8") as curve_file:
                anim_curve_write_yaml(curve, curve_file)
    
    if len(seqs["objseqs"]) > 0:
        objseqs_dir = dir.joinpath("objseqs")
        objseqs_dir.mkdir(exist_ok=True)

        for i, seq in enumerate(seqs["objseqs"]):
            if seq == None:
                continue
            filename = f"{i:04} {i:04X}.yaml"
            with open(objseqs_dir.joinpath(filename), "w", encoding="utf-8") as seq_file:
                obj_seq_write_yaml(i, seq, seq_file, obj_info)
    
    if len(seqs["post_animseqs"]) > 0:
        animseqs_dir.mkdir(exist_ok=True)
        post_dir = animseqs_dir.joinpath("post")
        post_dir.mkdir(exist_ok=True)

        for i, curve in enumerate(seqs["post_animseqs"]):
            if curve == None:
                continue
            filename = f"{i:04} {i:04X}.yaml"
            with open(post_dir.joinpath(filename), "w", encoding="utf-8") as curve_file:
                anim_curve_write_yaml(curve, curve_file)

def __parse_filename(filename: str):
    # Expects filename to be something like "0010 000A.yaml"
    id_str = filename.split(" ")[0].lstrip("0")
    if len(id_str) == 0:
        return 0
    
    return int(id_str)

def seqs_from_directory(dir: Path) -> Sequences:
    pre_animseqs: list[Curve | None] = []
    post_animseqs: list[Curve | None] = []

    animseqs_dir = dir.joinpath("animseqs")
    if animseqs_dir.exists():
        pre_dir = animseqs_dir.joinpath("pre")
        post_dir = animseqs_dir.joinpath("post")

        if pre_dir.exists():
            curve_map: dict[int, Curve] = {}
            highest_id = 0
            for p in pre_dir.glob("*.yaml"):
                id = __parse_filename(p.name)
                highest_id = max(highest_id, id)
                with open(p, "r") as curve_file:
                    curve_map[id] = anim_curve_from_yaml(yaml.safe_load(curve_file))

            for i in range(highest_id + 1):
                pre_animseqs.append(curve_map.get(i))
        
        if post_dir.exists():
            curve_map: dict[int, Curve] = {}
            highest_id = 0
            for p in post_dir.glob("*.yaml"):
                id = __parse_filename(p.name)
                highest_id = max(highest_id, id)
                with open(p, "r") as curve_file:
                    curve_map[id] = anim_curve_from_yaml(yaml.safe_load(curve_file))

            for i in range(highest_id + 1):
                post_animseqs.append(curve_map.get(i))

    seq_map: dict[int, ObjSeq] = {}
    highest_id = 0
    for p in dir.joinpath("objseqs").glob("*.yaml"):
        id = __parse_filename(p.name)
        highest_id = max(highest_id, id)
        with open(p, "r") as seq_file:
            seq_map[id] = obj_seq_from_yaml(yaml.safe_load(seq_file))
    
    seqs: list[ObjSeq | None] = []
    for i in range(highest_id + 1):
        seqs.append(seq_map.get(i))
    
    return { "objseqs": seqs, "pre_animseqs": pre_animseqs, "post_animseqs": post_animseqs }
