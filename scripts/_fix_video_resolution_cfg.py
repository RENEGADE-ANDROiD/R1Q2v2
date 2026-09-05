#!/usr/bin/env python3
"""Lock R1Q2v2 to 1920x1080: patch config.cfg + append video lock to autoexec files."""

from __future__ import annotations

import re
import sys
from pathlib import Path

Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")

VIDEO_LOCK = """
// --- Video lock (R1Q2v2 1920x1080; wins over config.cfg / video menu) ---
seta gl_mode "-1"
seta vid_forcewidth "1920"
seta vid_forceheight "1080"
"""

GL_MODE_RE = re.compile(r'^seta?\s+gl_mode\s+"[^"]*"\s*$', re.MULTILINE)
SW_MODE_RE = re.compile(r'^seta?\s+sw_mode\s+"[^"]*"\s*$', re.MULTILINE)


def patch_config(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="replace")
    orig = text

    if GL_MODE_RE.search(text):
        text = GL_MODE_RE.sub('seta gl_mode "-1"', text, count=1)
    else:
        text = text.rstrip() + '\nseta gl_mode "-1"\n'

    if SW_MODE_RE.search(text):
        text = SW_MODE_RE.sub('seta sw_mode "-1"', text, count=1)

    for cvar, val in (("vid_forcewidth", "1920"), ("vid_forceheight", "1080")):
        pat = re.compile(rf'^seta?\s+{cvar}\s+"[^"]*"\s*$', re.MULTILINE)
        line = f'seta {cvar} "{val}"'
        if pat.search(text):
            text = pat.sub(line, text, count=1)
        else:
            text = text.rstrip() + f"\n{line}\n"

    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def patch_autoexec(path: Path) -> bool:
    text = path.read_text(encoding="utf-8", errors="replace")
    if "Video lock (R1Q2v2 1920x1080" in text:
        return False
    path.write_text(text.rstrip() + VIDEO_LOCK + "\n", encoding="utf-8")
    return True


def main() -> int:
    changed_cfg: list[str] = []
    changed_ae: list[str] = []

    for path in sorted(Q2.rglob("config.cfg")):
        if "cfg-backup" in path.parts:
            continue
        if patch_config(path):
            changed_cfg.append(str(path))

    for path in sorted(Q2.rglob("autoexec.cfg")):
        if "cfg-backup" in path.parts:
            continue
        if patch_autoexec(path):
            changed_ae.append(str(path))

    print(f"config.cfg patched: {len(changed_cfg)}")
    for p in changed_cfg:
        print(f"  {p}")
    print(f"autoexec.cfg patched: {len(changed_ae)}")
    for p in changed_ae:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
