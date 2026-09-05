#!/usr/bin/env python3
"""Build the R1Q2v2 desktop tester zip (binaries + visual cfgs + OpenAL runtime)."""

from __future__ import annotations

import os
import shutil
import sys
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")
ZIP_NAME = "R1Q2v2-8012-win32.zip"

BINARIES = (
    "R1Q2v2.exe",
    "r1q2ded.exe",
    "gamex86.dll",
    "ref_r1gl.dll",
    "ref_gl.dll",
    "jpeg62.dll",
    "libpng16.dll",
    "z.dll",
    "OpenAL32.dll",
    "fmt.dll",  # OpenAL Soft (vcpkg) LoadLibrary dependency
)

# HRTF / speaker presets for OpenAL Soft. Do not ship Q2PRO-X als oft.ini
# (binaural cvars) or 64-bit soft_oal.dll — R1Q2v2 is Win32 and loads OpenAL32.dll.

# Runtime data for OpenAL Soft (HRTF + presets). Skip als oft-config (Qt GUI).
OAL_DIRS = (
    "oal/hrtf",
    "oal/hrtf_defs",
    "oal/presets",
)

CFG_FILES = (
    "baseq2/r1q2v2_visual.cfg",
    "baseq2/pretty_r1q2v2.cfg",
)

README = """\
R1Q2v2 (8012) — Windows Win32 test build
=================================================

Requires a normal Quake II install (Steam is fine). This zip is binaries,
visual presets, and OpenAL Soft — not the full game.

Install
-------
1. Copy everything in this folder into your Quake II directory (same folder
   as quake2.exe / your existing paks).
2. Overwrite when asked (back up first if you care about stock quake2.exe).
3. Launch R1Q2v2.exe.

Included
--------
- R1Q2v2.exe, r1q2ded.exe, gamex86.dll
- ref_r1gl.dll, ref_gl.dll (R1GL renderer)
- z.dll, libpng16.dll, jpeg62.dll (PNG/JPG support)
- OpenAL32.dll + fmt.dll (OpenAL Soft; R1Q2 loads OpenAL32.dll)
- oal/hrtf/, oal/presets/ (OpenAL Soft HRTF data, optional)
- baseq2/r1q2v2_visual.cfg — 1080p-friendly defaults
- baseq2/pretty_r1q2v2.cfg — alternate visual preset (R1-safe cvars only)

OpenAL audio
------------
Build must be compiled with USE_OPENAL (CMake R1Q2_USE_OPENAL=ON).
In autoexec or visual cfg: seta s_initsound "2"
Optional: seta s_openal_device ""  (empty = default output device)

Quick test
----------
  R1Q2v2.exe +exec r1q2v2_visual.cfg

Docs: https://github.com/RENEGADE-ANDROiD/R1Q2v2 (README.md, CVARS.md)
"""


def desktop_path() -> Path:
    one_drive = os.environ.get("OneDrive")
    if one_drive:
        desk = Path(one_drive) / "Desktop"
        if desk.is_dir():
            return desk / ZIP_NAME
    return Path.home() / "Desktop" / ZIP_NAME


def find_binary(name: str) -> Path | None:
    for root in (REPO / "build" / "bin", Q2):
        path = root / name
        if path.is_file():
            return path
    return None


def stage_oal(pkg: Path, q2: Path) -> list[str]:
    """Copy OpenAL Soft HRTF/preset data. DLLs come from BINARIES (Win32 vcpkg)."""
    staged: list[str] = []

    for rel in OAL_DIRS:
        src_dir = q2 / rel
        if not src_dir.is_dir():
            continue
        dest_dir = pkg / rel
        dest_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(src_dir, dest_dir, dirs_exist_ok=True)
        count = sum(1 for _ in dest_dir.rglob("*") if _.is_file())
        staged.append(f"{rel}/ ({count} files)")

    return staged


def main() -> int:
    stage_root = Path(os.environ.get("TEMP", ".")) / "R1Q2v2-package"
    if stage_root.is_dir():
        shutil.rmtree(stage_root)
    pkg = stage_root / "R1Q2v2"
    pkg.mkdir(parents=True)

    copied: list[str] = []
    missing: list[str] = []

    for name in BINARIES:
        src = find_binary(name)
        if src is None:
            missing.append(name)
            continue
        shutil.copy2(src, pkg / name)
        copied.append(f"{name} <- {src.parent.name}")

    for rel in CFG_FILES:
        for root in (REPO, Q2):
            src = root / rel
            if src.is_file():
                dest = pkg / rel
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dest)
                copied.append(rel)
                break

    oal = stage_oal(pkg, Q2)
    (pkg / "README.txt").write_text(README, encoding="utf-8")

    zip_path = desktop_path()
    if zip_path.is_file():
        zip_path.unlink()
    stale = zip_path.with_name("R1Q2v2-8012-RENEGADE-win32.zip")
    if stale.is_file():
        stale.unlink()

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for path in sorted(pkg.rglob("*")):
            if path.is_file():
                zf.write(path, path.relative_to(stage_root).as_posix())

    print(f"zip: {zip_path} ({zip_path.stat().st_size // 1024} KiB)")
    print(f"binaries: {len([c for c in copied if not c.startswith('baseq2') and 'oal' not in c.lower()])}")
    for line in copied:
        print(f"  {line}")
    print("openal:")
    for line in oal:
        print(f"  {line}")
    if missing:
        print("missing binaries:", ", ".join(missing), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
