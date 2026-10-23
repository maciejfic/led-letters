"""Preview the web app without the board: serves firmware/LedLetters/web_ui.h with a fake API.

Usage: python3 tools/mock_server.py [port]   then open http://localhost:8080
State lives in memory; /api/sign "restarts" instantly. Start with ?fresh to see the first-run screen.
"""
import json, math, os, re, sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

HERE = os.path.dirname(os.path.abspath(__file__))
FW = os.path.join(HERE, '..', 'firmware', 'LedLetters')

MODES = ['Solid', 'Rainbow', 'Rainbow sweep', 'Rainbow rise', 'Glyph colors', 'Gradient wave',
         'Heartbeat', 'Fire', 'Mixed', 'Test: glyphs']
MAX_G, WARN_G, MIN_L, MAX_L, UNCONF = 6, 4, 2, 60, 150


def html():
    s = open(os.path.join(FW, 'web_ui.h'), encoding='utf-8').read()
    return s.split('R"rawliteral(', 1)[1].rsplit(')rawliteral"', 1)[0]


def glyphs():
    src = open(os.path.join(FW, 'glyph_data.h'), encoding='utf-8').read()
    pts = [tuple(map(int, m)) for m in re.findall(r'\{(-?\d+),(-?\d+)\},', src.split('GLYPH_PTS')[1].split('};')[0])]
    segs = [tuple(map(int, m)) for m in re.findall(r'\{(\d+),(\d+),(\d+)\},', src.split('GLYPH_SEGS[]')[1].split('};')[0])]
    defs = re.findall(r'\{"(\w+)", "([^"]+)", (\d+), (\d+), (\d+), (\w+), (\d+), (\d+)\}', src)
    out = []
    for name, label, leds, w, h, pulses, s0, ns in defs:
        out.append({'k': name, 'l': label, 'n': int(leds), 'w': int(w), 'pulses': pulses == 'true',
                    'segs': [(segs[i][0], pts[segs[i][2]:segs[i][2] + segs[i][1]]) for i in range(int(s0), int(s0) + int(ns))]})
    return out


GLYPHS = glyphs()
BY = {g['k']: g for g in GLYPHS}


