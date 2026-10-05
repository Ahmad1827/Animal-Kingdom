#!/usr/bin/env python3
"""Draw the stone stronghold that stands behind a settlement.

Writes  assets/sprites/medieval/castle_<tier>.png   tier 0, 1, 2

The castle is built up in code on a small canvas, one art pixel at a time, then
enlarged without smoothing so its pixels are the same chunky size as the rest of
the village art. Each tier adds to the one before:
    0  gatehouse and a low curtain wall with one tower
    1  the wall raised, towers at both ends
    2  the great keep behind, with turrets and the long banner

Run from the project root:   python3 tools/build_castle.py [--preview out.png]
Everything that decides the look (sizes, colours, banner) is at the top.
"""
import argparse
import os
import random

import numpy as np
from PIL import Image

OUT_DIR = "assets/sprites/medieval"
W, H = 300, 190          # art pixels
SCALE = 3                # real pixels per art pixel
GROUND = H - 1

STONE = [(46, 48, 60), (66, 68, 82), (90, 92, 106), (118, 120, 132), (146, 148, 158), (176, 178, 184), (204, 205, 208)]
SLATE = [(26, 36, 62), (36, 52, 86), (48, 72, 112), (66, 96, 140), (92, 124, 166), (124, 152, 188)]
WOOD = [(44, 28, 18), (66, 42, 24), (92, 60, 32), (120, 82, 44), (150, 108, 60)]
MOSS = [(40, 66, 36), (58, 92, 46), (82, 120, 58)]
BANNER = [(92, 18, 24), (138, 26, 30), (178, 40, 40), (210, 66, 54)]
GOLD = [(150, 100, 24), (212, 158, 42), (250, 216, 96)]
GLOW = [(190, 110, 30), (240, 170, 60), (255, 222, 130)]
OUTLINE = (20, 18, 24)


