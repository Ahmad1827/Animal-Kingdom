#!/usr/bin/env python3
"""Authoring tool for the strategic map geography.

Counties are described as rings of named points in (longitude, latitude), so a
border shared by two counties is the same pair of names on both sides and can
never drift apart. Running the script checks the topology and rewrites the
"counties" block of assets/data/world_map.json plus the matching fallback table
in include/world/WorldMapRepository.h.

    python3 tools/gen_world_map.py            # regenerate
    python3 tools/gen_world_map.py --preview out.png   # also draw a quick preview
"""
import json, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def project(lon, lat):
    return (round(150 + (lon + 10.5) * 36), round(70 + (58.7 - lat) * 62))

P = {
    # --- south-west peninsula
    "LE": (-5.70, 50.07), "STIVES": (-5.50, 50.24), "TREVOSE": (-5.03, 50.55), "BUDE": (-4.57, 50.80),
    "HARTLAND": (-4.53, 51.02), "BARN": (-4.20, 51.07), "ILFRA": (-4.15, 51.21), "EXMOOR": (-3.75, 51.23),
    "CH1": (-3.30, 50.98), "LYME": (-2.95, 50.72), "EXE": (-3.40, 50.62), "TORBAY": (-3.50, 50.42),
    "START": (-3.65, 50.22), "PLYM": (-4.15, 50.35), "FOWEY": (-4.70, 50.32), "LIZ": (-5.20, 49.97),
    # --- south coast and Severn sea
    "BRIDGW": (-3.05, 51.20), "WESTON": (-3.00, 51.35), "AVON": (-2.70, 51.47), "SEVERN": (-2.50, 51.68),
    "HB1": (-1.90, 51.45), "JHWB": (-0.90, 51.30), "PORTS": (-0.95, 50.80), "SOLENT": (-1.40, 50.78),
    "SWANAGE": (-1.95, 50.60), "PORTLAND": (-2.45, 50.52),
    "JWBM": (-0.45, 51.42), "TH1": (0.10, 51.48), "NOTCH": (0.55, 51.47), "WHIT": (1.00, 51.37),
    "NFORE": (1.44, 51.38), "DOVER": (1.38, 51.13), "DUNGE": (0.97, 50.91), "BEACHY": (0.25, 50.74),
    "SELSEY": (-0.78, 50.72),
    # --- midlands and the east
    "BC1": (-2.35, 51.88), "JBWC": (-2.10, 52.08), "BW1": (-1.30, 52.05), "X4": (-0.45, 52.45),
    "JBMN": (-0.20, 51.95), "MN1": (0.50, 52.02), "ORWELL": (1.20, 51.97), "NAZE": (1.28, 51.85),
    "BLACKW": (0.95, 51.74), "SHOE": (0.85, 51.55),
    "NL1": (-0.10, 52.68), "WASH": (0.12, 52.83), "LYNN": (0.38, 52.78), "HUNST": (0.50, 52.97),
    "CROMER": (1.30, 52.93), "LOWES": (1.75, 52.48), "ALDE": (1.60, 52.15),
    "WL1": (-0.75, 52.85), "JWYL": (-0.95, 53.30), "LY1": (-0.80, 53.55), "HUMBER": (-0.50, 53.70),
    "GRIMSBY": (-0.10, 53.50), "MABLE": (0.28, 53.33), "GIB": (0.33, 53.08),
    "CW1": (-2.30, 52.60), "JCWY": (-2.20, 53.20), "WY1": (-1.50, 53.32),
    # --- Wales and the Marches
    "NEWPORT": (-2.95, 51.55), "CARDIFF": (-3.25, 51.40), "SWANSEA": (-3.85, 51.60), "GOWER": (-4.25, 51.55),
    "TENBY": (-4.65, 51.68), "PEMB": (-5.08, 51.62), "STDAV": (-5.30, 51.88), "CARDIGAN": (-4.70, 52.12),
    "ABERYST": (-4.08, 52.40), "HARLECH": (-4.12, 52.85), "LLYN": (-4.75, 52.80), "CAERN": (-4.35, 53.10),
    "HOLY": (-4.65, 53.30), "ANGNE": (-4.25, 53.42), "ORME": (-3.85, 53.33), "POINTAYR": (-3.32, 53.35),
    "MERSEY": (-2.95, 53.37), "CY1": (-2.60, 53.35),
    # --- the north
    "FORMBY": (-3.10, 53.55), "RIBBLE": (-2.85, 53.73), "FLEET": (-3.03, 53.92), "MORECAMBE": (-2.85, 54.12),
    "YD1": (-2.20, 54.25), "YD2": (-1.60, 54.45), "TEES": (-1.15, 54.63), "WHITBY": (-0.62, 54.50),
    "SCAR": (-0.40, 54.28), "FLAMB": (-0.08, 54.12), "SPURN": (0.15, 53.65),
    "WALNEY": (-3.25, 54.05), "RAVEN": (-3.40, 54.33), "STBEES": (-3.63, 54.50), "DB1": (-2.80, 54.70),
    "DB2": (-2.10, 54.85), "TYNE": (-1.42, 55.00),
    "SILLOTH": (-3.40, 54.87), "SOLWAY": (-3.10, 54.97), "BL1": (-2.60, 55.30), "BL2": (-2.20, 55.55),
    "BERWICK": (-2.00, 55.77), "BAMB": (-1.70, 55.62), "COQUET": (-1.55, 55.33),
    # --- Scotland
    "KIRKC": (-4.05, 54.78), "WIGTOWN": (-4.35, 54.72), "MULLG": (-4.87, 54.63), "CORSE": (-5.15, 55.00),
    "GIRVAN": (-4.87, 55.22), "AYR": (-4.65, 55.45), "ARDROSS": (-4.85, 55.65), "CLYDE": (-4.75, 55.95),
    "LG1": (-4.20, 56.05), "FORTH": (-3.75, 56.07), "LEITH": (-3.20, 55.98), "NBERW": (-2.75, 56.06),
    "DUNBAR": (-2.50, 56.00), "STABBS": (-2.13, 55.92),
    "COWAL": (-5.05, 55.90), "TARBERT": (-5.35, 55.85), "KINTYRE_E": (-5.45, 55.50), "MULLK": (-5.75, 55.30),
    "KINTYRE_W": (-5.75, 55.70), "CRINAN": (-5.65, 56.10), "OBAN": (-5.50, 56.40), "LINNHE": (-5.25, 56.62),
    "ARDNA": (-6.22, 56.73), "MALLAIG": (-5.85, 57.00), "KYLE": (-5.70, 57.28), "APPLE": (-5.85, 57.45),
    "GAIR": (-5.80, 57.75), "ULLA": (-5.25, 57.90), "ASSYNT": (-5.40, 58.25), "WRATH": (-5.00, 58.62),
    "STRATHY": (-4.00, 58.57), "DUNNET": (-3.38, 58.67), "DUNCAN": (-3.03, 58.63), "WICK": (-3.08, 58.42),
    "HELMS": (-3.65, 58.12), "DORNF": (-4.10, 57.86), "BLACKI": (-3.95, 57.68), "INVER": (-4.25, 57.50),
    "NAIRN": (-3.85, 57.60), "BURGH": (-3.45, 57.72), "BANFF": (-2.90, 57.68), "FRASER": (-2.00, 57.70),
    "PETER": (-1.78, 57.50), "ABER": (-2.05, 57.15), "MONTROSE": (-2.45, 56.72), "ARBR": (-2.55, 56.56),
    "TAYIN": (-3.05, 56.43), "FIFENESS": (-2.58, 56.28), "KIRK": (-3.15, 56.10),
    # --- Ireland
    "MALIN": (-7.37, 55.38), "FAIR": (-6.15, 55.22), "LARNE": (-5.75, 54.85), "ARDS": (-5.45, 54.45),
    "DUNDALK": (-6.30, 54.00), "DUBLIN": (-6.10, 53.35), "WICKLOW": (-6.00, 52.97), "CARNSORE": (-6.35, 52.17),
    "WATERF": (-7.10, 52.13), "CORK": (-8.30, 51.80), "MIZEN": (-9.80, 51.45), "DINGLE": (-10.40, 52.12),
    "KERRYHD": (-9.90, 52.42), "CLARE": (-9.45, 52.95), "GALWAY": (-8.95, 53.20), "SLYNE": (-10.15, 53.42),
    "ACHILL": (-10.10, 53.97), "ERRIS": (-9.95, 54.30), "KILLALA": (-9.20, 54.28), "SLIGO": (-8.50, 54.30),
    "DONBAY": (-8.20, 54.62), "GLENCOL": (-8.75, 54.70), "BLOODY": (-8.30, 55.15),
}

