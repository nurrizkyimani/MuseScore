#!/usr/bin/env python3
"""Convert an exact-key SFZ map into a small SoundFont 2 preset.

The converter intentionally supports the subset used by the Gamelan maps:
one sample per exact MIDI key, optional velocity ranges, and ordinary WAV
files.  It keeps stereo WAV files as linked left/right SoundFont samples and
adds the SFZ envelope values used by the pilot maps.
"""

from __future__ import annotations

import argparse
import math
import struct
import wave
from dataclasses import dataclass
from pathlib import Path


GUARD_SAMPLES = 46


@dataclass(frozen=True)
class Region:
    sample: Path
    key: int
    pitch_keycenter: int
    lokey: int
    hikey: int
    lovel: int | None = None
    hivel: int | None = None


def pack_name(value: str, size: int = 20) -> bytes:
    encoded = value.encode("utf-8")[:size]
    while encoded:
        try:
            encoded.decode("utf-8")
            break
        except UnicodeDecodeError:
            encoded = encoded[:-1]
    return encoded + b"\0" * (size - len(encoded))


def chunk(tag: bytes, payload: bytes) -> bytes:
    result = tag + struct.pack("<I", len(payload)) + payload
    if len(payload) & 1:
        result += b"\0"
    return result


def list_chunk(list_type: bytes, children: list[bytes]) -> bytes:
    return chunk(b"LIST", list_type + b"".join(children))


def parse_region(line: str, root: Path) -> Region:
    values: dict[str, str] = {}
    for token in line.split()[1:]:
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        values[key] = value

    required = ("sample", "key", "pitch_keycenter", "lokey", "hikey")
    missing = [key for key in required if key not in values]
    if missing:
        raise ValueError(f"SFZ region is missing {', '.join(missing)}: {line}")

    numeric = {key: int(values[key]) for key in required[1:]}
    if not (numeric["key"] == numeric["pitch_keycenter"] == numeric["lokey"] == numeric["hikey"]):
        raise ValueError(f"Only exact-key regions are supported: {line}")

    sample_path = root / values["sample"]
    if not sample_path.is_file():
        raise FileNotFoundError(sample_path)

    return Region(
        sample=sample_path,
        key=numeric["key"],
        pitch_keycenter=numeric["pitch_keycenter"],
        lokey=numeric["lokey"],
        hikey=numeric["hikey"],
        lovel=int(values["lovel"]) if "lovel" in values else None,
        hivel=int(values["hivel"]) if "hivel" in values else None,
    )


def parse_sfz(path: Path) -> tuple[list[Region], dict[str, float]]:
    regions: list[Region] = []
    globals_: dict[str, float] = {}
    current_scope: str | None = None
    with path.open("r", encoding="utf-8") as stream:
        for raw_line in stream:
            line = raw_line.split("//", 1)[0].strip()
            if not line:
                continue
            if line.startswith("<"):
                current_scope = line.split(">", 1)[0] + ">"
                if current_scope == "<region>":
                    regions.append(parse_region(line, path.parent))
                continue
            if current_scope == "<global>":
                for token in line.split():
                    if "=" in token:
                        key, value = token.split("=", 1)
                        try:
                            globals_[key] = float(value)
                        except ValueError:
                            pass

    if not regions:
        raise ValueError(f"No regions found in {path}")
    regions.sort(key=lambda region: (region.key, region.lovel if region.lovel is not None else 0))
    # Multiple regions may share a key when they are separate velocity
    # layers. Reject only overlapping layers; overlapping zones would make
    # playback choose a sample unpredictably.
    by_key: dict[int, list[Region]] = {}
    for region in regions:
        by_key.setdefault(region.key, []).append(region)
    for key, key_regions in by_key.items():
        if len(key_regions) == 1:
            continue
        ranges: list[tuple[int, int]] = []
        for region in key_regions:
            if region.lovel is None and region.hivel is None:
                raise ValueError(f"Duplicate MIDI key {key} has no velocity ranges")
            low = region.lovel if region.lovel is not None else 0
            high = region.hivel if region.hivel is not None else 127
            ranges.append((low, high))
        ranges.sort()
        for (_, previous_high), (current_low, _) in zip(ranges, ranges[1:]):
            if current_low <= previous_high:
                raise ValueError(f"Overlapping velocity ranges for MIDI key {key}")
    return regions, globals_


