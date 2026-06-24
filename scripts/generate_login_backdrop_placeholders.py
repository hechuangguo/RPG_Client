"""Generate LoginFlowBackdrop placeholder: backdrop_base.png only."""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "assets/_Project/Art/UI/LoginFlowBackdrop"


def write_png(path: Path, width: int, height: int, pixels: list[tuple[int, int, int, int]]) -> None:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            raw.extend((r, g, b, a))

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def lerp_color(c1: tuple[int, int, int], c2: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return (
        int(lerp(c1[0], c2[0], t)),
        int(lerp(c1[1], c2[1], t)),
        int(lerp(c1[2], c2[2], t)),
    )


def make_backdrop_base(path: Path) -> None:
    w, h = 1920, 1080
    pixels = [(0, 0, 0, 0)] * (w * h)
    sky_top = (45, 72, 110)
    sky_horizon = (212, 168, 120)
    mountain = (58, 98, 108)
    mountain_dark = (38, 68, 78)
    fade_teal = (42, 107, 90)

    for y in range(h):
        v = y / (h - 1)
        for x in range(w):
            u = x / (w - 1)
            if v < 0.55:
                sky_t = v / 0.55
                r, g, b = lerp_color(sky_top, sky_horizon, sky_t ** 0.7)
                sun = math.exp(-((u - 0.72) ** 2 + (v - 0.18) ** 2) / 0.008)
                r = min(255, int(r + sun * 80))
                g = min(255, int(g + sun * 50))
                b = min(255, int(b + sun * 20))
                alpha = 255
            elif v < 0.58:
                t = (v - 0.55) / 0.03
                r, g, b = lerp_color(sky_horizon, fade_teal, t)
                alpha = int(lerp(255, 180, t))
            else:
                t = (v - 0.58) / 0.42
                r, g, b = fade_teal
                alpha = int(lerp(180, 0, t ** 1.2))

            if v < 0.52:
                m1 = 0.38 + 0.12 * math.sin(u * 4.5) + 0.06 * math.sin(u * 11)
                m2 = 0.48 + 0.08 * math.sin(u * 3.2 + 1.2)
                if v > m1:
                    r, g, b = mountain
                    alpha = 255
                if v > m2 and u < 0.75:
                    r, g, b = mountain_dark
                    alpha = 255
                if u > 0.78 and v > 0.22 and v < 0.58:
                    cliff = (u - 0.78) * 3.5 + (v - 0.22) * 0.3
                    if cliff < 1.1:
                        r, g, b = (72, 88, 96)
                        alpha = 255

            pixels[y * w + x] = (r, g, b, alpha)
    write_png(path, w, h, pixels)


def main() -> None:
    out = ART / "backdrop_base.png"
    make_backdrop_base(out)
    print("Generated:", out)


if __name__ == "__main__":
    main()
