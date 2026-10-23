"""Generate firmware/LedLetters/glyph_data.h: the glyph catalog for the firmware.

Reads the glyph design files (the 3D model project): led_map.json (LED counts, sizes) and
glyphs/*.json (LED channel geometry).
Each LED channel becomes one segment: a polyline in mm (x right, y up, origin at the glyph's
bottom-left) along which its LEDs are spread. Glyphs without design data (E H L O V and the
originals M K &) use hand-made paths below.

Usage: python3 tools/gen_glyphs.py <folder with led_map.json and glyphs/>
"""
import json, math, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'firmware', 'LedLetters', 'glyph_data.h')
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', '..', 'led-letters')

# LED counts that differ from led_map.json (what is actually built)
COUNT_OVERRIDES = {'Heart': 27, 'Amp': 27}
PULSES = {'Heart', 'Amp'}          # beat in the Heartbeat mode
MAX_PTS = 12                       # points per segment polyline


def length(p):
    return sum(math.dist(p[i], p[i + 1]) for i in range(len(p) - 1))


def resample(p, n):
    """n points evenly spaced along polyline p"""
    total = length(p)
    if total == 0 or n < 2:
        return [p[0]] * max(n, 1)
    out, i, acc = [], 0, 0.0
    for k in range(n):
        d = total * k / (n - 1)
        while i < len(p) - 2 and acc + math.dist(p[i], p[i + 1]) < d:
            acc += math.dist(p[i], p[i + 1])
            i += 1
        seg = math.dist(p[i], p[i + 1]) or 1
        t = min(max((d - acc) / seg, 0), 1)
        out.append((p[i][0] + (p[i + 1][0] - p[i][0]) * t, p[i][1] + (p[i + 1][1] - p[i][1]) * t))
    return out


def centreline(a, b, closed):
    if closed:
        a, b = a + [a[0]], b + [b[0]]
    n = max(len(a), len(b), 40)
    ra, rb = resample(a, n), resample(b, n)
    return [((x1 + x2) / 2, (y1 + y2) / 2) for (x1, y1), (x2, y2) in zip(ra, rb)]


def simplify(p):
    """keep MAX_PTS points; for paths of 2 points keep them as they are"""
    if len(p) <= 2:
        return p
    return resample(p, MAX_PTS)


def from_json(d):
    segs = []
    for c in d['channels']:
        n = sum(c['leds'])
        if c['kind'] == 'S':
            path = [tuple(c['p0']), tuple(c['p1'])]
        elif 'outer' in c:
            path = centreline(c['outer'], c['inner'], c.get('closed'))
        else:
            path = centreline(c['left'], c['right'], c.get('closed'))
        segs.append((n, simplify(path)))
    return segs


def heart_from_json(d):
    """The built heart's chain starts at the bottom tip and runs up the left side."""
    (n, _), = from_json(d)
    c = d['channels'][0]
    p = centreline(c['outer'], c['inner'], True)[:-1]
    tip = min(range(len(p)), key=lambda i: p[i][1])
    p = p[tip:] + p[:tip]
    if p[1][0] > p[0][0]:                 # make it go left first
        p = [p[0]] + p[1:][::-1]
    p = p + [p[0]]
    return [(n, resample(p, MAX_PTS))]


def photo(segs, box, w, h):
    """hand-measured segments in photo pixels (y down) -> glyph mm (y up)"""
    x0, y0, x1, y1 = box
    return [(n, [((x - x0) * w / (x1 - x0), (y1 - y) * h / (y1 - y0)) for x, y in pts]) for n, pts in segs]


def ellipse(cx, cy, rx, ry, n=MAX_PTS):
    # starts at the bottom, runs clockwise as seen from the front (left side first)
    return [(cx - rx * math.sin(2 * math.pi * k / (n - 1)), cy - ry * math.cos(2 * math.pi * k / (n - 1)))
            for k in range(n)]


def split(total, parts):
    """spread `total` LEDs over paths in proportion to their length (at least 2 each)"""
    lens = [length(p) for p in parts]
    raw = [total * l / sum(lens) for l in lens]
    counts = [max(2, int(r)) for r in raw]
    while sum(counts) < total:
        i = max(range(len(parts)), key=lambda i: raw[i] - counts[i])
        counts[i] += 1
    while sum(counts) > total:
        i = max(range(len(parts)), key=lambda i: counts[i])
        counts[i] -= 1
    return list(zip(counts, parts))


# ---- glyphs without design data ----
# M, & and K: the layouts checked on the real sign (photo pixels, chain order)
M_PHOTO = [(2, [(225, 1225), (275, 1225)]), (6, [(250, 1130), (265, 800)]), (2, [(255, 720), (305, 720)]),
           (4, [(370, 815), (405, 960)]), (2, [(440, 1060), (440, 1130)]), (4, [(505, 960), (530, 830)]),
           (2, [(620, 710), (680, 710)]), (6, [(645, 830), (640, 1110)]), (2, [(600, 1215), (650, 1215)])]
