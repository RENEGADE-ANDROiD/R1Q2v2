#!/usr/bin/env python3
"""Extract stock Quake II menu PCXs from pak0, 2x Scale2x + light sharpen, pack zz_r1q2_titles.pkz.

Does not rewrite pak9. The pkz must load after numbered paks (named *.pkz).
Ship both the original PCX (gl_pic_scale size hints) and a 2x PNG (what R1GL draws).
Do not commit the generated pkz — it contains id Software art from pak0.
"""

from __future__ import annotations

import argparse
import io
import struct
import sys
import zipfile
from pathlib import Path

from PIL import Image, ImageFilter

PAK_IDENT = b"PACK"
DIR_ENTRY = 64
NAME_LEN = 56
TRANSPARENT_INDEX = 255
UNSHARP = ImageFilter.UnsharpMask(radius=1, percent=40, threshold=2)

DEFAULT_PAK0 = Path(r"D:\SteamLibrary\steamapps\common\Quake 2\baseq2\pak0.pak")
DEFAULT_OUT = Path(r"D:\SteamLibrary\steamapps\common\Quake 2\baseq2\zz_r1q2_titles.pkz")


def pak_files(pak_path: Path) -> list[tuple[str, bytes]]:
    data = pak_path.read_bytes()
    if len(data) < 12 or data[:4] != PAK_IDENT:
        raise SystemExit(f"not a Quake PAK: {pak_path}")
    dirofs, dirlen = struct.unpack_from("<II", data, 4)
    n = dirlen // DIR_ENTRY
    out: list[tuple[str, bytes]] = []
    for i in range(n):
        off = dirofs + i * DIR_ENTRY
        raw_name = data[off : off + NAME_LEN]
        name = raw_name.split(b"\0", 1)[0].decode("ascii", "replace").replace("\\", "/").lower()
        pos, size = struct.unpack_from("<II", data, off + NAME_LEN)
        if not _want(name):
            continue
        out.append((name, data[pos : pos + size]))
    out.sort(key=lambda t: t[0])
    return out


def _want(name: str) -> bool:
    if not name.endswith(".pcx"):
        return False
    return name.startswith("pics/m_banner_") or name.startswith("pics/m_main_")


def decode_pcx(blob: bytes) -> Image.Image:
    """Quake-style 8-bit RLE PCX: palette is the last 768 bytes; index 255 is transparent."""
    if len(blob) < 128 + 768:
        raise ValueError("PCX too small")
    manufacturer, version, encoding, bpp = blob[0], blob[1], blob[2], blob[3]
    xmin, ymin, xmax, ymax = struct.unpack_from("<HHHH", blob, 4)
    if manufacturer != 0x0A or version != 5 or encoding != 1 or bpp != 8:
        raise ValueError("unsupported PCX header")
    width = xmax + 1
    height = ymax + 1
    palette = blob[-768:]
    raw = blob[128:]
    pixels = bytearray(width * height)
    i = 0
    x = y = 0
    while y < height:
        if i >= len(raw):
            raise ValueError("truncated PCX RLE")
        b = raw[i]
        i += 1
        if (b & 0xC0) == 0xC0:
            run = b & 0x3F
            if i >= len(raw):
                raise ValueError("truncated PCX RLE run")
            val = raw[i]
            i += 1
        else:
            run = 1
            val = b
        for _ in range(run):
            if x >= width:
                raise ValueError("PCX run past row")
            pixels[y * width + x] = val
            x += 1
            if x >= width:
                x = 0
                y += 1
                if y >= height:
                    break
    rgba = bytearray(width * height * 4)
    for p, idx in enumerate(pixels):
        if idx == TRANSPARENT_INDEX:
            continue
        o = p * 4
        rgba[o : o + 3] = palette[idx * 3 : idx * 3 + 3]
        rgba[o + 3] = 255
    return Image.frombytes("RGBA", (width, height), bytes(rgba))


def _px(img: Image.Image, x: int, y: int) -> tuple[int, int, int, int]:
    if x < 0 or y < 0 or x >= img.width or y >= img.height:
        return (0, 0, 0, 0)
    return img.getpixel((x, y))


def scale2x(src: Image.Image) -> Image.Image:
    """Pixel-art Scale2x. 1px-thin images fall back to nearest-neighbor."""
    w, h = src.size
    if w <= 1 or h <= 1:
        return src.resize((w * 2, h * 2), Image.Resampling.NEAREST)
    dst = Image.new("RGBA", (w * 2, h * 2))
    dp = dst.load()
    for y in range(h):
        for x in range(w):
            b = _px(src, x, y - 1)
            d = _px(src, x - 1, y)
            e = _px(src, x, y)
            f = _px(src, x + 1, y)
            hpix = _px(src, x, y + 1)
            if b != hpix and d != f:
                e0 = d if d == b else e
                e1 = f if b == f else e
                e2 = d if d == hpix else e
                e3 = f if hpix == f else e
            else:
                e0 = e1 = e2 = e3 = e
            dx, dy = x * 2, y * 2
            dp[dx, dy] = e0
            dp[dx + 1, dy] = e1
            dp[dx, dy + 1] = e2
            dp[dx + 1, dy + 1] = e3
    return dst


def enhance(img: Image.Image) -> Image.Image:
    """Mild unsharp on opaque RGB; keep alpha binary so edges do not fringe."""
    alpha = img.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    rgb = img.convert("RGB").filter(UNSHARP)
    out = Image.merge("RGBA", (*rgb.split(), alpha))
    return out


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def pack(entries: list[tuple[str, bytes]], pngs: list[tuple[str, bytes]], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(out_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name, blob in entries:
            zf.writestr(name.replace("\\", "/"), blob)
        for name, blob in pngs:
            zf.writestr(name.replace("\\", "/"), blob)


def main() -> int:
    ap = argparse.ArgumentParser(description="Build zz_r1q2_titles.pkz from pak0 menu PCXs")
    ap.add_argument("--pak", type=Path, default=DEFAULT_PAK0, help="path to pak0.pak")
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT, help="output .pkz")
    args = ap.parse_args()
    if not args.pak.is_file():
        print(f"missing {args.pak}", file=sys.stderr)
        return 1

    entries = pak_files(args.pak)
    if not entries:
        print("no pics/m_banner_* or pics/m_main_* in pak", file=sys.stderr)
        return 1

    pngs: list[tuple[str, bytes]] = []
    for name, blob in entries:
        img = decode_pcx(blob)
        scaled = enhance(scale2x(img))
        png_name = name[:-4] + ".png"
        pngs.append((png_name, png_bytes(scaled)))
        print(f"{name}: {img.size[0]}x{img.size[1]} -> {scaled.size[0]}x{scaled.size[1]} PNG")

    pack(entries, pngs, args.out)
    print(f"wrote {args.out} ({args.out.stat().st_size} bytes, {len(entries)} pcx + {len(pngs)} png)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
