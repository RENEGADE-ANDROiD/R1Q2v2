#!/usr/bin/env python3
"""Add 44 kHz sound cvars to Quake 2 autoexec.cfg files."""

from __future__ import annotations

from pathlib import Path

Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")
BLOCK = (
    "\n\n"
    "// Sound (44 kHz / 16-bit for custom pak9 WAVs)\n"
    'seta s_khz "44"\n'
    'seta s_loadas8bit "0"'
)
ANCHOR = 'set r_vsync "0"'


def main() -> int:
    updated: list[str] = []
    skipped: list[str] = []

    for path in sorted(Q2.rglob("autoexec.cfg")):
        if "cfg-backup" in path.parts:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if "s_khz" in text:
            skipped.append(f"{path} (already set)")
            continue
        if ANCHOR not in text:
            skipped.append(f"{path} (no anchor)")
            continue
        path.write_text(text.replace(ANCHOR, ANCHOR + BLOCK, 1), encoding="utf-8")
        updated.append(str(path))

    print(f"updated {len(updated)}")
    for item in updated:
        print(f"  {item}")
    if skipped:
        print(f"skipped {len(skipped)}")
        for item in skipped:
            print(f"  {item}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