def along(p, n):
    L = [math.dist(p[i], p[i + 1]) for i in range(len(p) - 1)] or [0]
    T = sum(L)
    out = []
    for k in range(n):
        d = T / 2 if n == 1 else T * k / (n - 1)
        i = 0
        while i < len(L) - 1 and d > L[i]:
            d -= L[i]; i += 1
        t = d / L[i] if L[i] else 0
        a, b = p[i], p[min(i + 1, len(p) - 1)]
        out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def layout(spec):
    items = [x.split(':') for x in spec.split(',') if x]
    if not items:
        n = UNCONF
        return {'aspect': 24, 'x': [i * 255 // (n - 1) for i in range(n)], 'y': [128] * n, 's': [0] * n, 'p': [0]}
    X, Y, Sy, P, ox = [], [], [], [], 0
    for si, (k, n) in enumerate(items):
        g, n = BY[k], int(n)
        total = sum(c for c, _ in g['segs'])
        given, acc = 0, 0
        for c, path in g['segs']:
            acc += c
            upto = (acc * n + total // 2) // total
            cnt, given = upto - given, upto
            for x, y in along(path, cnt) if cnt else []:
                X.append(ox + x); Y.append(y); Sy.append(si)
        P.append(1 if g['pulses'] else 0)
        ox += g['w'] + 20
    mnx, mxx, mny, mxy = min(X), max(X), min(Y), max(Y)
    return {'aspect': round((mxx - mnx) / max(mxy - mny, 1), 2),
            'x': [round((x - mnx) * 255 / (mxx - mnx)) for x in X],
            'y': [round((mxy - y) * 255 / (mxy - mny)) for y in Y], 's': Sy, 'p': P}


join = {}   # simulated join from the setup hotspot: any non-empty password succeeds after 4 s
state = {'apSsid': 'LED-Letters-Setup', 'on': True, 'mode': 1, 'bri': 140, 'speed': 50, 'c1': '#ff7828', 'c2': '#ff003c', 'auto': 0,
         'maxma': 500, 'lm': [2, 7, 3, 2, 7, 3], 'sign': 'Heart:27,M:30,Amp:27,K:22', 'ap': False,
         'ssid': 'Home', 'ip': '192.168.1.181', 'host': 'led-letters', 'rssi': -61, 'fw': '2.0.0', 'up': 7380}


def full_state():
    items = [x for x in state['sign'].split(',') if x]
    s = dict(state)
    s.update(setup=bool(items), sym=[BY[x.split(':')[0]]['l'] for x in items] or ['Strip'],
             num=sum(int(x.split(':')[1]) for x in items) if items else UNCONF,
             mixed=MODES.index('Mixed'), safeMa=500, maxG=MAX_G, warnG=WARN_G, minL=MIN_L, maxL=MAX_L, heap=150000)
    s['lm'] = s['lm'][:len(s['sym'])]
    return s


class H(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def send(self, obj, code=200, ctype='application/json'):
        body = obj.encode() if isinstance(obj, str) else json.dumps(obj, ensure_ascii=False).encode()
        self.send_response(code)
        self.send_header('Content-Type', ctype + '; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def args(self):
        u = urlparse(self.path)
        a = {k: v[0] for k, v in parse_qs(u.query, keep_blank_values=True).items()}
        n = int(self.headers.get('Content-Length') or 0)
        if n:
            a.update({k: v[0] for k, v in parse_qs(self.rfile.read(n).decode()).items()})
        return u.path, a

    def do_GET(self):
        path, a = self.args()
        if path == '/':
            if 'fresh' in a:
                state['sign'] = ''
            if 'ap' in a:   # pretend the phone is on the setup hotspot
                state.update(ap=True, ssid='', ip='192.168.4.1')
            return self.send(html(), ctype='text/html')
        if path == '/api/state':
            return self.send(full_state())
        if path == '/api/modes':
            return self.send([{'n': m, 't': 1 if m.startswith('Test') else 0} for m in MODES])
        if path == '/api/glyphs':
            return self.send([{'k': g['k'], 'l': g['l'], 'n': g['n']} for g in GLYPHS])
        if path == '/api/layout':
            return self.send(layout(state['sign']))
        if path == '/api/wifi/status':
            import time
            ok = time.time() - join.get('t', 0) > 4
            st = 'idle' if not join else ('ok' if ok and join['pass'] else 'fail' if ok else 'trying')
            return self.send({'join': st, 'ssid': join.get('ssid', ''), 'ip': '192.168.1.181' if st == 'ok' else '', 'host': 'led-letters'})
        if path == '/api/wifi/scan':
            return self.send([{'s': 'Home', 'r': -58, 'l': 1}, {'s': 'Neighbours 5G', 'r': -74, 'l': 1},
                              {'s': 'Cafe guest', 'r': -80, 'l': 0}])
        self.send('Not found', 404, 'text/plain')

    def do_POST(self):
        path, a = self.args()
        if path == '/api/set':
            for k in ('on', 'mode', 'bri', 'speed', 'auto', 'maxma'):
                if k in a:
                    state[k] = int(a[k]) if k != 'on' else a[k] != '0'
            for k in ('c1', 'c2'):
                if k in a:
                    state[k] = a[k]
            for i in range(MAX_G):
                if 'lm%d' % i in a:
                    state['lm'][i] = int(a['lm%d' % i]); state['mode'] = MODES.index('Mixed')
            return self.send(full_state())
        if path == '/api/sign':
            items = [x.split(':') for x in a.get('s', '').split(',') if x]
            if len(items) > MAX_G:
                return self.send({'ok': False, 'err': 'At most %d glyphs: more would overload the current.' % MAX_G}, 400)
            if not items or any(k not in BY or not MIN_L <= int(n) <= MAX_L for k, n in items):
                return self.send({'ok': False, 'err': 'Unknown glyph, or an LED count outside %d-%d.' % (MIN_L, MAX_L)}, 400)
            state['sign'] = a['s']
            return self.send({'ok': True, 'num': sum(int(n) for _, n in items)})
        if path == '/api/wifi':
            import time
            if not state['ap']:
                return self.send({'ok': True, 'restart': True})
            join.update(ssid=a.get('ssid', ''), pass_=None, t=time.time()); join['pass'] = a.get('pass', '')
            return self.send({'ok': True, 'restart': False})
        if path in ('/api/wifi/forget', '/api/restart'):
            return self.send({'ok': True})
        self.send('Not found', 404, 'text/plain')


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print('Mock LED letters at http://localhost:%d (add ?fresh for first run)' % port)
    ThreadingHTTPServer(('127.0.0.1', port), H).serve_forever()