class Canvas:
    def __init__(self):
        self.px = np.zeros((H, W, 4), dtype=np.uint8)
        self.rng = random.Random(7)

    def put(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.px[y, x, :3] = c
            self.px[y, x, 3] = 255

    def solid(self, x, y):
        return 0 <= x < W and 0 <= y < H and self.px[y, x, 3] > 0

    # -- stone ---------------------------------------------------------------
    def stone(self, x0, y0, x1, y1, light=0.0, round_=False):
        """A wall of coursed stone. light shifts the whole face brighter or darker;
        round_ shades it as a drum, lit from the left."""
        course = 4
        for y in range(y0, y1 + 1):
            row = (y - y0) // course
            offset = 3 if row % 2 else 0
            for x in range(x0, x1 + 1):
                brick = (x - x0 + offset) // 7
                r = random.Random((brick * 92821) ^ (row * 68917) ^ (x0 * 31 + y0)).random()
                tone = 3.0 + light + (r - 0.5) * 1.6
                if round_:
                    u = (x - x0) / max(1.0, float(x1 - x0))           # 0 at the lit edge
                    tone += 1.3 - 3.4 * abs(u - 0.30) ** 1.2
                tone -= 1.2 * (y - y0) / max(1.0, float(y1 - y0)) * 0.6      # heavier toward the ground
                mortar_h = (y - y0) % course == course - 1
                mortar_v = (x - x0 + offset) % 7 == 6
                if mortar_h or mortar_v:
                    tone -= 1.5
                elif (y - y0) % course == 0:
                    tone += 0.5                                       # the top edge of each stone catches light
                self.put(x, y, STONE[int(max(0, min(len(STONE) - 1, round(tone))))])

    def merlons(self, x0, x1, y, light=0.0, round_=False):
        """Battlements: teeth along the top of a wall, standing on row y."""
        x = x0
        while x <= x1:
            self.stone(x, y - 5, min(x1, x + 4), y, light + 0.4, round_=False)
            x += 8

    def slit(self, x, y):
        for j in range(5):
            self.put(x, y + j, (16, 14, 20))
        self.put(x - 1, y + 2, (16, 14, 20))
        self.put(x + 1, y + 2, (16, 14, 20))

    def window(self, x, y):
        """A lit arched window, three wide."""
        for j in range(6):
            for i in range(3):
                if j == 0 and i != 1:
                    continue
                self.put(x + i, y + j, GLOW[2] if (i == 1 and j < 4) else GLOW[1])
        for j in range(1, 6):
            self.put(x - 1, y + j, STONE[1]); self.put(x + 3, y + j, STONE[1])
        self.put(x, y, STONE[1]); self.put(x + 2, y, STONE[1]); self.put(x + 1, y - 1, STONE[1])
        for i in range(-1, 4):
            self.put(x + i, y + 6, STONE[5])                          # sill

    def moss(self, x0, x1, y_base, amount):
        for _ in range(amount):
            cx = self.rng.randint(x0, x1)
            cy = y_base - int(abs(self.rng.gauss(0, 9)))
            for _ in range(self.rng.randint(3, 9)):
                x, y = cx + self.rng.randint(-2, 2), cy + self.rng.randint(-2, 2)
                if self.solid(x, y) and int(self.px[y, x, :3].sum()) > 120:      # not in the dark of the gateway
                    self.put(x, y, self.rng.choice(MOSS))

    # -- roofs and wood ----------------------------------------------------------
    def cone(self, cx, base_y, half, height):
        """A slate spire with a little overhang."""
        for j in range(height):
            y = base_y - j
            w = int(round((half + 2) * (1.0 - j / float(height))))
            for x in range(cx - w, cx + w + 1):
                u = (x - (cx - w)) / max(1.0, 2.0 * w)
                tone = 4.2 - 4.6 * abs(u - 0.28) ** 1.1
                if j % 3 == 2:
                    tone -= 1.3                                       # the shadow under each row of slates
                elif (x + (j // 3) * 2) % 4 == 0:
                    tone -= 0.6
                self.put(x, y, SLATE[int(max(0, min(len(SLATE) - 1, round(tone))))])
        for x in range(cx - half - 2, cx + half + 3):
            self.put(x, base_y + 1, SLATE[0])

    def flag(self, x, y_top, length=11, drop=7, right=True):
        for j in range(drop + 9):
            self.put(x, y_top + j, WOOD[1])
        self.put(x, y_top - 1, GOLD[2])
        d = 1 if right else -1
        for i in range(1, length + 1):
            wave = 1 if (i // 3) % 2 else 0
            for j in range(drop - (1 if i > length - 3 else 0)):
                tone = 2 if j < 2 else (1 if j < drop - 2 else 0)
                if i % 5 == 0:
                    tone = max(0, tone - 1)
                self.put(x + d * i, y_top + 1 + j + wave, BANNER[tone + (1 if j == 0 else 0)])
            self.put(x + d * i, y_top + 3 + wave, GOLD[1])            # a gold bar across the field

    def banner(self, x, y_top, width=7, length=26):
        """A long banner hung flat against a wall, swallow-tailed."""
        for i in range(-1, width + 1):
            self.put(x + i, y_top - 1, WOOD[1])
        for j in range(length):
            for i in range(width):
                if j > length - 4 and abs(i - width // 2) < (j - (length - 4)):
                    continue                                          # the notch of the tail
                tone = 3 if i == 1 else (2 if i < width - 2 else 1)
                self.put(x + i, y_top + j, BANNER[tone])
            if j in (2, length - 6):
                for i in range(width):
                    self.put(x + i, y_top + j, GOLD[1])
        # the device: a small gold crown
        cy = y_top + 9
        for i, col in enumerate([1, 3, 5]):
            self.put(x + col, cy, GOLD[2])
        for i in range(1, 6):
            self.put(x + i, cy + 1, GOLD[2]); self.put(x + i, cy + 2, GOLD[1])

    def gate(self, cx, y_top, half=11):
        """Arched gateway: dark passage, raised portcullis, oak doors ajar."""
        for y in range(y_top, GROUND + 1):
            j = y - y_top
            w = half if j >= half else int(round((half ** 2 - (half - j) ** 2) ** 0.5))
            for x in range(cx - w, cx + w + 1):
                self.put(x, y, (14, 12, 16))
            # arch stones
            for x in (cx - w - 1, cx + w + 1):
                self.put(x, y, STONE[5] if (y // 3) % 2 else STONE[4])
        for x in range(cx - half + 1, cx + half, 4):                 # portcullis teeth showing at the top
            for y in range(y_top + 3, y_top + 13):
                if self.px[y, x, 3] and tuple(self.px[y, x, :3]) == (14, 12, 16):
                    self.put(x, y, STONE[2])
        for y in range(y_top + 12, GROUND + 1):                      # doors
            for x in list(range(cx - half + 1, cx - half + 6)) + list(range(cx + half - 5, cx + half)):
                if tuple(self.px[y, x, :3]) == (14, 12, 16):
                    self.put(x, y, WOOD[2] if (x % 2) else WOOD[3])
            if (y - y_top) % 9 == 0:
                for x in list(range(cx - half + 1, cx - half + 6)) + list(range(cx + half - 5, cx + half)):
                    if self.solid(x, y):
                        self.put(x, y, STONE[1])                      # iron bands

    def torch(self, x, y):
        self.put(x, y + 2, WOOD[1]); self.put(x, y + 3, WOOD[1])
        self.put(x, y + 1, GLOW[1]); self.put(x, y, GLOW[2]); self.put(x, y - 1, GLOW[1])
        self.put(x - 1, y, GLOW[0]); self.put(x + 1, y, GLOW[0])

    # -- finishing -----------------------------------------------------------------
    def shadow(self, x0, x1, y0, y1, strength=40):
        """Darken a strip, for the shade one part throws on another."""
        for y in range(max(0, y0), min(H, y1 + 1)):
            for x in range(max(0, x0), min(W, x1 + 1)):
                if self.px[y, x, 3]:
                    self.px[y, x, :3] = np.clip(self.px[y, x, :3].astype(int) - strength, 0, 255)

    def outline(self):
        a = self.px[..., 3] > 0
        edge = np.zeros_like(a)
        edge[1:, :] |= a[1:, :] & ~a[:-1, :]
        edge[:-1, :] |= a[:-1, :] & ~a[1:, :]
        edge[:, 1:] |= a[:, 1:] & ~a[:, :-1]
        edge[:, :-1] |= a[:, :-1] & ~a[:, 1:]
        self.px[edge, :3] = OUTLINE

    def image(self):
        return Image.fromarray(self.px, "RGBA").resize((W * SCALE, H * SCALE), Image.NEAREST)


def tower(c, x0, x1, top, roof=True, flag_right=True):
    cx = (x0 + x1) // 2
    c.stone(x0, top, x1, GROUND, light=0.3, round_=True)
    # the fighting top stands a little proud of the drum
    c.stone(x0 - 2, top - 7, x1 + 2, top, light=0.6, round_=True)
    c.shadow(x0, x1, top + 1, top + 2, 45)
    if roof:
        c.cone(cx, top - 8, (x1 - x0) // 2 + 2, 30)
        c.flag(cx, top - 50, right=flag_right)
    else:
        c.merlons(x0 - 2, x1 + 2, top - 8, 0.6)
    c.slit(cx - 5, top + 14)
    c.slit(cx + 6, top + 40)
    c.slit(cx - 4, top + 66)


def build(tier):
    c = Canvas()
    wall_top = 134 if tier == 0 else 120

    if tier >= 2:
        # the great keep, behind everything
        c.stone(104, 46, 196, GROUND, light=-0.9)          # a tone darker: it stands further back
        c.merlons(104, 196, 45, -0.9)
        for x in (118, 147, 176):
            c.window(x, 62)
        for x in (132, 162):
            c.window(x, 86)
        # corner turrets
        for x0 in (98, 190):
            c.stone(x0, 30, x0 + 12, 60, light=0.4, round_=True)
            c.cone(x0 + 6, 29, 8, 20)
        c.flag(150, 14, length=14, drop=8)
        c.stone(149, 30, 151, 45, light=0.0)
        c.banner(146, 100, width=9, length=22)
        c.shadow(104, 196, 46, 49, 30)

    # curtain wall
    c.stone(22, wall_top, 278, GROUND, light=0.0)
    c.merlons(22, 278, wall_top - 1, 0.0)
    c.shadow(22, 278, wall_top, wall_top + 1, 35)
    for x in range(40, 270, 26):
        if not (118 <= x <= 182):
            c.slit(x, wall_top + 14)

    # gatehouse
    gate_top = wall_top - 22
    c.stone(122, gate_top, 178, GROUND, light=0.5)
    c.merlons(122, 178, gate_top - 1, 0.5)
    c.shadow(122, 178, gate_top, gate_top + 1, 35)
    c.gate(150, GROUND - 40)
    c.window(142, gate_top + 8); c.window(156, gate_top + 8)
    c.banner(126, gate_top + 20, width=7, length=24)
    c.banner(168, gate_top + 20, width=7, length=24)
    c.torch(136, GROUND - 26); c.torch(164, GROUND - 26)
    c.shadow(179, 184, wall_top, GROUND, 28)                        # the gatehouse shades the wall beside it

    # towers
    tower(c, 30, 68, 84 if tier >= 1 else 104, roof=True, flag_right=True)
    c.shadow(69, 75, wall_top, GROUND, 28)
    if tier >= 1:
        tower(c, 232, 270, 74, roof=True, flag_right=False)
        c.shadow(271, 277, wall_top, GROUND, 28)
    else:
        # the far tower is still going up: a stump with scaffolding poles
        c.stone(234, wall_top - 10, 268, GROUND, light=0.3, round_=True)
        for x in (236, 250, 266):
            for y in range(wall_top - 30, wall_top - 10):
                c.put(x, y, WOOD[2])
        for x in range(234, 269):
            c.put(x, wall_top - 22, WOOD[3])

    c.moss(22, 278, GROUND, 70)
    c.outline()
    return c.image()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview")
    args = ap.parse_args()
    os.makedirs(OUT_DIR, exist_ok=True)
    imgs = []
    for tier in range(3):
        img = build(tier)
        path = os.path.join(OUT_DIR, f"castle_{tier}.png")
        img.save(path)
        imgs.append(img)
        print("wrote", path, img.size)
    if args.preview:
        sheet = Image.new("RGBA", (W * SCALE * 3 + 40, H * SCALE + 20), (120, 170, 210, 255))
        for i, img in enumerate(imgs):
            sheet.paste(img, (10 + i * (W * SCALE + 10), 10), img)
        sheet.convert("RGB").save(args.preview)


if __name__ == "__main__":
    main()