def timecents(seconds: float) -> int:
    if seconds <= 0:
        return -32768
    return max(-32768, min(32767, round(1200.0 * math.log2(seconds))))


def read_wav(path: Path) -> tuple[int, list[list[int]]]:
    with wave.open(str(path), "rb") as stream:
        channels = stream.getnchannels()
        sample_width = stream.getsampwidth()
        rate = stream.getframerate()
        frames = stream.getnframes()
        raw = stream.readframes(frames)

    if channels not in (1, 2) or sample_width != 2:
        raise ValueError(f"Expected 16-bit mono/stereo WAV: {path}")

    values = list(struct.unpack("<" + "h" * (len(raw) // 2), raw))
    if channels == 1:
        return rate, [values]
    return rate, [values[0::2], values[1::2]]


def make_sf2(regions: list[Region], globals_: dict[str, float], preset_name: str) -> bytes:
    sample_data = bytearray()
    sample_headers: list[dict[str, int | str]] = []
    zone_data: list[tuple[Region, int, int]] = []

    for region in regions:
        rate, channels = read_wav(region.sample)
        first_sample_id = len(sample_headers)
        starts: list[tuple[int, int]] = []
        for channel in channels:
            start = len(sample_data) // 2
            sample_data.extend(struct.pack("<" + "h" * len(channel), *channel))
            sample_data.extend(b"\0\0" * GUARD_SAMPLES)
            end = start + len(channel)
            starts.append((start, end))

        if len(channels) == 1:
            start, end = starts[0]
            sample_headers.append({
                "name": f"GB_{region.key}_M",
                "start": start,
                "end": end,
                "startloop": start,
                "endloop": end,
                "rate": rate,
                "pitch": region.pitch_keycenter,
                "link": 0,
                "type": 1,
            })
        else:
            left_start, left_end = starts[0]
            right_start, right_end = starts[1]
            left_id = first_sample_id
            right_id = first_sample_id + 1
            sample_headers.extend([
                {
                    "name": f"GB_{region.key}_L",
                    "start": left_start,
                    "end": left_end,
                    "startloop": left_start,
                    "endloop": left_end,
                    "rate": rate,
                    "pitch": region.pitch_keycenter,
                    "link": right_id,
                    "type": 4,
                },
                {
                    "name": f"GB_{region.key}_R",
                    "start": right_start,
                    "end": right_end,
                    "startloop": right_start,
                    "endloop": right_end,
                    "rate": rate,
                    "pitch": region.pitch_keycenter,
                    "link": left_id,
                    "type": 2,
                },
            ])
        zone_data.append((region, first_sample_id, len(channels)))

    # INFO list.
    info_name = preset_name.encode("utf-8") + b"\0"
    # SoundFont INFO string chunks must have an even payload size. RIFF
    # permits an external pad byte, but FluidSynth rejects an odd INAM chunk
    # length, so include the terminating padding inside the chunk payload.
    if len(info_name) & 1:
        info_name += b"\0"

    info = list_chunk(b"INFO", [
        chunk(b"ifil", struct.pack("<HH", 2, 1)),
        chunk(b"isng", b"EMU8000\0"),
        chunk(b"INAM", info_name),
    ])

    sdta = list_chunk(b"sdta", [chunk(b"smpl", bytes(sample_data))])

    # One preset points to one instrument. The preset instrument generator is
    # the only pgen record, and all note-specific generators live in the
    # instrument zones below.
    phdr = b"".join([
        struct.pack("<20sHHHIII", pack_name(preset_name), 0, 0, 0, 0, 0, 0),
        struct.pack("<20sHHHIII", pack_name("EOP"), 0, 0, 1, 0, 0, 0),
    ])
    pbag = struct.pack("<HH", 0, 0) + struct.pack("<HH", 1, 0)
    pmod = struct.pack("<HHhHH", 0, 0, 0, 0, 0)
    pgen = struct.pack("<Hh", 41, 0)
    # Instrument bag 0 is a global envelope zone. Each following bag is one
    # exact-key sample region. The final bag is the required terminal record.
    ibag_records: list[tuple[int, int]] = [(0, 0)]
    global_generators: list[tuple[int, int]] = []
    if "ampeg_attack" in globals_:
        global_generators.append((34, timecents(globals_["ampeg_attack"])))
    if "rt_decay" in globals_:
        global_generators.append((36, timecents(globals_["rt_decay"])))
    if "ampeg_release" in globals_:
        global_generators.append((38, timecents(globals_["ampeg_release"])))

    igen_records: list[tuple[int, int]] = list(global_generators)
    current_generator_index = len(igen_records)
    ibag_records[0] = (0, 0)
    for region, sample_id, _channels in zone_data:
        ibag_records.append((current_generator_index, 0))
        key_range = region.lokey | (region.hikey << 8)
        igen_records.append((43, key_range))
        if region.lovel is not None or region.hivel is not None:
            low_velocity = region.lovel if region.lovel is not None else 0
            high_velocity = region.hivel if region.hivel is not None else 127
            igen_records.append((44, low_velocity | (high_velocity << 8)))
        igen_records.append((53, sample_id))
        current_generator_index += 2 + int(region.lovel is not None or region.hivel is not None)
    ibag_records.append((current_generator_index, 0))

    # The terminal EOI record points at the terminal instrument-bag record,
    # not at the first sample zone.  FluidSynth rejects the SoundFont when
    # these table indices disagree.
    inst = b"".join([
        struct.pack("<20sH", pack_name(preset_name), 0),
        struct.pack("<20sH", pack_name("EOI"), len(ibag_records) - 1),
    ])
    ibag = b"".join(struct.pack("<HH", *record) for record in ibag_records)
    imod = struct.pack("<HHhHH", 0, 0, 0, 0, 0)
    igen = b"".join(struct.pack("<Hh", operator, amount) for operator, amount in igen_records)

    shdr_records = []
    for sample in sample_headers:
        shdr_records.append(struct.pack(
            "<20sIIIIIbbHH",
            pack_name(str(sample["name"])),
            int(sample["start"]),
            int(sample["end"]),
            int(sample["startloop"]),
            int(sample["endloop"]),
            int(sample["rate"]),
            int(sample["pitch"]),
            0,
            int(sample["link"]),
            int(sample["type"]),
        ))
    shdr_records.append(struct.pack("<20sIIIIIbbHH", pack_name("EOS"), 0, 0, 0, 0, 0, 0, 0, 0, 1))
    shdr = b"".join(shdr_records)

    pdta = list_chunk(b"pdta", [
        chunk(b"phdr", phdr),
        chunk(b"pbag", pbag),
        chunk(b"pmod", pmod),
        chunk(b"pgen", pgen),
        chunk(b"inst", inst),
        chunk(b"ibag", ibag),
        chunk(b"imod", imod),
        chunk(b"igen", igen),
        chunk(b"shdr", shdr),
    ])

    body = b"sfbk" + info + sdta + pdta
    return b"RIFF" + struct.pack("<I", len(body)) + body


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sfz", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--name", default="Gendèr Barung Tuning 8.1 Med Diversity")
    args = parser.parse_args()

    regions, globals_ = parse_sfz(args.sfz)
    data = make_sf2(regions, globals_, args.name)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Wrote {args.output} ({len(regions)} regions, {len(data) / 1024 / 1024:.1f} MiB)")


if __name__ == "__main__":
    main()
