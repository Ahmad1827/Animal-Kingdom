#!/usr/bin/env python3
"""Draw the medieval village: halls, tower, palisade and yard props.

Writes  assets/sprites/medieval/village.png   one atlas holding every piece

Nothing here is taken from the old sprite sheets. Each piece is built up in code
on a small canvas, one art pixel at a time, then enlarged without smoothing so
its pixels are the same chunky size as the castle from tools/build_castle.py.
The game draws every piece at scale 1, standing on its bottom edge.

Run from the project root:   python3 tools/build_village.py [--preview out.png]
It prints the atlas cells; paste them into include/world/StructureManager.h and
include/world/WorldManager.h if any size changes.
"""
import argparse
import os

import numpy as np
from PIL import Image

OUT = "assets/sprites/medieval/village.png"
SCALE = 3                # real pixels per art pixel
ATLAS_W = 2048
PAD = 2

STONE = [(46, 48, 60), (66, 68, 82), (90, 92, 106), (118, 120, 132), (146, 148, 158), (176, 178, 184), (204, 205, 208)]
SLATE = [(26, 36, 62), (36, 52, 86), (48, 72, 112), (66, 96, 140), (92, 124, 166), (124, 152, 188)]
WOOD = [(44, 28, 18), (66, 42, 24), (92, 60, 32), (120, 82, 44), (150, 108, 60), (178, 136, 84)]
BEAM = [(30, 20, 14), (48, 31, 19), (68, 44, 26), (90, 60, 34)]
PLASTER = [(140, 122, 96), (176, 158, 126), (206, 190, 156), (228, 216, 186), (242, 234, 210)]
THATCH = [(78, 56, 24), (110, 82, 34), (144, 112, 46), (178, 144, 62), (206, 174, 86), (228, 202, 120)]
MOSS = [(40, 66, 36), (58, 92, 46), (82, 120, 58)]
BANNER = [(92, 18, 24), (138, 26, 30), (178, 40, 40), (210, 66, 54)]
GOLD = [(150, 100, 24), (212, 158, 42), (250, 216, 96)]
GLOW = [(190, 110, 30), (240, 170, 60), (255, 222, 130)]
IRON = [(34, 36, 44), (62, 66, 76), (104, 110, 122), (156, 162, 172), (206, 210, 216)]
FIRE = [(196, 60, 18), (236, 110, 24), (250, 170, 44), (255, 224, 120), (255, 248, 210)]
DARK = (16, 13, 17)
OUTLINE = (20, 18, 24)


def hsh(a, b, c=0):
    n = ((a * 73856093) ^ (b * 19349663) ^ (c * 83492791)) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0


