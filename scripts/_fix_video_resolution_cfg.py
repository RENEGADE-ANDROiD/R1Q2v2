#!/usr/bin/env python3
"""Lock R1Q2v2 to 1920x1080 via gl_mode 17 + vid_force (this display's 1080p index)."""

from __future__ import annotations

import re
import sys
from pathlib import Path

Q2 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2")
REPO = Path(__file__).resolve().parents[1]
MODE = "17"

VIDEO_LOCK = f"""
// --- Video lock (R1Q2v2 1920x1080 @ gl_mode {MODE} on this display) ---
seta gl_mode "{MODE}"
seta sw_mode "{MODE}"
seta vid_forcewidth "1920"
seta vid_forceheight "1080"
"""

GL_MODE_RE = re.compile(r'^seta?\s+gl_mode\s+"[^"]*".*$', re.MULTILINE)
SW_MODE_RE = re.compile(r'^seta?\s+sw_mode\s+"[^"]*".*$', re.MULTILINE)
VIDEO_LOCK_RE = re.compile(
    r"\n// --- Video lock \(R1Q2v2 1920x1080.*?seta vid_forceheight \"1080\".*\n",
    re.DOTALL,
)


def patch_video_cvars(text: str) -> str:
    if GL_MODE_RE.search(text):
        text = GL_MODE_RE.sub(f'seta gl_mode "{MODE}"', text)
    else:
        text = text.rstrip() + f'\nseta gl_mode "{MODE}"\n'

    if SW_MODE_RE.search(text):
        text = SW_MODE_RE.sub(f'seta sw_mode "{MODE}"', text)
    else:
        text = re.sub(
            rf'(^seta?\s+gl_mode\s+"{MODE}".*$)',
            rf'\1\nseta sw_mode "{MODE}"',
            text,
            count=1,
            flags=re.MULTILINE,
        )
        if not SW_MODE_RE.search(text):
            text = text.rstrip() + f'\nseta sw_mode "{MODE}"\n'

    for cvar, val in (("vid_forcewidth", "1920"), ("vid_forceheight", "1080")):
        pat = re.compile(rf'^seta?\s+{cvar}\s+"[^"]*".*$', re.MULTILINE)
        line = f'seta {cvar} "{val}"'
        if pat.search(text):
            text = pat.sub(line, text, count=1)
        else:
            text = text.rstrip() + f"\n{line}\n"
    return text


def consolidate_sw_mode(text: str) -> str:
    """Place a single sw_mode line immediately after the first gl_mode line."""
    if not GL_MODE_RE.search(text):
        return text
    lines = [
        ln for ln in text.splitlines()
        if not re.match(r'^seta?\s+sw_mode\s', ln)
    ]
    text = "\n".join(lines)
    if not text.endswith("\n"):
        text += "\n"
    return re.sub(
        rf'(^seta?\s+gl_mode\s+"{MODE}".*$)',
        rf'\1\nseta sw_mode "{MODE}"',
        text,
        count=1,
        flags=re.MULTILINE,
    )


def dedupe_video_lines(text: str) -> str:
    """Keep the first gl_mode/sw_mode/vid_force* line; drop later duplicates."""
    seen: set[str] = set()
    out: list[str] = []
    for line in text.splitlines():
        m = re.match(r'^seta?\s+(gl_mode|sw_mode|vid_forcewidth|vid_forceheight)\s', line)
        if m:
            key = m.group(1)
            if key in seen:
                continue
            seen.add(key)
        out.append(line)
    trailing_nl = "\n" if text.endswith("\n") else ""
    return "\n".join(out) + trailing_nl


def patch_config(path: Path) -> bool:
    orig = path.read_text(encoding="utf-8", errors="replace")
    text = patch_video_cvars(orig)
    text = consolidate_sw_mode(text)
    text = dedupe_video_lines(text)
    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def patch_autoexec(path: Path) -> bool:
    orig = path.read_text(encoding="utf-8", errors="replace")
    text = patch_video_cvars(orig)
    text = VIDEO_LOCK_RE.sub("\n", text)
    text = consolidate_sw_mode(text)
    text = dedupe_video_lines(text)
    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def patch_visual_cfg(path: Path) -> bool:
    orig = path.read_text(encoding="utf-8", errors="replace")
    text = orig
    text = re.sub(
        r"// (?:gl_mode -1|R1 uses gl_mode \+ vid_force\* \(no vid_geometry\))\..*",
        f"// gl_mode {MODE} = 1920x1080 on this display (+ vid_force* backup)",
        text,
        count=1,
    )
    text = patch_video_cvars(text)
    text = consolidate_sw_mode(text)
    text = dedupe_video_lines(text)
    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def patch_bat(path: Path) -> bool:
    if not path.is_file():
        return False
    orig = path.read_text(encoding="utf-8", errors="replace")
    text = orig.replace("+seta gl_mode -1", f"+seta gl_mode {MODE}")
    text = text.replace("+set gl_mode -1", f"+seta gl_mode {MODE}")
    if f"+seta sw_mode {MODE}" not in text:
        text = text.replace(
            f"+seta gl_mode {MODE}",
            f"+seta gl_mode {MODE} +seta sw_mode {MODE}",
            1,
        )
    if text != orig:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> int:
    changed: list[str] = []

    for path in sorted(Q2.rglob("config.cfg")):
        if "cfg-backup" in path.parts:
            continue
        if patch_config(path):
            changed.append(str(path))

    for path in sorted(Q2.rglob("autoexec.cfg")):
        if "cfg-backup" in path.parts:
            continue
        if patch_autoexec(path):
            changed.append(str(path))

    for rel in (
        "baseq2/r1q2v2_visual.cfg",
        "baseq2/pretty_r1q2v2.cfg",
    ):
        for root in (Q2, REPO):
            path = root / rel
            if path.is_file() and patch_visual_cfg(path):
                changed.append(str(path))

    bat = Q2 / "-PLAY- R1Q2v2.bat"
    if patch_bat(bat):
        changed.append(str(bat))

    print(f"patched {len(changed)} files")
    for p in changed:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