# id, county, settlement, modern name, holder, de jure, opinion, supply, ring (clockwise, north up)
COUNTIES = [
    (1, "Cornwall", "Kernow", "Tintagel", "Cornwall", "Cornwall", 15, 22,
     "LE STIVES TREVOSE BUDE HARTLAND BARN ILFRA EXMOOR CH1 LYME EXE TORBAY START PLYM FOWEY LIZ"),
    (2, "Hampshire", "Wintanceaster", "Winchester", "Wessex", "Wessex", 65, 45,
     "EXMOOR BRIDGW WESTON AVON SEVERN HB1 JHWB PORTS SOLENT SWANAGE PORTLAND LYME CH1"),
    (3, "Wight", "Hamwic", "Southampton", "Wessex", "Wessex", 40, 25,
     "JHWB JWBM TH1 NOTCH WHIT NFORE DOVER DUNGE BEACHY SELSEY PORTS"),
    (4, "Berkshire", "Readingas", "Reading", "Mercia", "Wessex", -18, 30,
     "SEVERN BC1 JBWC BW1 X4 JBMN JWBM JHWB HB1"),
    (5, "Middlesex", "Lundenburh", "London", "East Anglia", "Wessex", -25, 40,
     "JWBM JBMN MN1 ORWELL NAZE BLACKW SHOE NOTCH TH1"),
    (6, "Norfolk", "Theodford", "Thetford", "East Anglia", "East Anglia", 15, 35,
     "JBMN X4 NL1 WASH LYNN HUNST CROMER LOWES ALDE ORWELL MN1"),
    (7, "Chester", "Legaceaster", "Chester", "Mercia", "Mercia", 15, 24,
     "SEVERN NEWPORT CARDIFF SWANSEA GOWER TENBY PEMB STDAV CARDIGAN ABERYST HARLECH LLYN CAERN HOLY ANGNE "
     "ORME POINTAYR MERSEY CY1 JCWY CW1 JBWC BC1"),
    (8, "Warwick", "Tamworthig", "Tamworth", "Mercia", "Mercia", 15, 28,
     "JBWC CW1 JCWY WY1 JWYL WL1 X4 BW1"),
    (9, "Lincoln", "Lindcylene", "Lincoln", "Northumbria", "Mercia", 15, 28,
     "X4 WL1 JWYL LY1 HUMBER GRIMSBY MABLE GIB WASH NL1"),
    (10, "Yorkshire", "Jorvik", "York", "Northumbria", "Northumbria", 15, 38,
     "MERSEY FORMBY RIBBLE FLEET MORECAMBE YD1 YD2 TEES WHITBY SCAR FLAMB SPURN HUMBER LY1 JWYL WY1 JCWY CY1"),
    (11, "Durham", "Dunholm", "Durham", "Northumbria", "Northumbria", 15, 28,
     "MORECAMBE WALNEY RAVEN STBEES DB1 DB2 TYNE TEES YD2 YD1"),
    (12, "Bamburgh", "Bebbanburg", "Bamburgh", "Northumbria", "Northumbria", 15, 18,
     "STBEES SILLOTH SOLWAY BL1 BL2 BERWICK BAMB COQUET TYNE DB2 DB1"),
    (13, "Lothian", "Dun Eideann", "Edinburgh", "Northumbria", "Alba", 15, 24,
     "SOLWAY KIRKC WIGTOWN MULLG CORSE GIRVAN AYR ARDROSS CLYDE LG1 FORTH LEITH NBERW DUNBAR STABBS BERWICK BL2 BL1"),
    (14, "Gowrie", "Sgain", "Scone", "Alba", "Alba", 15, 20,
     "CLYDE COWAL TARBERT KINTYRE_E MULLK KINTYRE_W CRINAN OBAN LINNHE ARDNA MALLAIG KYLE APPLE GAIR ULLA ASSYNT "
     "WRATH STRATHY DUNNET DUNCAN WICK HELMS DORNF BLACKI INVER NAIRN BURGH BANFF FRASER PETER ABER MONTROSE ARBR "
     "TAYIN FIFENESS KIRK FORTH LG1"),
    (15, "Meath", "Dublin", "Dublin", "Ireland", "Ireland", 15, 28,
     "MALIN FAIR LARNE ARDS DUNDALK DUBLIN WICKLOW CARNSORE WATERF CORK MIZEN DINGLE KERRYHD CLARE GALWAY SLYNE "
     "ACHILL ERRIS KILLALA SLIGO DONBAY GLENCOL BLOODY"),
]

