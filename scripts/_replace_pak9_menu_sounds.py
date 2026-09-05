#!/usr/bin/env python3
"""Replace pak9 menu/tele sounds, remove quads; update Desktop preview + pak copies."""

from __future__ import annotations

import struct
import subprocess
import sys
from pathlib import Path

import imageio_ffmpeg

PAK_IDENT = b"PACK"
DIR_ENTRY = 64
NAME_LEN = 56
FFMPEG = imageio_ffmpeg.get_ffmpeg_exe()

DESKTOP = Path.home() / "OneDrive" / "Desktop"
if not DESKTOP.is_dir():
    DESKTOP = Path.home() / "Desktop"
PREVIEW = DESKTOP / "pak9-sounds"

SRC = {
    "sound/misc/menu1.wav": DESKTOP
    / "temp"
    / "ProjectBrutality2022"
    / "SOUNDS"
    / "HUD Sounds"
    / "VisorGarbled.wav",
    "sound/misc/menu2.wav": DESKTOP
    / "temp"
    / "ProjectBrutality2022"
    / "SOUNDS"
    / "HUD Sounds"
    / "HelpNotification.ogg",
    "sound/misc/tele1.wav": DESKTOP
    / "temp"
    / "ProjectBrutality2022"
    / "SOUNDS"
    / "Items"
    / "INVISIBL1.ogg",
}
DROP = {"sound/quad30.wav", "sound/quad60.wav", "sound/weapons/temp.tmp"}

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
    for i in range(dirlen // DIR_ENTRY):
        off = dirofs + i * DIR_ENTRY
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
    tmp = path.with_name(path.name + ".tmp_snd")
    tmp.write_bytes(bytes(buf))
    tmp.replace(path)
    return len(buf)


def main() -> int:
    converted: dict[str, bytes] = {}
    work = PREVIEW / "_convert"
    work.mkdir(parents=True, exist_ok=True)
    for dest_name, src in SRC.items():
        if not src.is_file():
            print(f"missing {src}", file=sys.stderr)
            return 1
        out_wav = work / Path(dest_name).name
        print(f"convert {src.name} -> {dest_name}")
        to_q2_wav(src, out_wav)
        blob = out_wav.read_bytes()
        converted[dest_name] = blob
        # update preview tree
        prev = PREVIEW / dest_name.replace("/", "\\")
        prev.parent.mkdir(parents=True, exist_ok=True)
        prev.write_bytes(blob)
        print(f"  preview {prev} ({len(blob)} bytes)")

    # remove quads from preview
    for q in ("sound/quad30.wav", "sound/quad60.wav"):
        p = PREVIEW / q.replace("/", "\\")
        if p.is_file():
            p.unlink()
            print(f"removed preview {p}")

    for pak in TARGETS:
        if not pak.is_file():
            print(f"missing {pak}", file=sys.stderr)
            continue
        entries = read_entries(pak)
        new_entries = []
        for raw_name, blob, name in entries:
            key = name.replace("\\", "/").lower()
            if key in DROP:
                print(f"  drop {name}")
                continue
            if key in converted:
                blob = converted[key]
                print(f"  replace {name} ({len(blob)} bytes)")
            new_entries.append((raw_name, blob, name))
        size = write_pak(pak, new_entries)
        names = {e[2].replace("\\", "/").lower() for e in new_entries}
        assert "sound/misc/menu1.wav" in names
        assert "sound/quad30.wav" not in names
        print(f"{pak}: {len(new_entries)} files, {size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
