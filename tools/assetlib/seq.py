from __future__ import annotations
from copy import deepcopy
from io import BufferedReader, BufferedWriter
import os
from pathlib import Path
import struct
from typing import Any, TextIO, TypedDict
import pylibyaml
import yaml

class Keyframe(TypedDict):
    value: float
    interp_type: int
    weight: int
    channel: int
    time: int

class Event(TypedDict):
    type: int
    delay: int
    params: int

class Curve(TypedDict):
    events: list[Event]
    keyframes: list[Keyframe]

class Actor(TypedDict):
    uid: int
    settings: int
    unk5: int
    objectID: int

class ObjSeq(TypedDict):
    actors: list[Actor]
    curves: list[Curve]

class Sequences(TypedDict):
    pre_animseqs: list[Curve | None]
    post_animseqs: list[Curve | None]
    objseqs: list[ObjSeq | None]

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
                    objseq_bin.write(struct.pack(">IBBh", actor["uid"], actor["settings"], actor["unk5"], actor["objectID"]))

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
        animcurves_bin.write(struct.pack(">BBH", event["type"], event["delay"], event["params"]))
    for kf in curve["keyframes"]:
        interp = ((kf["weight"] & 0b111111) << 2) | (kf["interp_type"] & 0b11)
        animcurves_bin.write(struct.pack(">fBBH", kf["value"], interp, kf["channel"], kf["time"]))

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
                uid, settings, unk5, objectID = struct.unpack(">IBBh", objseq_bin.read(8))

                actors.append({ "uid": uid, "settings": settings, "unk5": unk5, "objectID": objectID })

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

    events: list[Event] = []
    for _ in range(event_count):
        evt_type, delay, params = struct.unpack(">BBH", animcurves_bin.read(4))
        events.append({ "type": evt_type, "delay": delay, "params": params })
    
    keyframes: list[Keyframe] = []
    keyframe_count = (file_size - (event_count * 4)) // 8
    for _ in range(keyframe_count):
        value, interp, channel, time = struct.unpack(">fBBH", animcurves_bin.read(8))
        interp_type = interp & 0b11
        weight = (interp >> 2) & 0b111111
        keyframes.append({ "value": value, "interp_type": interp_type, "weight": weight, "channel": channel, "time": time })

    return { "events": events, "keyframes": keyframes }

def obj_seq_write_yaml(seq: ObjSeq, output: TextIO):
    output.write("actors:\n")
    for i, actor in enumerate(seq["actors"]):
        output.write(f"# Actor {i}\n")
        output.write(f"- uid: 0x{actor['uid']:X}\n")
        output.write(f"  settings: 0x{actor['settings']:X}\n")
        output.write(f"  unk5: 0x{actor['unk5']:X}\n")
        if actor['objectID'] < 0:
            output.write(f"  objectID: {actor['objectID']}\n")
        else:
            output.write(f"  objectID: 0x{actor['objectID']:X}\n")
    
    output.write("\ncurves:\n")
    for i, curve in enumerate(seq["curves"]):
        output.write(f"# Curve {i}\n")
        output.write("- events:\n")
        for event in curve["events"]:
            output.write(f"  - {{ type: 0x{event['type']:X}, delay: {event['delay']}, params: 0x{event['params']:X} }}\n")
        output.write("  keyframes:\n")
        for kf in curve["keyframes"]:
            output.write(f"  - {{ channel: 0x{kf['channel']:X}, time: {kf['time']}, value: {kf['value']}, interp: {kf['interp_type']}, weight: {kf['weight']} }}\n")

def anim_curve_write_yaml(curve: Curve, output: TextIO):
    output.write("events:\n")
    for event in curve["events"]:
        output.write(f"- {{ type: 0x{event['type']:X}, delay: {event['delay']}, params: 0x{event['params']:X} }}\n")
    output.write("keyframes:\n")
    for kf in curve["keyframes"]:
        output.write(f"- {{ channel: 0x{kf['channel']:X}, time: {kf['time']}, value: {kf['value']}, interp: {kf['interp_type']}, weight: {kf['weight']} }}\n")

def obj_seq_from_yaml(seq_yaml: Any) -> ObjSeq:
    actors: list[Actor] = []
    if seq_yaml["actors"] != None:
        for actor_yaml in seq_yaml["actors"]:
            actors.append({
                "uid": int(actor_yaml["uid"]),
                "settings": int(actor_yaml["settings"]),
                "unk5": int(actor_yaml["unk5"]),
                "objectID": int(actor_yaml["objectID"])
            })

    curves: list[Curve] = []
    if seq_yaml["curves"] != None:
        for curve_yaml in seq_yaml["curves"]:
            curves.append(anim_curve_from_yaml(curve_yaml))

    return { "actors": actors, "curves": curves }

def anim_curve_from_yaml(curve_yaml: Any) -> Curve:
    events: list[Event] = []
    if curve_yaml["events"] != None:
        for event_yaml in curve_yaml["events"]:
            events.append({
                "type": int(event_yaml["type"]),
                "delay": int(event_yaml["delay"]),
                "params": int(event_yaml["params"])
            })
        
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

def seqs_dump_to_dir(dir: Path, seqs: Sequences):
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
                obj_seq_write_yaml(seq, seq_file)
    
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
