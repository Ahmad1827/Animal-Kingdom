#!/usr/bin/env python3
"""Dress the ape sprite sheet in medieval clothing.

Reads   assets/sprites/spritesheet.png            (8 x 4 frames, the bare ape)
Writes  assets/sprites/medieval/apes_<outfit>.png (same layout, one sheet per outfit)

Nothing is drawn freehand over the art. Each frame is taken apart by colour
(bare skin against fur), the chest and belly are found, and the cloth is made
by repainting those pixels with a cloth palette while keeping the original
light and shade. That is why the folds follow the body and the result still
looks like it was painted with the rest of the sheet.

Run from the project root:   python3 tools/dress_apes.py
Add an outfit by adding an entry to OUTFITS; preview with --preview.
"""
import argparse
import os

import numpy as np
from PIL import Image
from scipy import ndimage

SRC = "assets/sprites/spritesheet.png"
OUT_DIR = "assets/sprites/medieval"
COLS, ROWS = 8, 4
BLOCK = 5                 # the sheet's own "pixel" is about this many real pixels

# Palettes run dark -> light.
def ramp(*rgb):
    return np.array(rgb, dtype=np.float32)

GREEN  = ramp((30, 46, 34), (44, 70, 46), (60, 96, 58), (84, 124, 72), (116, 152, 92), (150, 178, 116))
OCHRE  = ramp((70, 44, 24), (100, 66, 32), (134, 92, 44), (168, 122, 60), (198, 154, 84), (222, 184, 116))
BLUE   = ramp((28, 36, 62), (38, 54, 92), (52, 76, 124), (72, 102, 152), (100, 132, 178), (136, 164, 200))
RED    = ramp((70, 14, 20), (106, 20, 26), (146, 28, 32), (182, 44, 42), (210, 72, 58), (232, 110, 86))
PURPLE = ramp((44, 22, 62), (66, 34, 92), (92, 50, 124), (120, 72, 154), (150, 100, 182), (184, 138, 208))
STEEL  = ramp((44, 48, 58), (70, 76, 90), (102, 110, 126), (140, 148, 164), (180, 188, 202), (222, 228, 236))
ERMINE = ramp((120, 112, 104), (160, 152, 142), (196, 190, 180), (222, 218, 208), (240, 237, 230), (252, 250, 246))
LEATHER = ramp((40, 24, 14), (62, 38, 20), (88, 56, 30), (116, 78, 42))
GOLD   = ramp((120, 78, 20), (176, 122, 28), (226, 172, 48), (252, 222, 104))
OUTLINE = np.array((24, 14, 10), dtype=np.float32)

OUTFITS = {
    # name:      tunic    trim    head             shoulders        emblem
    "peasant_a": dict(tunic=GREEN,  trim=LEATHER, head=None,      mantle=None,   emblem=None,    belt="rope"),
    "peasant_b": dict(tunic=OCHRE,  trim=LEATHER, head="coif",    mantle=None,   emblem=None,    belt="rope"),
    "peasant_c": dict(tunic=BLUE,   trim=LEATHER, head=None,      mantle=None,   emblem=None,    belt="leather"),
    "guard":     dict(tunic=RED,    trim=GOLD,    head="helm",    mantle=STEEL,  emblem="cross", belt="leather"),
    "noble":     dict(tunic=PURPLE, trim=GOLD,    head="circlet", mantle=None,   emblem=None,    belt="gold", cloak=BLUE),
    "king":      dict(tunic=RED,    trim=GOLD,    head="crown",   mantle=ERMINE, emblem="crown", belt="gold", cloak=RED),
}


def luminance(rgb):
    return (0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]) / 255.0


def paint(frame, mask, palette, lum, lo, hi):
    """Repaint mask with palette, picking the shade from each pixel's own brightness."""
    if not mask.any():
        return
    t = np.clip((lum[mask] - lo) / max(hi - lo, 1e-4), 0.0, 0.999)
    idx = (t * len(palette)).astype(int)
    frame[mask, :3] = palette[idx]


def ellipse(rx, ry):
    y, x = np.ogrid[-ry:ry + 1, -rx:rx + 1]
    return (x * x) / float(rx * rx) + (y * y) / float(ry * ry) <= 1.0


