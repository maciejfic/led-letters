# HTTP API

The phone app talks to the controller over a small JSON API. It is the same on the home network
(`http://led-letters.local`) and on the setup hotspot (`http://192.168.4.1`). There is no
authentication: anyone on the same network can control the sign.

## Read

| Request | Returns |
|---|---|
| `GET /api/state` | Current state (below). |
| `GET /api/modes` | Effects: `[{"n":"Rainbow","t":0}, ...]`; `t:1` marks the test mode. |
| `GET /api/glyphs` | Glyph catalog: `[{"k":"A","l":"A","n":22}, ...]` (name, label, default LEDs). |
| `GET /api/layout` | LED positions for the preview: `{"aspect":4.27,"x":[...],"y":[...],"s":[...],"p":[...]}` (x, y 0–255, glyph index per LED, heartbeat flag per glyph). |
| `GET /api/wifi/scan` | Networks nearby: `[{"s":"Home","r":-58,"l":1}]` (SSID, RSSI, secured). Takes a few seconds. |
| `GET /api/wifi/status` | Progress of a join started from the hotspot: `{"join":"trying|ok|fail|idle","ssid":"Home","ip":"192.168.1.181","host":"led-letters"}`. |

### State

```json
{
  "on": true, "mode": 1, "bri": 140, "speed": 50, "c1": "#ff7828", "c2": "#ff003c",
  "auto": 0, "maxma": 500, "safeMa": 500,
  "sign": "Heart:27,M:30,Amp:27,K:22", "setup": true, "sym": ["♥","M","&","K"], "lm": [2,7,3,2],
  "mixed": 8, "num": 106, "maxG": 6, "warnG": 4, "minL": 2, "maxL": 60,
  "ap": false, "ssid": "Home", "ip": "192.168.1.181", "host": "led-letters", "apSsid": "LED-Letters-Setup",
  "rssi": -61, "fw": "2.0.0", "up": 7380, "heap": 150000
}
```

`speed` 50 is normal speed. `auto` is seconds between automatic effect changes (0 = off). `lm` holds
the effect of each glyph in Mixed mode.

## Change

| Request | Effect |
|---|---|
| `POST /api/set?<key>=<value>&...` | Any of `on` (0/1), `mode`, `bri` (1–255), `speed` (1–255), `c1`, `c2` (hex colour), `auto` (s), `maxma` (100–10000 mA), `lm0`…`lm5` (effect per glyph; switches to Mixed). Returns the new state. |
| `POST /api/sign` with `s=<list>` | Saves the glyph list and restarts. `400` with `{"ok":false,"err":"..."}` for an unknown glyph, an LED count outside 2–60 or more than 6 glyphs. |
| `POST /api/wifi` with `ssid`, `pass` | Saves the network. From the hotspot the controller tries it right away (`"restart":false`; follow `/api/wifi/status`); otherwise it restarts and joins (`"restart":true`). |
| `POST /api/wifi/forget` | Forgets the network and restarts into the hotspot. |
| `POST /api/restart` | Restarts the controller. |

Example:

```bash
curl -X POST "http://led-letters.local/api/set?mode=7&bri=180"
```