def pick(pal, tone):
    return pal[int(max(0, min(len(pal) - 1, round(tone))))]


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.ground = h - 1
        self.px = np.zeros((h, w, 4), dtype=np.uint8)

    def put(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y, x, :3] = c
            self.px[y, x, 3] = 255

    def solid(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h and self.px[y, x, 3] > 0

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, c)

    # -- walls -------------------------------------------------------------------
    def stone(self, x0, y0, x1, y1, light=0.0, round_=False, bw=7, course=4):
        """Coursed stone. round_ shades the face as a drum lit from the left."""
        for y in range(y0, y1 + 1):
            row = (y - y0) // course
            offset = bw // 2 if row % 2 else 0
            for x in range(x0, x1 + 1):
                brick = (x - x0 + offset) // bw
                tone = 3.0 + light + (hsh(brick, row, x0 * 31 + y0) - 0.5) * 1.6
                if round_:
                    u = (x - x0) / max(1.0, float(x1 - x0))
                    tone += 1.3 - 3.4 * abs(u - 0.30) ** 1.2
                tone -= 0.7 * (y - y0) / max(1.0, float(y1 - y0))
                if (y - y0) % course == course - 1 or (x - x0 + offset) % bw == bw - 1:
                    tone -= 1.5
                elif (y - y0) % course == 0:
                    tone += 0.5
                self.put(x, y, pick(STONE, tone))

    def planks(self, x0, y0, x1, y1, light=0.0, pw=4, upright=True, pal=WOOD):
        """Sawn boards, upright or laid flat."""
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                along, across = (y - y0, x - x0) if upright else (x - x0, y - y0)
                board = across // pw
                tone = 2.6 + light + (hsh(board, x0, y0) - 0.5) * 1.4
                if across % pw == pw - 1:
                    tone -= 1.6                                       # the gap between boards
                elif across % pw == 0:
                    tone += 0.5
                if hsh(board, along // 3, 5) > 0.86:
                    tone -= 0.7                                       # grain
                self.put(x, y, pick(pal, tone))

    def plaster(self, x0, y0, x1, y1, light=0.0):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                tone = 2.7 + light + (hsh(x // 2, y // 2, 9) - 0.5) * 0.9
                tone -= 0.8 * (y - y0) / max(1.0, float(y1 - y0))     # grubbier toward the ground
                if y == y0:
                    tone -= 1.0                                       # shade under whatever is above
                self.put(x, y, pick(PLASTER, tone))

    def line(self, x0, y0, x1, y1, pal=BEAM, tone=1.0, thick=1):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            x = int(round(x0 + (x1 - x0) * i / n))
            y = int(round(y0 + (y1 - y0) * i / n))
            for t in range(thick):
                self.put(x + (t if abs(y1 - y0) > abs(x1 - x0) else 0), y + (0 if abs(y1 - y0) > abs(x1 - x0) else t),
                         pick(pal, tone + (0.8 if t == 0 else 0.0)))

    def beam(self, x0, y0, x1, y1):
        """A squared timber, upright or level, two or three pixels thick."""
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                lit = (x == x0) if (y1 - y0) > (x1 - x0) else (y == y0)
                self.put(x, y, pick(BEAM, 2.0 if lit else (1.0 if hsh(x, y, 3) > 0.25 else 0.0)))

    def half_timber(self, x0, y0, x1, y1, bay=18, light=0.0, braces=True):
        """Plaster panels in a dark oak frame."""
        self.plaster(x0, y0, x1, y1, light)
        posts = list(range(x0, x1 - 2, bay))
        if x1 - 2 - posts[-1] < bay // 2:
            posts.pop()
        posts.append(x1 - 2)
        mid = (y0 + y1) // 2
        for i in range(len(posts) - 1):
            a, b = posts[i] + 2, posts[i + 1]
            if braces and (y1 - y0) >= 14:
                if i % 2 == 0:
                    self.line(a, y1 - 2, b, mid, thick=2)
                else:
                    self.line(a, mid, b, y1 - 2, thick=2)
        self.beam(x0, mid, x1, mid + 1) if (y1 - y0) >= 22 else None
        for p in posts:
            self.beam(p, y0, p + 2, y1)
        self.beam(x0, y0, x1, y0 + 2)
        self.beam(x0, y1 - 2, x1, y1)

    def log(self, x0, y0, x1, y1, light=0.0, tip=False):
        """An upright round log, lit from the left. tip sharpens the top."""
        w = x1 - x0 + 1
        for y in range(y0, y1 + 1):
            inset = max(0, (w // 2) - (y - y0)) if tip else 0
            for x in range(x0 + inset, x1 - inset + 1):
                u = (x - x0) / max(1.0, float(w - 1))
                tone = 3.6 + light - 3.0 * abs(u - 0.25)
                if hsh(x, y // 4, x0) > 0.8:
                    tone -= 0.7
                self.put(x, y, pick(WOOD, tone))

    # -- roofs -------------------------------------------------------------------
    def roof(self, cx, y_top, y_bot, half_top, half_bot, kind="thatch", light=0.0):
        """A roof slope seen from the front: narrow at the ridge, wide at the eave."""
        pal = THATCH if kind == "thatch" else SLATE
        span = max(1, y_bot - y_top)
        for y in range(y_top, y_bot + 1):
            t = (y - y_top) / float(span)
            hw = int(round(half_top + (half_bot - half_top) * t))
            for x in range(cx - hw, cx + hw + 1):
                u = (x - (cx - hw)) / max(1.0, 2.0 * hw)
                if kind == "thatch":
                    tone = 3.9 + light - 2.0 * u
                    band = (y - y_top) % 7
                    tone += (hsh(x, (y - y_top) // 7, 11) - 0.5) * 1.5
                    if band == 6:
                        tone -= 1.4                                   # the shadow under each layer of straw
                    elif band == 0:
                        tone += 0.6
                    if hsh(x, 0, cx) > 0.9:
                        tone -= 0.6
                else:
                    tone = 3.6 + light - 2.4 * u
                    row = (y - y_top) // 4
                    if (y - y_top) % 4 == 3:
                        tone -= 1.4
                    elif (x + row * 3) % 6 == 0:
                        tone -= 0.8
                    tone += (hsh((x + row * 3) // 6, row, 13) - 0.5) * 0.9
                self.put(x, y, pick(pal, tone))
        if kind == "thatch":
            for x in range(cx - half_bot, cx + half_bot + 1):         # a ragged eave
                r = hsh(x, y_bot, 17)
                if r > 0.35:
                    self.put(x, y_bot + 1, pick(THATCH, 1.0 + r))
                if r > 0.75:
                    self.put(x, y_bot + 2, THATCH[0])
            for x in range(cx - half_top - 1, cx + half_top + 2):     # ridge roll
                self.put(x, y_top - 1, THATCH[4]); self.put(x, y_top, THATCH[2]); self.put(x, y_top + 1, THATCH[1])
        else:
            for x in range(cx - half_bot - 1, cx + half_bot + 2):
                self.put(x, y_bot + 1, SLATE[0])
            for x in range(cx - half_top - 1, cx + half_top + 2):
                self.put(x, y_top - 1, STONE[4])

    def gable(self, cx, y_apex, y_base, half, kind="thatch"):
        """The end of a roof facing us: a framed plaster triangle under two verges."""
        pal = THATCH if kind == "thatch" else SLATE
        span = max(1, y_base - y_apex)
        for y in range(y_apex, y_base + 1):
            hw = int(round(half * (y - y_apex) / float(span)))
            for x in range(cx - hw, cx + hw + 1):
                self.put(x, y, pick(PLASTER, 2.6 + (hsh(x // 2, y // 2, 9) - 0.5) - (0.9 if abs(x - cx) > hw - 4 else 0)))
        self.beam(cx - 1, y_apex + 3, cx + 1, y_base)
        self.beam(cx - half + 4, y_base - 2, cx + half - 4, y_base)
        self.line(cx - half // 2, y_base - 2, cx - 1, y_apex + span // 2, thick=2)
        self.line(cx + half // 2, y_base - 2, cx + 1, y_apex + span // 2, thick=2)
        for y in range(y_apex - 3, y_base + 3):                       # the verges, four thick
            hw = int(round((half + 3) * (y - (y_apex - 3)) / float(span + 5)))
            for t in range(4):
                for side in (-1, 1):
                    x = cx + side * (hw - t)
                    if (side < 0 and x <= cx) or (side > 0 and x >= cx):
                        self.put(x, y, pick(pal, (4.0 if side < 0 else 2.2) - t * 0.7))

    def cone(self, cx, base_y, half, height):
        for j in range(height):
            y = base_y - j
            w = int(round((half + 2) * (1.0 - j / float(height))))
            for x in range(cx - w, cx + w + 1):
                u = (x - (cx - w)) / max(1.0, 2.0 * w)
                tone = 4.2 - 4.6 * abs(u - 0.28) ** 1.1
                if j % 3 == 2:
                    tone -= 1.3
                elif (x + (j // 3) * 2) % 4 == 0:
                    tone -= 0.6
                self.put(x, y, pick(SLATE, tone))
        for x in range(cx - half - 2, cx + half + 3):
            self.put(x, base_y + 1, SLATE[0])

    # -- fittings ----------------------------------------------------------------
    def window(self, x, y, w=4, h=6, lit=True, arched=False):
        for j in range(h):
            for i in range(w):
                if arched and j == 0 and i in (0, w - 1):
                    continue
                c = (GLOW[2] if j < h - 2 else GLOW[1]) if lit else DARK
                self.put(x + i, y + j, c)
        for j in range(-1, h + 1):
            self.put(x - 1, y + j, BEAM[1]); self.put(x + w, y + j, BEAM[1])
        for i in range(-1, w + 1):
            self.put(x + i, y - 1, BEAM[1]); self.put(x + i, y + h, BEAM[3])
        for j in range(h):
            self.put(x + w // 2, y + j, BEAM[1])                      # mullion
        for i in range(w):
            self.put(x + i, y + h // 2, BEAM[1])

    def door(self, cx, y_bot, half, height, surround=STONE):
        """An arched plank door with iron straps, set in a stone or oak surround."""
        y_top = y_bot - height + 1
        for y in range(y_top, y_bot + 1):
            j = y - y_top
            w = half if j >= half else int(round((half ** 2 - (half - j) ** 2) ** 0.5))
            for x in range(cx - w, cx + w + 1):
                tone = 1.6 + (0.8 if (x - cx) % 3 == 0 else 0.0) - (1.0 if x == cx else 0.0) + (hsh(x, y // 3, 21) - 0.5)
                self.put(x, y, pick(WOOD, tone))
            for x in (cx - w - 1, cx + w + 1):
                self.put(x, y, pick(surround, 4 + ((y // 3) % 2)) if surround is STONE else BEAM[2])
            if j == 0:
                for x in range(cx - w - 1, cx + w + 2):
                    self.put(x, y - 1, pick(surround, 5) if surround is STONE else BEAM[2])
            if j in (half + 2, height - 5):
                for x in range(cx - w + 1, cx + w):
                    self.put(x, y, IRON[1])
        self.put(cx - 2, y_bot - height // 3, IRON[3]); self.put(cx + 2, y_bot - height // 3, IRON[3])

    def banner(self, x, y_top, width=7, length=24, device=True):
        for i in range(-1, width + 1):
            self.put(x + i, y_top - 1, BEAM[1])
        for j in range(length):
            for i in range(width):
                if j > length - 4 and abs(i - width // 2) < (j - (length - 4)):
                    continue
                self.put(x + i, y_top + j, BANNER[3 if i == 1 else (2 if i < width - 2 else 1)])
            if j in (2, length - 6):
                for i in range(width):
                    self.put(x + i, y_top + j, GOLD[1])
        if device:
            cy, m = y_top + length // 2 - 3, width // 2
            for dx in (-2, 0, 2):
                self.put(x + m + dx, cy, GOLD[2])
            for dx in range(-2, 3):
                self.put(x + m + dx, cy + 1, GOLD[2]); self.put(x + m + dx, cy + 2, GOLD[1])

    def flag(self, x, y_top, length=11, drop=7, right=True, pole=16):
        for j in range(pole):
            self.put(x, y_top + j, BEAM[1])
        self.put(x, y_top - 1, GOLD[2])
        d = 1 if right else -1
        for i in range(1, length + 1):
            wave = 1 if (i // 3) % 2 else 0
            for j in range(drop - (1 if i > length - 3 else 0)):
                tone = 2 if j < 2 else (1 if j < drop - 2 else 0)
                self.put(x + d * i, y_top + 1 + j + wave, BANNER[min(3, tone + (1 if j == 0 else 0))])
            self.put(x + d * i, y_top + 3 + wave, GOLD[1])

    def torch(self, x, y):
        self.put(x, y + 2, BEAM[1]); self.put(x, y + 3, BEAM[1])
        self.put(x, y + 1, GLOW[1]); self.put(x, y, GLOW[2]); self.put(x, y - 1, GLOW[1])
        self.put(x - 1, y, GLOW[0]); self.put(x + 1, y, GLOW[0])

    def shield(self, cx, cy, r=4):
        for y in range(-r, r + 1):
            for x in range(-r, r + 1):
                d = (x * x + y * y) ** 0.5
                if d <= r + 0.3:
                    c = IRON[2] if d > r - 1 else (BANNER[2] if x < 0 else BANNER[1])
                    self.put(cx + x, cy + y, c)
        self.put(cx, cy, GOLD[2])

    def barrel(self, x0, y_bot, w=9, h=12):
        for y in range(y_bot - h + 1, y_bot + 1):
            j = y - (y_bot - h + 1)
            bulge = 0 if j in (0, h - 1) else 1
            for x in range(x0 - bulge, x0 + w + bulge):
                u = (x - x0) / float(w)
                tone = 3.4 - 2.6 * abs(u - 0.3) - (0.8 if (x - x0) % 3 == 2 else 0)
                self.put(x, y, pick(WOOD, tone) if j not in (2, h - 3) else pick(IRON, 2.6 - 2 * abs(u - 0.3)))

    def crate(self, x0, y_bot, s=10):
        self.planks(x0, y_bot - s + 1, x0 + s - 1, y_bot, light=0.4, pw=3, upright=False)
        for i in range(s):
            self.put(x0 + i, y_bot - s + 1, BEAM[2]); self.put(x0 + i, y_bot, BEAM[1])
            self.put(x0, y_bot - i, BEAM[2]); self.put(x0 + s - 1, y_bot - i, BEAM[1])
            self.put(x0 + i, y_bot - i, BEAM[2])

    def moss(self, x0, x1, y_base, amount, seed=1):
        for k in range(amount):
            cx = x0 + int(hsh(k, seed, 1) * (x1 - x0))
            cy = y_base - int(hsh(k, seed, 2) ** 2 * 10)
            for m in range(3 + int(hsh(k, seed, 3) * 6)):
                x, y = cx + int(hsh(k, m, 4) * 5) - 2, cy + int(hsh(k, m, 5) * 4) - 2
                if self.solid(x, y) and int(self.px[y, x, :3].sum()) > 150:
                    self.put(x, y, MOSS[int(hsh(k, m, 6) * 3) % 3])

    # -- finishing ---------------------------------------------------------------
    def shadow(self, x0, x1, y0, y1, strength=40):
        for y in range(max(0, y0), min(self.h, y1 + 1)):
            for x in range(max(0, x0), min(self.w, x1 + 1)):
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
        return Image.fromarray(self.px, "RGBA").resize((self.w * SCALE, self.h * SCALE), Image.NEAREST)


# ---------------------------------------------------------------------------
# The pieces
# ---------------------------------------------------------------------------
def brazier():
    """An iron fire basket on three legs, burning."""
    c = Canvas(16, 36)
    g = c.ground
    for y in range(g - 14, g + 1):
        j = y - (g - 14)
        c.put(8 - 1 - j // 3, y, IRON[2]); c.put(8 + 1 + j // 3, y, IRON[1]); c.put(8, y, IRON[1])
    for y in range(g - 20, g - 13):
        for x in range(3, 13):
            if (y - (g - 20)) + 3 >= abs(x - 7.5) - 2:
                c.put(x, y, pick(IRON, 2.6 - 0.35 * (x - 3)) if (x + y) % 3 else GLOW[0])
    c.outline()
    flame_into(c, 8, g - 20, 5, 14)
    return c


def flame_into(c, cx, y_bot, half, height):
    for j in range(height):
        y = y_bot - j
        t = j / float(height)
        w = half * (1.0 - t) ** 0.7 * (0.75 + 0.25 * min(1.0, j / 3.0))
        lean = int(round(1.2 * np.sin(j * 0.8)))
        for x in range(int(cx - w + lean), int(cx + w + lean) + 1):
            d = abs(x - cx - lean) / max(1.0, w)
            c.put(x, y, pick(FIRE, 4.4 - 3.2 * d - 2.6 * t))


def flame():
    c = Canvas(24, 30)
    flame_into(c, 12, c.ground, 9, 28)
    return c


def firepit():
    """A ring of stones with logs laid across and a spit over it."""
    c = Canvas(72, 36)
    g = c.ground
    for i, (x, w, h) in enumerate([(8, 9, 6), (16, 8, 7), (25, 10, 6), (36, 9, 7), (46, 10, 6), (55, 9, 7)]):
        for yy in range(h):
            inset = 1 if yy in (0, h - 1) else 0
            for xx in range(x + inset, x + w - inset):
                c.put(xx, g - h + 1 + yy, pick(STONE, 4.4 - yy * 0.6 - (xx - x) * 0.25 + (hsh(xx, yy, i) - 0.5)))
    c.line(22, g - 9, 48, g - 6, pal=WOOD, tone=2.0, thick=3)
    c.line(24, g - 6, 50, g - 10, pal=WOOD, tone=1.0, thick=3)
    for x in (5, 66):                                                # forked uprights
        c.beam(x, g - 30, x + 1, g)
        c.put(x - 1, g - 31, BEAM[2]); c.put(x + 2, g - 31, BEAM[2]); c.put(x - 2, g - 33, BEAM[2]); c.put(x + 3, g - 33, BEAM[2])
        c.put(x - 1, g - 32, BEAM[2]); c.put(x + 2, g - 32, BEAM[2])
    for x in range(4, 69):
        c.put(x, g - 30, IRON[2])
    c.outline()
    return c


def cottage():
    """A peasant's cot: wattle walls in an oak frame under a deep thatch."""
    c = Canvas(76, 56)
    g = c.ground
    c.stone(46, 4, 52, 20, light=-0.3, bw=4, course=3)               # chimney
    c.half_timber(10, 30, 65, g - 3, bay=14)
    c.stone(9, g - 2, 66, g, light=-0.4, bw=5, course=3)
    c.door(24, g - 3, 5, 18, surround=BEAM)
    c.window(42, 38, w=5, h=6)
    c.window(55, 38, w=4, h=6, lit=False)
    c.roof(38, 9, 31, 20, 36, "thatch")
    c.shadow(10, 65, 33, 35, 34)
    c.barrel(66, g, w=7, h=10)
    c.moss(9, 66, g, 12, seed=3)
    c.outline()
    return c


def hall(tier):
    """The clan's hall. tier 1 is an oak-framed longhall under thatch; tier 2
    stands on a stone storey, roofed in slate, with a gate tower and banners."""
    if tier == 1:
        c = Canvas(250, 170)
        g = c.ground
        c.stone(60, 30, 70, 56, light=-0.4, bw=5, course=3)          # chimney stack
        c.half_timber(22, 112, 227, g - 5, bay=22)
        c.stone(20, g - 4, 229, g, light=-0.3)
        for x in (33, 55, 77, 165, 187, 209):
            c.window(x, 124, w=5, h=8, arched=True)
        c.roof(125, 44, 113, 78, 116, "thatch")
        c.shadow(22, 227, 116, 119, 36)
        # the porch: a framed gable standing forward of the roof
        c.half_timber(98, 104, 152, g - 5, bay=27, braces=False)
        c.gable(125, 62, 104, 30, "thatch")
        c.window(123, 84, w=4, h=7, arched=True)
        c.door(125, g - 5, 13, 40, surround=BEAM)
        c.banner(102, 112, width=7, length=26); c.banner(141, 112, width=7, length=26)
        c.flag(125, 34, length=14, drop=8, pole=26)
        for side in (-1, 1):                                         # crossed finials at the ridge ends
            x = 125 + side * 78
            c.line(x, 46, x + side * 6, 36, thick=2); c.line(x, 46, x - side * 3, 37, thick=2)
        c.moss(20, 229, g, 40, seed=5)
    else:
        c = Canvas(256, 216)
        g = c.ground
        c.stone(196, 44, 208, 74, light=-0.4, bw=5, course=3)        # chimney
        c.stone(24, 150, 231, g, light=0.0)                          # stone ground storey
        for x in (40, 66, 172, 198):
            c.window(x, 164, w=5, h=10, arched=True)
        c.half_timber(18, 108, 237, 150, bay=22)                     # jettied upper storey
        c.shadow(24, 231, 151, 154, 42)
        for x in (30, 52, 74, 162, 184, 206):
            c.window(x, 118, w=5, h=8)
        c.roof(128, 58, 109, 84, 124, "slate")
        c.shadow(18, 237, 112, 114, 34)
        for x in (60, 196):                                          # dormers
            c.rect(x - 7, 82, x + 7, 98, PLASTER[2])
            c.window(x - 2, 87, w=4, h=7)
            c.gable(x, 72, 84, 10, "slate")
        # gate tower
        c.stone(102, 60, 154, g, light=0.5)
        c.shadow(155, 160, 110, g, 30)
        for x in range(100, 157, 8):
            c.stone(x, 53, min(156, x + 4), 60, light=0.9)
        c.stone(100, 60, 156, 64, light=0.9)
        c.cone(128, 52, 24, 34)
        c.flag(128, 4, length=15, drop=8, pole=18)
        c.window(118, 78, w=4, h=9, arched=True); c.window(134, 78, w=4, h=9, arched=True)
        c.banner(123, 100, width=11, length=40)
        c.door(128, g, 15, 46)
        c.banner(106, 150, width=7, length=26); c.banner(144, 150, width=7, length=26)
        c.torch(110, g - 20); c.torch(146, g - 20)
        c.moss(24, 231, g, 55, seed=6)
    c.outline()
    return c


def tower():
    """The border watchtower: a stone drum, a timber fighting gallery, a slate cap."""
    c = Canvas(80, 184)
    g = c.ground
    c.stone(24, 74, 55, g, light=0.3, round_=True)
    for i, y in enumerate((92, 118, 144)):
        for j in range(5):
            c.put(34 + (i % 2) * 12, y + j, DARK)
    c.door(40, g, 6, 18)
    for x in (22, 38, 55):                                           # brackets under the gallery
        c.line(x, 76, x + (-6 if x < 30 else (6 if x > 50 else 0)), 66, thick=2)
    c.planks(14, 52, 65, 68, light=0.2, pw=4)                        # hoarding
    c.beam(13, 66, 66, 68); c.beam(13, 50, 66, 52)
    for x in (22, 37, 52):
        c.rect(x, 56, x + 4, 60, DARK)
    for x in (15, 31, 47, 63):                                       # posts of the open gallery
        c.beam(x, 34, x + 1, 50)
    c.rect(17, 36, 62, 49, (26, 22, 28))                             # the dark inside
    for x in (15, 31, 47, 63):
        c.beam(x, 34, x + 1, 50)
    c.cone(40, 34, 28, 30)
    c.flag(40, 0, length=11, drop=7, pole=8)
    c.shield(40, 60, 4)
    c.torch(31, g - 18)
    c.moss(24, 55, g, 16, seed=8)
    c.outline()
    return c


def platform():
    """A lookout: a railed deck on four legs with a ladder and a scrap of thatch."""
    c = Canvas(76, 100)
    g = c.ground
    for x in (16, 57):
        c.log(x, 40, x + 3, g)
    c.line(19, g - 4, 57, 46, thick=2); c.line(57, g - 4, 19, 46, thick=2)
    c.beam(14, 68, 61, 69)
    for x in (33, 41):                                               # ladder
        c.beam(x, 40, x + 1, g)
    for y in range(46, g, 6):
        for x in range(34, 42):
            c.put(x, y, WOOD[3])
    c.planks(8, 38, 67, 42, light=0.6, pw=5, upright=False)          # deck
    c.planks(9, 28, 66, 37, light=0.0, pw=4)                         # rail
    c.beam(8, 27, 67, 28)
    for x in (10, 64):
        c.beam(x, 14, x + 1, 28)
    c.roof(38, 4, 15, 12, 35, "thatch")
    c.shield(22, 33, 3); c.shield(53, 33, 3)
    c.outline()
    return c


def rack():
    """Tools and arms on an oak rack, with stores stacked at its foot."""
    c = Canvas(104, 72)
    g = c.ground
    for x in (10, 92):
        c.log(x, 14, x + 3, g)
    c.beam(6, 18, 99, 20); c.beam(8, 44, 97, 45)
    for i, x in enumerate((22, 30, 38)):                             # spears
        for y in range(2, g - 2):
            c.put(x, y, WOOD[3])
        for j in range(6):
            for dx in range(-(j // 2 if j < 4 else 1), (j // 2 if j < 4 else 1) + 1):
                c.put(x + dx, j - 2 + 2, pick(IRON, 3.5 - dx))
    for x in (52, 64):                                               # axes hung by the head
        for y in range(22, 50):
            c.put(x, y, WOOD[4]); c.put(x + 1, y, WOOD[2])
        for yy in range(8):
            for xx in range(6 - abs(yy - 4) // 2):
                c.put(x + 2 + xx, 22 + yy, pick(IRON, 3.6 - xx * 0.5))
    c.shield(80, 32, 7)
    c.barrel(44, g, w=9, h=13)
    c.crate(60, g, 11); c.crate(72, g, 9)
    c.outline()
    return c


def woodpile():
    """Cut logs stacked end-on between two stakes."""
    c = Canvas(84, 38)
    g = c.ground
    for row in range(4):
        n = 8 - row
        for i in range(n):
            cx, cy = 14 + i * 8 + row * 4, g - 4 - row * 7
            for y in range(-4, 5):
                for x in range(-4, 5):
                    d = (x * x + y * y) ** 0.5
                    if d <= 4.3:
                        tone = 1.0 if d > 3.2 else (4.6 - d * 0.5 - (0.8 if int(d) == 2 else 0) + (hsh(i, row, 7) - 0.5))
                        c.put(cx + x, cy + y, pick(WOOD, tone))
    for x in (4, 77):
        c.log(x, 4, x + 2, g, tip=True)
    c.outline()
    return c


def stonepile():
    """Dressed blocks from the quarry, stacked for the masons."""
    c = Canvas(66, 40)
    g = c.ground
    for (x, y, w, h, l) in [(4, 28, 18, 12, 0.2), (23, 26, 20, 14, 0.6), (44, 29, 18, 11, -0.2),
                            (10, 15, 19, 12, 0.8), (30, 13, 20, 12, 0.3), (20, 3, 17, 10, 1.0)]:
        for yy in range(h):
            for xx in range(w):
                tone = 3.4 + l - xx * 0.09 - yy * 0.12 + (hsh(xx // 2, yy // 2, x) - 0.5) * 0.9
                if yy == 0 or xx == 0:
                    tone += 1.2
                if yy == h - 1 or xx == w - 1:
                    tone -= 1.4
                c.put(x + xx, y + yy, pick(STONE, tone))
    c.moss(4, 60, g, 8, seed=9)
    c.outline()
    return c


def standard():
    """The boundary standard: a tall pole in a cairn, flying the clan's long banner."""
    c = Canvas(40, 108)
    g = c.ground
    for y in range(6, g - 6):
        c.put(19, y, WOOD[4]); c.put(20, y, WOOD[2])
    for dx, dy in ((0, 0), (1, 0), (-1, 1), (2, 1), (0, 1), (1, 1), (0, 2), (1, 2), (0, -1), (1, -1)):
        c.put(19 + dx, 3 + dy, GOLD[2 if dx <= 0 else 1])
    c.beam(7, 10, 32, 11)
    c.put(7, 12, GOLD[1]); c.put(32, 12, GOLD[1])
    c.banner(10, 13, width=20, length=52, device=False)
    for y in range(30, 36):                                          # the device: a crown
        for x in range(14, 26):
            if y >= 32 or (x - 14) % 4 < 2:
                c.put(x, y, GOLD[2] if y < 34 else GOLD[1])
    for i, (x, w, h) in enumerate([(8, 9, 6), (16, 9, 8), (24, 9, 6), (12, 8, 5), (20, 8, 5)]):
        top = g - h + 1 - (6 if i >= 3 else 0)
        for yy in range(h):
            inset = 1 if yy in (0, h - 1) else 0
            for xx in range(x + inset, x + w - inset):
                c.put(xx, top + yy, pick(STONE, 4.6 - yy * 0.5 - (xx - x) * 0.3 + (hsh(xx, yy, i) - 0.5)))
    c.outline()
    return c


def palisade(width, height, log_w, rail_ys):
    """One tiling length of sharpened-log wall. Its ends meet the next length cleanly."""
    c = Canvas(width, height)
    g = c.ground
    n = width // log_w
    for i in range(n):
        top = int(hsh(i, width, 31) * 4)
        c.log(i * log_w, top, i * log_w + log_w - 2, g, light=(hsh(i, width, 33) - 0.5) * 0.8, tip=True)
        for y in range(top + log_w // 2, g + 1):
            c.put(i * log_w + log_w - 1, y, BEAM[0])                 # the dark between logs
    for ry in rail_ys:
        for x in range(width):
            c.put(x, ry, BEAM[3]); c.put(x, ry + 1, BEAM[1])
            if x % log_w == log_w // 2:
                c.put(x, ry, IRON[3])                                # a nail in every log
    a = c.px[..., 3] > 0                                             # outline the tops only, never the tile's ends
    edge = np.zeros_like(a)
    edge[1:, :] |= a[1:, :] & ~a[:-1, :]
    c.px[edge, :3] = OUTLINE
    c.moss(0, width - 1, g, max(3, width // 8), seed=width)
    return c


PIECES = [
    ("rectHallTimber", lambda: hall(1)),
    ("rectHallStone", lambda: hall(2)),
    ("rectLookpost", tower),
    ("rectLookpostBamboo", platform),
    ("rectBorderMonument", standard),
    ("rectToolRack", rack),
    ("rectVillageHut", cottage),
    ("rectFirePit", firepit),
    ("rectBrazier", brazier),
    ("rectFxFire", flame),
    ("rectMeetingHollowLog", woodpile),
    ("rectMeetingStone", stonepile),
    ("rectPalisadeMiddle", lambda: palisade(64, 46, 8, (14, 34))),
    ("rectPalisadeRear", lambda: palisade(60, 25, 5, (9,))),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview")
    args = ap.parse_args()

    imgs = [(name, make().image()) for name, make in PIECES]
    cells, x, y, shelf = {}, PAD, PAD, 0
    for name, img in sorted(imgs, key=lambda p: -p[1].size[1]):
        if x + img.size[0] + PAD > ATLAS_W:
            x, y, shelf = PAD, y + shelf + PAD, 0
        cells[name] = (x, y, img.size[0], img.size[1])
        x += img.size[0] + PAD
        shelf = max(shelf, img.size[1])
    atlas = Image.new("RGBA", (ATLAS_W, y + shelf + PAD), (0, 0, 0, 0))
    for name, img in imgs:
        atlas.paste(img, cells[name][:2])
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    atlas.save(OUT)
    print("wrote", OUT, atlas.size)
    for name, _ in PIECES:
        cx, cy, w, h = cells[name]
        print(f"    const sf::IntRect {name:<21}= sf::IntRect({cx}, {cy}, {w}, {h});")

    if args.preview:
        total = sum(img.size[0] + 12 for _, img in imgs) + 12
        tall = max(img.size[1] for _, img in imgs)
        half = total // 2 + 400
        sheet = Image.new("RGBA", (half, tall * 2 + 60), (120, 170, 210, 255))
        px, row = 12, 0
        for _, img in imgs:
            if px + img.size[0] + 12 > half:
                px, row = 12, row + 1
            sheet.paste(img, (px, (row + 1) * (tall + 20) - img.size[1]), img)
            px += img.size[0] + 12
        sheet.convert("RGB").save(args.preview)


if __name__ == "__main__":
    main()
