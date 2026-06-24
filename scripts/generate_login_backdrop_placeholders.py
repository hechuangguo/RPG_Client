"""Generate LoginFlowBackdrop placeholder art.

Default: FX only (fx_bird_sheet, fx_boat_fisherman).
--split-layers: also generate near-view split backdrop_base/water/trees/waterfall for dev.
"""
from __future__ import annotations

import argparse
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


def draw_bird_silhouette(pixels: list[tuple[int, int, int, int]], w: int, h: int, ox: int, wing: float) -> None:
    for y in range(h):
        for x in range(w):
            lx, ly = x - ox, y - h // 2
            body = ly > -4 and ly < 6 and lx > -8 and lx < 18
            wing_y = int(wing * 10)
            left_wing = ly > wing_y - 8 and ly < wing_y + 2 and lx > -28 and lx < -2
            right_wing = ly > -wing_y - 2 and ly < -wing_y + 8 and lx > -2 and lx < 26
            neck = ly > -10 and ly < 0 and lx > 14 and lx < 22
            if body or left_wing or right_wing or neck:
                pixels[y * w + x] = (240, 245, 255, 245)


def make_bird_sheet(path: Path) -> None:
    fw, fh, frames = 96, 48, 4
    w, h = fw * frames, fh
    pixels = [(0, 0, 0, 0)] * (w * h)
    wings = [0.2, 0.8, 0.2, -0.5]
    for i, wing in enumerate(wings):
        frame_pixels = [(0, 0, 0, 0)] * (fw * fh)
        draw_bird_silhouette(frame_pixels, fw, fh, fw // 2 - 8, wing)
        for y in range(fh):
            for x in range(fw):
                pixels[y * w + i * fw + x] = frame_pixels[y * fw + x]
    write_png(path, w, h, pixels)


def make_boat(path: Path) -> None:
    w, h = 200, 100
    pixels = [(0, 0, 0, 0)] * (w * h)
    for y in range(h):
        for x in range(w):
            u, v = x / w, y / h
            hull = v > 0.55 and v < 0.72 and u > 0.08 and u < 0.92
            hull &= abs((u - 0.5) * 1.8) + (v - 0.64) * 2 < 0.42
            hat = (u - 0.62) ** 2 + (v - 0.38) ** 2 < 0.012
            body = u > 0.58 and u < 0.72 and v > 0.32 and v < 0.58
            rod = u > 0.7 and u < 0.95 and v > 0.15 and v < 0.42 and (u + v) < 1.15
            if hull:
                pixels[y * w + x] = (120, 85, 55, 250)
            elif hat or body or rod:
                pixels[y * w + x] = (230, 225, 210, 245)
    write_png(path, w, h, pixels)


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

            # layered mountains upper region
            if v < 0.52:
                m1 = 0.38 + 0.12 * math.sin(u * 4.5) + 0.06 * math.sin(u * 11)
                m2 = 0.48 + 0.08 * math.sin(u * 3.2 + 1.2)
                if v > m1:
                    r, g, b = mountain
                    alpha = 255
                if v > m2 and u < 0.75:
                    r, g, b = mountain_dark
                    alpha = 255
                # right cliff
                if u > 0.78 and v > 0.22 and v < 0.58:
                    cliff = (u - 0.78) * 3.5 + (v - 0.22) * 0.3
                    if cliff < 1.1:
                        r, g, b = (72, 88, 96)
                        alpha = 255

            pixels[y * w + x] = (r, g, b, alpha)
    write_png(path, w, h, pixels)


def make_backdrop_water(path: Path) -> None:
    w, h = 1024, 512
    pixels = [(0, 0, 0, 0)] * (w * h)
    base = (42, 107, 90)
    highlight = (72, 145, 128)
    for y in range(h):
        for x in range(w):
            u, v = x / w, y / h
            wave = math.sin(u * 28 + v * 6) * 0.5 + math.sin(u * 14 - v * 4) * 0.3
            t = 0.5 + wave * 0.5
            r, g, b = lerp_color(base, highlight, t)
            edge_fade = min(1.0, v * 2.5, (1 - v) * 2.5)
            alpha = int(220 * edge_fade)
            pixels[y * w + x] = (r, g, b, alpha)
    write_png(path, w, h, pixels)


def make_backdrop_trees(path: Path) -> None:
    w, h = 806, 418
    pixels = [(0, 0, 0, 0)] * (w * h)
    trunk = (32, 48, 38)
    foliage = (28, 72, 58)
    for y in range(h):
        for x in range(w):
            u = x / w
            v = y / h
            # trunk left
            if x < 90 and v > 0.15 and abs(x - 55) < 18 * (1.1 - v * 0.3):
                pixels[y * w + x] = (*trunk, 240)
                continue
            # branches
            branch = False
            for i in range(5):
                bx = 40 + i * 35
                by = int(h * (0.25 + i * 0.12))
                dist = math.hypot(x - bx, y - by)
                angle = math.atan2(y - by, x - bx)
                if dist < 55 - i * 6 and angle > -1.2 and angle < 0.8:
                    branch = True
            # canopy blobs
            for cx, cy, rad in [(180, 120, 95), (280, 180, 110), (120, 260, 85), (350, 280, 100)]:
                if math.hypot(x - cx, y - cy) < rad * (0.7 + 0.3 * (y / h)):
                    branch = True
            if branch:
                shade = 0.85 + 0.15 * math.sin(x * 0.08 + y * 0.05)
                pixels[y * w + x] = (
                    int(foliage[0] * shade),
                    int(foliage[1] * shade),
                    int(foliage[2] * shade),
                    235,
                )
    write_png(path, w, h, pixels)


def make_backdrop_waterfall(path: Path) -> None:
    w, h = 256, 512
    pixels = [(0, 0, 0, 0)] * (w * h)
    for y in range(h):
        for x in range(w):
            u, v = x / w, y / h
            stream = abs(u - 0.55) < 0.12 + 0.04 * math.sin(v * 12)
            mist = abs(u - 0.55) < 0.22 and v > 0.7
            if stream:
                bright = 200 + int(40 * math.sin(v * 30 + u * 10))
                pixels[y * w + x] = (bright, bright, 255, int(180 + 60 * (1 - abs(u - 0.55) / 0.12)))
            elif mist:
                pixels[y * w + x] = (200, 220, 230, int(80 * (1 - abs(u - 0.55) / 0.22)))
    write_png(path, w, h, pixels)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--split-layers", action="store_true", help="Generate split backdrop layers for dev")
    args = parser.parse_args()

    make_bird_sheet(ART / "fx_bird_sheet.png")
    make_boat(ART / "fx_boat_fisherman.png")

    if args.split_layers:
        make_backdrop_base(ART / "backdrop_base.png")
        make_backdrop_water(ART / "backdrop_water.png")
        make_backdrop_trees(ART / "backdrop_trees.png")
        make_backdrop_waterfall(ART / "backdrop_waterfall.png")
        print("Generated split layers + FX:", ART)
    else:
        print("Generated FX placeholders:", ART)


if __name__ == "__main__":
    main()
