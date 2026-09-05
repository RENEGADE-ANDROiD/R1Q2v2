#!/usr/bin/env python3
"""Copy baseq2 autoexec.cfg + config.cfg to all Quake 2 gamedir folders."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")
SRC = Q2 / "baseq2"

SKIP_DIRS = {
    "baseq2",
    "crash_analysis",
    "docs",
    "oal",
    "OTHER PAK's",
    "rerelease",
}


def targets() -> list[Path]:
    out: list[Path] = []
    for child in sorted(Q2.iterdir()):
        if not child.is_dir() or child.name in SKIP_DIRS:
            continue
        out.append(child)
    return out


def main() -> int:
    files = ("autoexec.cfg", "config.cfg")
    for name in files:
        if not (SRC / name).is_file():
            print(f"missing source {SRC / name}", file=sys.stderr)
            return 1

    copied: list[str] = []
    for dest_dir in targets():
        for name in files:
            src = SRC / name
            dest = dest_dir / name
            shutil.copy2(src, dest)
            copied.append(f"{dest_dir.name}/{name}")

    print(f"copied from {SRC} -> {len(targets())} gamedirs ({len(copied)} files)")
    for item in copied:
        print(f"  {item}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
