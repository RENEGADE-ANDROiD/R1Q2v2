#!/usr/bin/env python3
"""Extract Quake Live railgf1a.ogg, convert to Q2 WAV, patch pak9 copies."""

from __future__ import annotations

import struct
import subprocess
import sys
import zipfile
from pathlib import Path

import imageio_ffmpeg

FFMPEG = imageio_ffmpeg.get_ffmpeg_exe()
QL_PK3 = Path(r"E:\SteamLibrary\steamapps\common\Quake Live\baseq3\pak00.pk3")
QL_OGG = "sound/weapons/railgun/railgf1a.ogg"
PAK_NAME = "sound/weapons/railgf1a.wav"
PAK_IDENT = b"PACK"
NAME_LEN = 56

DESKTOP = Path.home() / "OneDrive" / "Desktop"
if not DESKTOP.is_dir():
    DESKTOP = Path.home() / "Desktop"
WORK = DESKTOP / "pak9-sounds" / "_convert"
PREVIEW = DESKTOP / "pak9-sounds" / "sound" / "weapons" / "railgf1a.wav"

Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")
TARGETS = [
    Q2 / "baseq2" / "pak9.pak",
    Q2 / "OTHER PAK's" / "altpak9.pak",
    Q2 / "jugfull" / "pak9.pak",
]


def to_q2_wav(src: Path, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        FFMPEG,
        "-y",
        "-i",
        str(src),
        "-ac",
        "1",
        "-ar",
        "44100",
        "-c:a",
        "pcm_s16le",
        str(dest),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(f"ffmpeg failed for {src}:\n{r.stderr}")


def read_entries(path: Path):
    data = path.read_bytes()
    if data[:4] != PAK_IDENT:
        raise SystemExit(f"not a pak: {path}")
    dirofs, dirlen = struct.unpack_from("<II", data, 4)
    out = []
    for i in range(dirlen // 64):
        off = dirofs + i * 64
        raw_name = data[off : off + NAME_LEN]
        name = raw_name.split(b"\0", 1)[0].decode("ascii", "replace")
        pos, size = struct.unpack_from("<II", data, off + NAME_LEN)
        out.append((raw_name, data[pos : pos + size], name))
    return out


def write_pak(path: Path, entries) -> int:
    buf = bytearray(12)
    dir_bytes = bytearray()
    for raw_name, blob, _name in entries:
        pos = len(buf)
        buf.extend(blob)
        name = raw_name[:NAME_LEN].ljust(NAME_LEN, b"\0")
        dir_bytes.extend(name)
        dir_bytes.extend(struct.pack("<II", pos, len(blob)))
    dirofs = len(buf)
    buf.extend(dir_bytes)
    buf[0:4] = PAK_IDENT
    buf[4:12] = struct.pack("<II", dirofs, len(dir_bytes))
    tmp = path.with_name(path.name + ".tmp_rail")
    tmp.write_bytes(bytes(buf))
    tmp.replace(path)
    return len(buf)


def main() -> int:
    if not QL_PK3.is_file():
        print(f"missing {QL_PK3}", file=sys.stderr)
        return 1

    WORK.mkdir(parents=True, exist_ok=True)
    raw_ogg = WORK / "railgf1a.ogg"
    out_wav = WORK / "railgf1a.wav"

    with zipfile.ZipFile(QL_PK3) as z:
        if QL_OGG not in z.namelist():
            print(f"missing {QL_OGG} in {QL_PK3}", file=sys.stderr)
            return 1
        raw_ogg.write_bytes(z.read(QL_OGG))
    print(f"extracted QL: {QL_OGG} ({raw_ogg.stat().st_size} bytes)")

    to_q2_wav(raw_ogg, out_wav)
    blob = out_wav.read_bytes()
    print(f"converted: {PAK_NAME} ({len(blob)} bytes, 44100 Hz mono PCM)")

    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    PREVIEW.write_bytes(blob)
    print(f"preview: {PREVIEW}")

    key = PAK_NAME.lower()
    for pak in TARGETS:
        if not pak.is_file():
            print(f"missing {pak}", file=sys.stderr)
            continue
        entries = read_entries(pak)
        new_entries = []
        replaced = False
        for raw_name, old_blob, name in entries:
            nk = name.replace("\\", "/").lower()
            if nk == key:
                print(f"  replace {name}: {len(old_blob)} -> {len(blob)}")
                new_entries.append((raw_name, blob, name))
                replaced = True
            else:
                new_entries.append((raw_name, old_blob, name))
        if not replaced:
            raw = PAK_NAME.encode("ascii").ljust(NAME_LEN, b"\0")
            new_entries.append((raw, blob, PAK_NAME))
            print(f"  add {PAK_NAME} ({len(blob)} bytes)")
        size = write_pak(pak, new_entries)
        print(f"{pak}: {len(new_entries)} files, {size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