def blocks(frame, pattern, cx, top, palette_map, opaque_only=None):
    """Stamp a small pixel pattern, one character per BLOCK x BLOCK cell, centred on cx."""
    h, w = len(pattern), len(pattern[0])
    x0 = int(round(cx - w * BLOCK / 2.0))
    for j, row in enumerate(pattern):
        for i, ch in enumerate(row):
            if ch == '.':
                continue
            ys, xs = top + j * BLOCK, x0 + i * BLOCK
            y1, x1 = min(frame.shape[0], ys + BLOCK), min(frame.shape[1], xs + BLOCK)
            if ys < 0 or xs < 0 or ys >= y1 or xs >= x1:
                continue
            cell = frame[ys:y1, xs:x1]
            if opaque_only is not None:
                sel = opaque_only[ys:y1, xs:x1]
                cell[sel, :3] = palette_map[ch]
                cell[sel, 3] = 255
            else:
                cell[..., :3] = palette_map[ch]
                cell[..., 3] = 255


def snap(mask):
    """Round a mask to the sheet's own pixel grid, so new edges are as chunky as the old ones."""
    h, w = mask.shape
    hh, ww = (h // BLOCK) * BLOCK, (w // BLOCK) * BLOCK
    cells = mask[:hh, :ww].reshape(hh // BLOCK, BLOCK, ww // BLOCK, BLOCK).mean(axis=(1, 3)) >= 0.5
    out = np.zeros_like(mask)
    out[:hh, :ww] = np.repeat(np.repeat(cells, BLOCK, axis=0), BLOCK, axis=1)
    return out


def outline_new(frame, added, body):
    """Dark edge where newly drawn cloth meets empty space, like the rest of the sprite has."""
    solid = added | body
    edge = added & ~ndimage.binary_erosion(solid, iterations=2)
    frame[edge, :3] = OUTLINE


CROWN = [
    "G..G..G..G",
    "GG.GG.GG.G",
    "GYGYRGYGYG",
    "gGGGGGGGGg",
]
CIRCLET = [
    "..G..",
    "gGRGg",
]
EMBLEM_CROSS = [
    ".Y.",
    "YYY",
    ".Y.",
    ".Y.",
]
EMBLEM_CROWN = [
    "Y.Y.Y",
    "YYYYY",
]
GOLD_MAP = {'G': GOLD[2], 'Y': GOLD[3], 'g': GOLD[1], 'R': RED[3]}


def dress(frame, outfit):
    """frame: HxWx4 float32 array for one animation frame, edited in place."""
    alpha = frame[..., 3] > 40
    if alpha.sum() < 200:
        return
    rgb = frame[..., :3]
    lum = luminance(rgb)
    ys, xs = np.where(alpha)
    y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
    H, W = y1 - y0 + 1, x1 - x0 + 1

    # ---- take the body apart by colour --------------------------------------------
    skin = alpha & (rgb[..., 0] > 140) & (rgb[..., 1] > 92) & ((rgb[..., 0] - rgb[..., 2]) > 45)
    skin = ndimage.binary_opening(skin, iterations=1)
    labels, count = ndimage.label(skin)
    face = np.zeros_like(alpha)
    chest = np.zeros_like(alpha)
    other_skin = np.zeros_like(alpha)
    face_area = 0
    for i in range(1, count + 1):
        comp = labels == i
        area = comp.sum()
        if area < 40:
            continue
        cy, cx = ndimage.center_of_mass(comp)
        ry, rx = (cy - y0) / H, (cx - x0) / W
        cys, cxs = np.where(comp)
        touches_floor = cys.max() > y1 - 0.06 * H
        if ry < 0.42 and area > max(face_area, 700):  # the biggest patch of skin up top is the face (a raised fist is smaller)
            other_skin |= face
            face, face_area = comp, area
        elif 0.30 <= ry <= 0.74 and 0.16 <= rx <= 0.84 and area > 260 and not touches_floor:
            chest |= comp
        else:
            other_skin |= comp                       # hands, feet, ears
    fur = alpha & ~skin

    back_view = not chest.any()
    if back_view:
        # Climbing frames show the back: there is no chest to find, so the cloth
        # is a band across the trunk instead.
        yy, xx = np.mgrid[0:frame.shape[0], 0:frame.shape[1]]
        chest = alpha & (yy > y0 + 0.36 * H) & (yy < y0 + 0.62 * H) & (xx > x0 + 0.22 * W) & (xx < x0 + 0.72 * W)

    cys, cxs = np.where(chest)
    cy0, cy1, cx0, cx1 = cys.min(), cys.max(), cxs.min(), cxs.max()
    ch_h = cy1 - cy0 + 1
    ccx = (cx0 + cx1) / 2.0
    keep_bare = ndimage.binary_dilation(other_skin | face, iterations=2)

    yy, xx = np.mgrid[0:frame.shape[0], 0:frame.shape[1]]
    trim = outfit["trim"]

    # ---- cloak hanging behind (drawn first: everything else covers it) -------------------
    if outfit.get("cloak") is not None and not back_view:
        # A sheet of cloth from the shoulders to the knees, behind the body: it
        # shows past the back and through the gap between arm and flank.
        cloak = np.zeros_like(alpha)
        c_top, c_bot = cy0 - int(0.05 * H), y0 + int(0.78 * H)
        for y in range(max(0, c_top), min(frame.shape[0], c_bot)):
            row = np.where(alpha[y])[0]
            if not len(row):
                continue
            t = (y - c_top) / float(max(1, c_bot - c_top))
            left = int(row.min() - BLOCK * (1.0 + 3.0 * t))          # flares toward the hem
            cloak[y, max(0, left):int(ccx)] = True
        cloak &= ~alpha
        cloak = snap(cloak)
        cb = (xx // BLOCK) * BLOCK
        shade = 0.62 - 0.30 * np.clip((yy - cy0) / float(H), 0, 1) + 0.12 * np.cos(cb / (1.6 * BLOCK))
        frame[cloak, 3] = 255
        paint(frame, cloak, outfit["cloak"], shade, 0.0, 1.0)
        outline_new(frame, cloak, alpha)

    # ---- tunic: a garment with its own silhouette, hung from the chest ---------------------
    # Row by row: as wide as the chest down to the belly, then flaring into a skirt
    # that covers the gap between the legs like real cloth would.
    grown = ndimage.binary_dilation(chest, structure=ellipse(11, 7))
    top = max(0, cy0 - BLOCK)
    hem_y = min(frame.shape[0] - 1, cy1 + int(0.15 * H))
    lefts, rights = [], []
    last = None
    for y in range(top, hem_y + 1):
        row = np.where(grown[y])[0] if y <= cy1 else []
        if len(row):
            last = [float(row.min()), float(row.max())]
        elif last is not None and y > cy1:
            last = [last[0] - 0.30, last[1] + 0.30]          # the skirt flares
        lefts.append(last[0] if last else ccx)
        rights.append(last[1] if last else ccx)
    lefts = ndimage.uniform_filter1d(np.array(lefts), 9, mode="nearest")
    rights = ndimage.uniform_filter1d(np.array(rights), 9, mode="nearest")

    tunic = np.zeros_like(alpha)
    half = np.ones(frame.shape[:2], dtype=np.float32)
    mid = np.full(frame.shape[:2], ccx, dtype=np.float32)
    for i, y in enumerate(range(top, hem_y + 1)):
        l, r = int(round(lefts[i])), int(round(rights[i]))
        if r <= l:
            continue
        tunic[y, max(0, l):min(frame.shape[1], r + 1)] = True
        half[y, :] = max(1.0, (r - l) / 2.0)
        mid[y, :] = (l + r) / 2.0
    belt_y = cy0 + int(ch_h * 0.70)
    tunic = snap(tunic)
    tunic &= ~keep_bare
    tunic &= alpha | (yy > belt_y)                           # above the belt it stays on the body
    new_cloth = tunic & ~alpha

    # Shade it as cloth, not as the muscles underneath: darker at the sides,
    # lit from the upper front, folds in the skirt. Worked out per art-pixel.
    by, bx = (yy // BLOCK) * BLOCK + BLOCK // 2, (xx // BLOCK) * BLOCK + BLOCK // 2
    u = np.clip((bx - mid) / half, -1.2, 1.2)
    soft = ndimage.gaussian_filter(lum, 9)
    shade = 0.60 - 0.26 * np.abs(u) ** 1.6 + 0.07 * u + 0.30 * (soft - 0.45)
    skirt_zone = by > belt_y
    shade = shade + np.where(skirt_zone, 0.10 * np.cos(u * 3.0 * np.pi), 0.0)
    shade = shade - np.where((by > belt_y) & (by < belt_y + 2 * BLOCK), 0.14, 0.0)
    frame[new_cloth, 3] = 255
    paint(frame, tunic, outfit["tunic"], shade, 0.12, 0.92)

    # ---- hem, neckline, belt -------------------------------------------------------------------
    hem = tunic & (yy > hem_y - BLOCK)
    frame[hem, :3] = trim[min(2, len(trim) - 1)]
    neck = tunic & (yy < top + BLOCK + 2)
    frame[neck, :3] = trim[min(1, len(trim) - 1)]

    belt = tunic & (yy >= belt_y - 3) & (yy <= belt_y + 3)
    if outfit["belt"] == "rope":
        paint(frame, belt, ramp((112, 88, 54), (150, 122, 78), (190, 162, 110)), shade, 0.2, 0.9)
    elif outfit["belt"] == "gold":
        paint(frame, belt, GOLD, shade, 0.2, 0.9)
    else:
        paint(frame, belt, LEATHER, shade, 0.2, 0.9)
    if not back_view and outfit["belt"] != "rope":
        blocks(frame, ["YY", "YY"], ccx, belt_y - BLOCK, {'Y': GOLD[3]}, opaque_only=tunic)
    outline_new(frame, new_cloth, alpha)

    # ---- emblem on the chest -------------------------------------------------------------------------
    if not back_view and outfit["emblem"]:
        pattern = EMBLEM_CROSS if outfit["emblem"] == "cross" else EMBLEM_CROWN
        blocks(frame, pattern, ccx, cy0 + int(ch_h * 0.14), {'Y': GOLD[3]}, opaque_only=tunic)

    # ---- mantle: a rounded collar over the shoulders -------------------------------------------------------
    if outfit["mantle"] is not None:
        my, mx = cy0 + ch_h * 0.02, ccx
        inside = ((xx - mx) / (0.43 * W)) ** 2 + ((yy - my) / (0.17 * H)) ** 2 <= 1.0
        mantle = snap(inside) & fur & ~tunic & ~keep_bare
        if face.any():
            fys, _ = np.where(face)
            mantle &= yy > fys.max() - BLOCK                 # nothing above the chin: that is head
        mantle &= yy > cy0 - int(0.12 * H)                   # a collar, not a shirt
        if back_view:
            mantle[:] = False
        paint(frame, mantle, outfit["mantle"], 0.20 + lum * 2.0, 0.16, 1.0)
        if outfit["mantle"] is ERMINE:
            # black tail-tips, on a regular lattice so they hold still between frames
            spots = mantle & ((yy // BLOCK) % 3 == 1) & (((xx // BLOCK) + (yy // BLOCK) // 3) % 3 == 0)
            frame[spots, :3] = (40, 34, 32)
        mbelow = np.zeros_like(alpha); mbelow[:-BLOCK] = mantle[BLOCK:]
        frame[mantle & ~mbelow, :3] = trim[min(2, len(trim) - 1)]

    # ---- headgear ---------------------------------------------------------------------------------------------
    if face.any() and outfit["head"] and not back_view:
        fys, fxs = np.where(face)
        fy0, fy1, fx0, fx1 = fys.min(), fys.max(), fxs.min(), fxs.max()
        fh = fy1 - fy0 + 1
        head_cx = (fx0 + fx1) / 2.0
        # the fur dome above the brow, a little wider than the face
        dome = fur & (yy < fy0 + fh * 0.10) & (yy > fy0 - fh * 0.75)
        dome &= (xx > fx0 - fh * 0.75) & (xx < fx1 + fh * 0.15)
        dome &= ~ndimage.binary_dilation(other_skin, iterations=3)      # leave the ear alone
        dome &= ~ndimage.binary_dilation(face, iterations=3)            # and the brow
        dys, dxs = np.where(dome)
        if len(dys):
            top = dys.min()
            dcx = (dxs.min() + dxs.max()) / 2.0
            if outfit["head"] == "helm":
                paint(frame, dome, STEEL, 0.10 + lum * 2.6, 0.10, 1.0)
                rim = dome & (yy > dys.max() - 5)
                frame[rim, :3] = STEEL[1]
            elif outfit["head"] == "coif":
                paint(frame, dome, ramp((70, 44, 30), (98, 64, 42), (128, 88, 58), (158, 114, 78), (188, 144, 104)), 0.10 + lum * 2.4, 0.10, 1.0)
                rim = dome & (yy > dys.max() - 4)
                frame[rim, :3] = LEATHER[2]
            elif outfit["head"] == "crown":
                blocks(frame, CROWN, dcx, top - 2 * BLOCK, GOLD_MAP)
            elif outfit["head"] == "circlet":
                blocks(frame, CIRCLET, dcx, top + 1 * BLOCK, GOLD_MAP, opaque_only=None)


def build(outfit_name, sheet):
    out = sheet.copy()
    fh, fw = sheet.shape[0] // ROWS, sheet.shape[1] // COLS
    for r in range(ROWS):
        for c in range(COLS):
            dress(out[r * fh:(r + 1) * fh, c * fw:(c + 1) * fw], OUTFITS[outfit_name])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", help="also write a side-by-side preview image to this path")
    args = ap.parse_args()

    sheet = np.array(Image.open(SRC).convert("RGBA")).astype(np.float32)
    os.makedirs(OUT_DIR, exist_ok=True)
    results = {}
    for name in OUTFITS:
        results[name] = np.clip(build(name, sheet), 0, 255).astype(np.uint8)
        Image.fromarray(results[name], "RGBA").save(os.path.join(OUT_DIR, f"apes_{name}.png"))
        print("wrote", os.path.join(OUT_DIR, f"apes_{name}.png"))

    if args.preview:
        fh, fw = sheet.shape[0] // ROWS, sheet.shape[1] // COLS
        picks = [(0, 0), (1, 3), (2, 4), (3, 1), (3, 4)]           # idle, walk, jump, climb front, climb back
        names = ["bare"] + list(OUTFITS)
        canvas = Image.new("RGBA", (fw * len(names), fh * len(picks)), (74, 112, 84, 255))
        for i, name in enumerate(names):
            src = sheet.astype(np.uint8) if name == "bare" else results[name]
            for j, (r, c) in enumerate(picks):
                cell = Image.fromarray(src[r * fh:(r + 1) * fh, c * fw:(c + 1) * fw], "RGBA")
                canvas.paste(cell, (i * fw, j * fh), cell)
        canvas.convert("RGB").save(args.preview)
        print("wrote", args.preview)


if __name__ == "__main__":
    main()


# ---------------------------------------------------------------------------
# Things apes carry. One character is one BLOCK x BLOCK art pixel, as above.
# Written to assets/sprites/medieval/items.png as a strip; ITEM_RECTS in
# src/entities/Ape.cpp must list the same cells in the same order.
# ---------------------------------------------------------------------------
ITEM_COLORS = {
    'o': (24, 14, 10), 'I': (214, 220, 230), 'i': (140, 148, 164), 'B': (132, 88, 46), 'b': (84, 52, 26),
    'W': (214, 178, 110), 'Y': (164, 122, 62), 'y': (250, 214, 70), 'k': (206, 160, 40),
    'S': (170, 172, 180), 's': (112, 114, 124), 'L': (176, 128, 74), 'R': (178, 44, 42), 'G': (226, 172, 48),
}
ITEMS = [
    ("spear", [".oIo.", "oIIIo", "oIiIo", ".oio.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.",
               ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".oBo.", ".obo.", "..o.."]),
    ("axe",   ["..ooooo..", ".oiIIBo..", "oiIIIBo..", "oIIIIBo..", "oiIIIBo..", ".oiIIBo..", "..oooBo..", "....oBo..",
               "....oBo..", "....oBo..", "....oBo..", "....obo..", ".....o..."]),
    ("basket", [".oooooooo.", "oWYWYWYWYo", "oYWYWYWYWo", "oWYWYWYWYo", "oYWYWYWYWo", ".oYWYWYWo.", "..oooooo.."]),
    ("bananas", ["...oo....", "..oyyo.o.", ".oyykoyyo", "oyykoyyko", "oykoyykko", ".oooykko.", "...oooo.."]),
    ("log",   [".oooooooooo.", "oLBBBBBBBBbo", "oLBbBBBbBBbo", "oLBBBbBBBBbo", ".oooooooooo."]),
    ("stone", ["..oooo..", ".oSSSso.", "oSSSSsso", "oSSssssо".replace("о", "o"), ".oooooo."]),
    ("shield", [".ooooooo.", "oRRRGRRRo", "oRRRGRRRo", "oGGGGGGGo", "oRRRGRRRo", "oRRRGRRRo", ".oRRGRRo.", "..oRGRo..", "...ooo..."]),
]


def make_items():
    width = sum(len(rows[0]) * BLOCK + BLOCK for _, rows in ITEMS)
    height = max(len(rows) for _, rows in ITEMS) * BLOCK
    img = np.zeros((height, width, 4), dtype=np.uint8)
    x0 = 0
    rects = []
    for name, rows in ITEMS:
        w, h = len(rows[0]) * BLOCK, len(rows) * BLOCK
        for j, row in enumerate(rows):
            assert len(row) == len(rows[0]), (name, j)
            for i, ch in enumerate(row):
                if ch == '.':
                    continue
                img[j * BLOCK:(j + 1) * BLOCK, x0 + i * BLOCK:x0 + (i + 1) * BLOCK, :3] = ITEM_COLORS[ch]
                img[j * BLOCK:(j + 1) * BLOCK, x0 + i * BLOCK:x0 + (i + 1) * BLOCK, 3] = 255
        rects.append((name, x0, 0, w, h))
        x0 += w + BLOCK
    Image.fromarray(img, "RGBA").save(os.path.join(OUT_DIR, "items.png"))
    for r in rects:
        print("item %-8s rect(%d, %d, %d, %d)" % r)


if __name__ == "__main__":
    make_items()