def build():
    out, edges = [], {}
    for cid, county, settlement, modern, holder, dejure, opinion, supply, ring in COUNTIES:
        names = ring.split()
        assert len(set(names)) == len(names), f"{county}: repeated point"
        pts = [project(*P[n]) for n in names]
        area = sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
                   for i in range(len(pts)))
        assert area > 0, f"{county}: ring must be clockwise with north up"
        for i in range(len(pts)):
            a, b = pts[i], pts[(i + 1) % len(pts)]
            assert a != b, f"{county}: {names[i]} collapses onto the next point"
            assert (a, b) not in edges, f"{county}: edge {names[i]} already used by {edges[(a, b)]}"
            edges[(a, b)] = county
        out.append(dict(id=cid, countyName=county, settlementName=settlement, modernName=modern, kingdom=holder,
                        deJureKingdom=dejure, opinion=opinion, supplyLimit=supply, fortTier=0, points=pts))
    # A vertex resting on the middle of someone else's edge would leave a crack in the map.
    verts = {p for c in out for p in c["points"]}
    for (a, b), owner in edges.items():
        for v in verts:
            if v in (a, b):
                continue
            cross = (b[0] - a[0]) * (v[1] - a[1]) - (b[1] - a[1]) * (v[0] - a[0])
            dot = (v[0] - a[0]) * (b[0] - a[0]) + (v[1] - a[1]) * (b[1] - a[1])
            if cross == 0 and 0 < dot < (b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2:
                raise AssertionError(f"{owner}: point {v} sits on edge {a}-{b}")
    shared = sum(1 for (a, b) in edges if (b, a) in edges) // 2
    print(f"{len(out)} counties, {len(verts)} points, {shared} shared borders")
    return out

def county_json(c):
    pts = ", ".join(f"[{x}, {y}]" for x, y in c["points"])
    rows = [f'      "id": {c["id"]}', f'      "countyName": "{c["countyName"]}"',
            f'      "settlementName": "{c["settlementName"]}"', f'      "modernName": "{c["modernName"]}"',
            f'      "kingdom": "{c["kingdom"]}"', f'      "deJureKingdom": "{c["deJureKingdom"]}"',
            f'      "opinion": {c["opinion"]}', f'      "supplyLimit": {c["supplyLimit"]}',
            f'      "fortTier": {c["fortTier"]}', f'      "points": [{pts}]']
    return "    {\n" + ",\n".join(rows) + "\n    }"

def write_json(counties):
    path = ROOT / "assets/data/world_map.json"
    text = path.read_text()
    head = text[:text.index('"counties"')]
    path.write_text(head + '"counties": [\n' + ",\n".join(county_json(c) for c in counties) + "\n  ]\n}\n")

def write_header(counties):
    path = ROOT / "include/world/WorldMapRepository.h"
    text = path.read_bytes().decode()
    nl = "\r\n" if "\r\n" in text else "\n"     # keep the file's own line endings
    rows = []
    for c in counties:
        pts = ", ".join(f"{{{x}.f, {y}.f}}" for x, y in c["points"])
        rows.append(f'        addDef({c["id"]}, "{c["countyName"]}", "{c["settlementName"]}", "{c["modernName"]}", '
                    f'"{c["kingdom"]}", "{c["deJureKingdom"]}", {c["opinion"]}, {c["supplyLimit"]}, {c["fortTier"]},{nl}'
                    f'               {{ {pts} }});')
    begin, end = "        // <generated: tools/gen_world_map.py>" + nl, "        // </generated>" + nl
    if begin in text:
        first, last = text.index(begin), text.index(end) + len(end)
    else:
        first = text.index("        addDef(1,")
        last = text.index("    }" + nl, text.rindex("        addDef("))
    text = text[:first] + begin + nl.join(rows) + nl + end + text[last:]
    path.write_bytes(text.encode())

def preview(counties, dest):
    from PIL import Image, ImageDraw
    s = 2
    img = Image.new("RGB", (760 * s, 700 * s), (60, 90, 110))
    d = ImageDraw.Draw(img)
    kingdoms = {k["id"]: tuple(k["color"]) for k in json.loads((ROOT / "assets/data/world_map.json").read_text())["kingdoms"]}
    for c in counties:
        pts = [(x * s, y * s) for x, y in c["points"]]
        d.polygon(pts, fill=kingdoms.get(c["kingdom"], (150, 140, 125)), outline=(20, 15, 10))
        cx = sum(p[0] for p in pts) / len(pts); cy = sum(p[1] for p in pts) / len(pts)
        d.text((cx - 20, cy), c["countyName"], fill=(0, 0, 0))
    img.save(dest)

if __name__ == "__main__":
    data = build()
    write_json(data)
    write_header(data)
    if "--preview" in sys.argv:
        preview(data, sys.argv[sys.argv.index("--preview") + 1])