AMP_PHOTO = [(6, [(1048, 1200), (976, 1240), (916, 1220), (874, 1168), (866, 1087), (892, 1047)]),
             (7, [(1072, 939), (1116, 885), (1110, 700), (948, 700), (942, 736)]),
             (7, [(978, 948), (1140, 1170)]), (3, [(1184, 1096), (1254, 1013)]),
             (2, [(1232, 939), (1272, 939)]), (2, [(1248, 1240), (1296, 1240)])]
K_PHOTO = [(2, [(1525, 1255), (1575, 1255)]), (6, [(1555, 1180), (1550, 860)]), (2, [(1520, 745), (1570, 745)]),
           (2, [(1755, 775), (1800, 775)]), (5, [(1765, 825), (1640, 1000)]), (3, [(1720, 1080), (1770, 1190)]),
           (2, [(1775, 1275), (1825, 1275)])]


def hand_made(name, total, w, h):
    if name == 'M':
        return photo(M_PHOTO, (150, 650, 750, 1330), w, h)
    if name == 'Amp':
        return photo(AMP_PHOTO, (800, 690, 1400, 1300), w, h)
    if name == 'K':
        return photo(K_PHOTO, (1450, 700, 1950, 1330), w, h)
    if name == 'L':
        return split(total, [[(34, 182), (34, 12)], [(34, 12.5), (w - 12, 12.5)]])
    if name == 'E':
        return split(total, [[(34, 182.5), (w - 12, 182.5)], [(34, 170), (34, 25)],
                             [(34, 97.5), (w - 40, 97.5)], [(34, 12.5), (w - 12, 12.5)]])
    if name == 'H':
        return split(total, [[(34, 12), (34, 182)], [(34, 97.5), (w - 34, 97.5)], [(w - 34, 182), (w - 34, 12)]])
    if name == 'V':
        return split(total, [[(34, 182), (w / 2, 14)], [(w / 2, 14), (w - 34, 182)]])
    if name == 'O':
        return [(total, ellipse(w / 2, h / 2, w / 2 - 12.5, h / 2 - 12.5))]
    raise ValueError(name)


def main():
    if not os.path.exists(os.path.join(SRC, 'led_map.json')):
        sys.exit('led_map.json not found in %s\nUsage: python3 tools/gen_glyphs.py <folder with led_map.json and glyphs/>' % SRC)
    lm = json.load(open(os.path.join(SRC, 'led_map.json'), encoding='utf-8'))['glyphs']
    glyphs = []
    for label, g in lm.items():
        name = g['name']
        total = COUNT_OVERRIDES.get(name, g['leds'])
        f = os.path.join(SRC, 'glyphs', name + '.json')
        if name == 'Heart':
            segs = heart_from_json(json.load(open(f)))
        elif os.path.exists(f) and name not in ('M', 'K', 'Amp'):
            segs = from_json(json.load(open(f)))
        else:
            segs = hand_made(name, total, g['width_mm'], g['height_mm'])
        glyphs.append((name, label, total, g['width_mm'], g['height_mm'], segs))

    pts, segs_out, defs = [], [], []
    for name, label, total, w, h, segs in glyphs:
        seg0 = len(segs_out)
        for n, path in segs:
            segs_out.append((n, len(path), len(pts)))
            pts.extend((round(x), round(y)) for x, y in path)
        defs.append((name, label, total, round(w), round(h), name in PULSES, seg0, len(segs)))

    L = ['// Generated by tools/gen_glyphs.py from the glyph design files - do not edit by hand.',
         '#pragma once', '#include <stdint.h>', '',
         'struct GlyphSeg { uint8_t leds; uint8_t numPts; uint16_t firstPt; };',
         'struct GlyphDef { const char* name; const char* label; uint8_t leds; uint16_t widthMm, heightMm;',
         '                  bool pulses; uint16_t firstSeg; uint8_t numSegs; };', '',
         '// polyline points in mm (x right, y up, origin bottom-left of the glyph)',
         'static const int16_t GLYPH_PTS[][2] = {']
    for i in range(0, len(pts), 10):
        L.append('  ' + ' '.join('{%d,%d},' % p for p in pts[i:i + 10]))
    L += ['};', '', 'static const GlyphSeg GLYPH_SEGS[] = {']
    for i in range(0, len(segs_out), 6):
        L.append('  ' + ' '.join('{%d,%d,%d},' % s for s in segs_out[i:i + 6]))
    L += ['};', '', 'static const GlyphDef GLYPHS[] = {']
    for name, label, total, w, h, pulses, s0, ns in defs:
        L.append('  {"%s", "%s", %d, %d, %d, %s, %d, %d},' % (name, label, total, w, h,
                                                              'true' if pulses else 'false', s0, ns))
    L += ['};', 'static const uint8_t GLYPH_COUNT = sizeof(GLYPHS) / sizeof(GLYPHS[0]);', '']
    open(OUT, 'w', encoding='utf-8').write('\n'.join(L))

    for name, label, total, w, h, segs in glyphs:
        s = sum(n for n, _ in segs)
        flag = '' if s == total else '  (segments sum to %d)' % s
        print('%-6s %3d LEDs %2d segments%s' % (name, total, len(segs), flag))
    print('%d glyphs, %d segments, %d points -> %s' % (len(defs), len(segs_out), len(pts), os.path.normpath(OUT)))


if __name__ == '__main__':
    main()
