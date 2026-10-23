#pragma once

// The web app, served at /. Self-contained (no web fonts or CDNs): it must work from the setup
// hotspot, where the phone has no internet.
static const char INDEX_HTML[] = R"rawliteral(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark light">
<title>LED Letters</title>
<style>
:root{
  --ground:#000;--surface:#111;--raised:#1c1c1c;--active:#262626;
  --border:#232323;--rule:#1a1a1a;--control:#444;
  --text:#fff;--muted:#b4b4b4;--faint:#949494;
  --primary:#fff;--on-primary:#000;
  --notice-bg:#2a2210;--notice-ink:#f2c66d;--notice-line:#4a3a16;
  --neg:#e5566f;--neg-bg:#2c1a20;--ok-bg:#172a1d;--ok-ink:#3fc07a;
  --stage:#050505;--led-off:#1d1d1d;
  --r-card:14px;--r-ctl:10px;
  --font:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
}
@media (prefers-color-scheme:light){:root{
  --ground:#f6f6f5;--surface:#fff;--raised:#f4f4f3;--active:#ececeb;
  --border:#e6e6e4;--rule:#efefee;--control:#cfd1d4;
  --text:#111;--muted:#5f6368;--faint:#6b6b6b;
  --primary:#1d1f20;--on-primary:#fff;
  --notice-bg:#fffbeb;--notice-ink:#92400e;--notice-line:#fde68a;
  --neg:#c4242a;--neg-bg:#ffedea;--ok-bg:#dcfae5;--ok-ink:#04250e;
  --stage:#0b0b0b;--led-off:#222;
}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--ground);color:var(--text);font:14px/20px var(--font);
  padding-bottom:calc(72px + env(safe-area-inset-bottom))}
main{max-width:560px;margin:0 auto;padding:16px 16px 24px}
h1{font-size:24px;line-height:30px;font-weight:600;margin:0;letter-spacing:-.01em}
h2{font-size:16px;line-height:24px;font-weight:600;margin:24px 0 10px}
p{margin:0}
.meta{color:var(--muted);font-size:13px;margin-top:2px}
.faint{color:var(--faint);font-size:12px;line-height:16px}
.num{font-variant-numeric:tabular-nums}

/* header */
.top{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:4px 0 16px}
.power{display:flex;align-items:center;gap:10px;color:var(--muted);font-weight:500;cursor:pointer}
.switch{flex:none;width:58px;height:34px;border-radius:999px;border:1px solid var(--control);
  background:var(--raised);position:relative;cursor:pointer;padding:0}
.switch::after{content:"";position:absolute;top:3px;left:3px;width:26px;height:26px;border-radius:50%;
  background:var(--faint);transition:transform .18s,background .18s}
.switch[aria-checked=true]{background:var(--primary);border-color:var(--primary)}
.switch[aria-checked=true]::after{transform:translateX(24px);background:var(--on-primary)}

/* the sign preview */
.stage{background:var(--stage);border-radius:var(--r-card);padding:18px 14px;margin-bottom:12px}
.stage svg{display:block;width:100%;height:auto}
.stage .cap{display:flex;justify-content:space-between;color:#8d8d8d;font-size:12px;margin-top:12px}

/* panels and rows */
.panel{background:var(--surface);border:1px solid var(--border);border-radius:var(--r-card);padding:4px 16px}
.row{display:flex;align-items:center;gap:12px;min-height:52px;border-top:1px solid var(--rule)}
.row:first-child{border-top:0}
.row>.lab{flex:1;min-width:0}
.row .val{color:var(--muted);min-width:44px;text-align:right}
input[type=range]{width:100%;accent-color:var(--text);margin:0;height:28px}
.slider{padding:10px 0 12px;border-top:1px solid var(--rule)}
.slider:first-child{border-top:0}
.slider .head{display:flex;justify-content:space-between;margin-bottom:2px}

/* effect picker */
.modes{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.mode{height:48px;border-radius:var(--r-ctl);border:1px solid var(--border);background:var(--surface);
  color:var(--text);font:inherit;text-align:left;padding:0 14px;cursor:pointer}
.mode:last-child:nth-child(odd){grid-column:1/-1}
.mode[aria-pressed=true]{background:var(--primary);border-color:var(--primary);color:var(--on-primary);font-weight:600}

/* controls */
button{font:inherit;color:inherit}
.btn{height:44px;padding:0 16px;border-radius:var(--r-ctl);border:1px solid var(--control);background:var(--surface);
  color:var(--text);cursor:pointer;font-weight:500}
.btn.primary{background:var(--primary);border-color:var(--primary);color:var(--on-primary);font-weight:600}
.btn.text{border-color:transparent;background:none;padding:0 8px;color:var(--muted)}
.btn.danger{color:var(--neg);border-color:var(--neg)}
.btn:disabled{opacity:.45;cursor:default}
.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:16px}
.actions .btn.primary{flex:1}
select,input[type=text],input[type=password],input[type=number]{height:44px;border-radius:var(--r-ctl);
  border:1px solid var(--control);background:var(--surface);color:var(--text);font:inherit;padding:0 12px;width:100%}
select{width:auto;max-width:60%}
label.field{display:block;font-weight:500;margin:16px 0 8px}
.swatch{width:44px;height:32px;border:1px solid var(--control);border-radius:8px;padding:2px;background:none;cursor:pointer}

/* glyph setup */
.glyph{display:flex;align-items:center;gap:10px;min-height:56px;border-top:1px solid var(--rule)}
.glyph:first-child{border-top:0}
.glyph .idx{width:18px;color:var(--faint)}
.glyph .ch{width:34px;font-size:22px;font-weight:600;text-align:center}
.glyph .lab{flex:1;color:var(--muted)}
.step{display:flex;align-items:center;border:1px solid var(--control);border-radius:var(--r-ctl);height:36px}
.step button{width:34px;height:34px;border:0;background:none;cursor:pointer;font-size:18px;color:var(--muted)}
.step input{width:40px;height:34px;border:0;padding:0;text-align:center;background:none;-moz-appearance:textfield}
.step input::-webkit-inner-spin-button{-webkit-appearance:none}
.icon-btn{width:36px;height:36px;border:0;border-radius:8px;background:none;color:var(--muted);cursor:pointer}
.icon-btn:active{background:var(--active)}
.keys{display:grid;grid-template-columns:repeat(8,1fr);gap:6px;margin-top:10px}
.keys button{height:44px;border-radius:8px;border:1px solid var(--border);background:var(--surface);
  font-size:16px;font-weight:600;cursor:pointer}
.keys button:disabled{opacity:.35;cursor:default}
.empty{border:1px dashed var(--control);border-radius:10px;padding:14px;color:var(--muted);margin:12px 0}
.notice{border-radius:10px;padding:12px 14px;margin-top:12px;font-size:13px;line-height:18px}
.notice.warn{background:var(--notice-bg);color:var(--notice-ink);border:1px solid var(--notice-line)}
.notice.err{background:var(--neg-bg);color:var(--neg)}
.notice.ok{background:var(--ok-bg);color:var(--ok-ink)}
.notice.info{background:var(--raised);color:var(--muted)}
.notice a{color:inherit;font-weight:600}
a{color:var(--text);text-underline-offset:3px}
.big{display:block;font-size:18px;line-height:26px;font-weight:600;margin:2px 0 8px;overflow-wrap:anywhere}
ol.steps{margin:0;padding:6px 0 6px 20px}
ol.steps li{padding:8px 0 8px 4px;border-top:1px solid var(--rule);color:var(--muted)}
ol.steps li:first-child{border-top:0}
ol.steps b{color:var(--text);font-weight:600}
.reach{padding:12px 0}
.reach+.reach{border-top:1px solid var(--rule)}
.bar{height:6px;border-radius:3px;background:var(--active);overflow:hidden;margin:8px 0 6px}
.bar i{display:block;height:100%;background:var(--text)}
.bar.over i{background:var(--neg)}
.sum{display:flex;justify-content:space-between;color:var(--muted);font-size:13px;margin-top:14px}

/* lists */
.net{display:flex;justify-content:space-between;align-items:center;min-height:48px;border-top:1px solid var(--rule);cursor:pointer}
.net:first-child{border-top:0}
dl{margin:0}
dl div{display:flex;justify-content:space-between;min-height:44px;align-items:center;border-top:1px solid var(--rule)}
dl div:first-child{border-top:0}
dt{color:var(--muted)}dd{margin:0}

/* bottom navigation */
nav.tabs{position:fixed;left:0;right:0;bottom:0;background:var(--surface);border-top:1px solid var(--border);
  display:flex;justify-content:center;padding-bottom:env(safe-area-inset-bottom);z-index:5}
nav.tabs a{flex:1;max-width:140px;display:flex;flex-direction:column;align-items:center;gap:3px;padding:9px 0 8px;
  color:var(--faint);text-decoration:none;font-size:12px}
nav.tabs a[aria-current=page]{color:var(--text);font-weight:600}
nav.tabs svg{width:20px;height:20px;fill:none;stroke:currentColor;stroke-width:1.6;stroke-linecap:round;stroke-linejoin:round}
.view[hidden],[hidden]{display:none!important}
</style></head><body>
<main>
  <header class="top">
    <div><h1 id="title">LED letters</h1><p class="meta" id="status">Connecting…</p></div>
    <label class="power" id="powerWrap"><span id="powerLab">Off</span>
      <button class="switch" id="power" role="switch" aria-checked="false" aria-label="Lights on"></button></label>
  </header>

  <!-- Light -->
  <section class="view" id="v-light">
    <div class="stage">
      <svg id="sign" role="img" aria-label="Preview of the sign"></svg>
      <div class="cap"><span id="capMode">–</span><span class="num" id="capLeds"></span></div>
    </div>
    <div class="panel">
      <div class="slider"><div class="head"><span>Brightness</span><span class="val num" id="briV"></span></div>
        <input type="range" id="bri" min="1" max="255" aria-label="Brightness"></div>
      <div class="slider"><div class="head"><span>Speed</span><span class="val num" id="speedV"></span></div>
        <input type="range" id="speed" min="5" max="200" aria-label="Speed"></div>
    </div>

    <h2>Effect</h2>
    <div class="modes" id="modes"></div>

    <h2>Colours</h2>
    <div class="panel">
      <div class="row"><div class="lab">Main colour<div class="faint">Solid, Gradient wave, Heartbeat</div></div>
        <input type="color" class="swatch" id="c1" aria-label="Main colour"></div>
      <div class="row"><div class="lab">Second colour<div class="faint">Gradient wave, the beating glyphs in Heartbeat</div></div>
        <input type="color" class="swatch" id="c2" aria-label="Second colour"></div>
    </div>

    <div id="mixWrap">
      <h2>Mixed: an effect per glyph</h2>
      <div class="panel" id="mix"></div>
    </div>

    <h2>Change effect automatically</h2>
    <div class="panel"><div class="row"><div class="lab">Every</div>
      <select id="auto"><option value="0">Off</option><option value="15">15 seconds</option><option value="30">30 seconds</option>
        <option value="60">minute</option><option value="300">5 minutes</option><option value="900">15 minutes</option></select></div></div>
  </section>

  <!-- Glyphs -->
  <section class="view" id="v-glyphs" hidden>
    <p class="meta" id="firstRun" hidden>Last step: add the glyphs on your chain, then save.</p>
    <h2 style="margin-top:8px">Your glyphs, in chain order</h2>
    <p class="faint">1 is the glyph the controller's data wire goes into. Each glyph gets its standard LED count; change it if yours is soldered differently.</p>
    <div class="empty" id="gEmpty" hidden>No glyphs yet. Tap them below in the order they are chained.</div>
    <div class="panel" style="margin-top:12px" id="glyphPanel"><div id="glyphList"></div></div>
    <div class="sum"><span class="num" id="gCount"></span><span class="num" id="gLeds"></span></div>
    <div class="notice warn" id="gWarn" hidden></div>
    <div class="notice err" id="gErr" hidden></div>

    <h2>Add a glyph</h2>
    <div class="keys" id="keys"></div>

    <div class="actions">
      <button class="btn primary" id="gSave">Save and restart</button>
      <button class="btn" id="gUndo">Undo</button>
    </div>
    <div class="actions" style="margin-top:8px">
      <button class="btn text" id="gTest">Check glyphs on the sign</button>
    </div>
    <p class="faint">Check lights each glyph in its own colour. If a colour runs into the next glyph, or LEDs at the end stay dark, correct the LED counts.</p>
  </section>

  <!-- Wi-Fi -->
  <section class="view" id="v-wifi" hidden>
    <p class="meta" style="margin-top:8px">Wi-Fi is optional. The sign works and keeps all its settings without it.</p>

    <h2>Open this page</h2>
    <div class="panel" id="reach"></div>
    <p class="faint" style="margin-top:8px" id="tip">Tip: bookmark the address or add it to your home screen.</p>

    <h2>Setup hotspot</h2>
    <div class="panel"><ol class="steps">
      <li>When the sign is switched on and is not connected to a Wi-Fi network, it opens the hotspot
        <b class="apn">LED-Letters-Setup</b> for 10 minutes. It stays open while a phone is connected to it.</li>
      <li>Join it on your phone. This page opens by itself, or go to <a href="http://192.168.4.1">http://192.168.4.1</a>.</li>
      <li>Missed the 10 minutes? Unplug the sign and plug it back in.</li>
      <li>Everything you change over the hotspot is saved on the sign and kept when it is switched off.</li>
    </ol></div>

    <h2>Join your Wi-Fi <span class="faint">optional</span></h2>
    <p class="faint">Lets you change the sign any time from any phone or computer on your home network, without joining the hotspot. While the sign is on your Wi-Fi, the hotspot stays off.</p>
    <div class="panel" id="nets" style="margin-top:12px"><div class="row"><span class="lab faint">Scan to see networks nearby.</span>
      <button class="btn" id="scan">Scan</button></div></div>
    <label class="field" for="ssid">Network name</label>
    <input type="text" id="ssid" autocapitalize="none" autocomplete="off" spellcheck="false">
    <label class="field" for="pass">Password</label>
    <input type="password" id="pass" autocomplete="off">
    <div class="actions"><button class="btn primary" id="wsave">Save and connect</button></div>
    <div id="joinMsg" hidden></div>

    <h2>Forget network</h2>
    <p class="faint">The sign forgets the saved network and restarts into its setup hotspot <b class="apn">LED-Letters-Setup</b>.</p>
    <div class="actions"><button class="btn danger" id="wforget">Forget network</button></div>
  </section>

  <!-- System -->
  <section class="view" id="v-system" hidden>
    <h2 style="margin-top:8px">Power limit</h2>
    <div class="panel"><div class="row"><div class="lab">Maximum current<div class="faint">500 mA is the safe default. Lower it for a weak USB port.</div></div>
      <input type="number" id="maxma" min="100" max="10000" step="50" style="width:96px" aria-label="Maximum current in mA">
      <span class="faint">mA</span></div></div>
    <div class="notice err" id="pWarn" hidden>Above 500 mA you risk damaging the controller, the wiring and the power supply. Only raise it if all of them are rated for the higher current.</div>
    <div class="bar" id="pBar"><i></i></div>
    <p class="faint num" id="pText"></p>

    <h2>Device</h2>
    <div class="panel"><dl>
      <div><dt>Firmware</dt><dd class="num" id="dFw">–</dd></div>
      <div><dt>LEDs</dt><dd class="num" id="dLeds">–</dd></div>
      <div><dt>Running for</dt><dd class="num" id="dUp">–</dd></div>
    </dl></div>
    <div class="actions"><button class="btn" id="restart">Restart</button></div>
  </section>
</main>

<nav class="tabs" aria-label="Sections">
  <a href="#light" data-v="light"><svg viewBox="0 0 24 24"><path d="M9 18h6M10 21h4M12 3a6 6 0 0 0-3.5 10.9c.6.5 1 1.2 1 2.1h5c0-.9.4-1.6 1-2.1A6 6 0 0 0 12 3z"/></svg>Light</a>
  <a href="#glyphs" data-v="glyphs"><svg viewBox="0 0 24 24"><path d="M4 19 9 5l5 14M5.8 14h6.4M16 19V9M16 9h2.5a2.5 2.5 0 0 1 0 5H16"/></svg>Glyphs</a>
  <a href="#wifi" data-v="wifi"><svg viewBox="0 0 24 24"><path d="M2.5 9a14 14 0 0 1 19 0M5.5 12.5a9.5 9.5 0 0 1 13 0M8.5 16a5 5 0 0 1 7 0"/><circle cx="12" cy="19" r=".8"/></svg>Wi-Fi</a>
  <a href="#system" data-v="system"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3M5.3 5.3l2.1 2.1M16.6 16.6l2.1 2.1M5.3 18.7l2.1-2.1M16.6 7.4l2.1-2.1"/></svg>System</a>
</nav>

<script>
const $=id=>document.getElementById(id);
const el=(tag,cls,text)=>{const e=document.createElement(tag);if(cls)e.className=cls;if(text!=null)e.textContent=text;return e};
let S=null,M=[],G=[],L=null,sign=[],mixKey='';
const post=(u,b)=>fetch(u,{method:'POST',body:b}).then(r=>r.json());
const getj=u=>fetch(u).then(r=>r.json());
function set(o){return post('/api/set?'+new URLSearchParams(o)).then(s=>{S=s;render()})}

// throttled live updates for sliders and colour pickers
const q={};
function live(k,v){
  if(q[k]){q[k].v=v;return}
  q[k]={v:null};set({[k]:v});
  setTimeout(()=>{const p=q[k];delete q[k];if(p.v!==null)live(k,p.v)},120);
}
const amps=leds=>leds*0.06;           // WS2812B at full white: about 60 mA per LED
const fmtA=a=>a.toFixed(1)+' A';
function fmtUp(s){const d=s/86400|0,h=s/3600%24|0,m=s/60%60|0;return d?d+' d '+h+' h':h?h+' h '+m+' min':m+' min'}

// ---------- views ----------
function show(v){
  if(!['light','glyphs','wifi','system'].includes(v))v='light';
  document.querySelectorAll('.view').forEach(s=>s.hidden=s.id!=='v-'+v);
  $('powerWrap').hidden=v!=='light';   // the power switch belongs to the Light page only
  document.querySelectorAll('nav.tabs a').forEach(a=>a.dataset.v===v?a.setAttribute('aria-current','page'):a.removeAttribute('aria-current'));
  scrollTo(0,0);
}
addEventListener('hashchange',()=>show(location.hash.slice(1)));

// ---------- render ----------
function render(){
  if(!S)return;
  const name=S.setup?S.sym.join(''):'LED letters';
  document.title=$('title').textContent=name;
  const mode=M[S.mode]?M[S.mode].n:'';
  $('status').textContent=(S.on?mode:'Off')+' · '+(S.ap?'setup hotspot':'Wi-Fi '+S.ssid);
  $('power').setAttribute('aria-checked',S.on);
  $('powerLab').textContent=S.on?'On':'Off';
  for(const k of ['bri','speed','c1','c2','auto','maxma'])
    if(document.activeElement!==$(k)&&!q[k])$(k).value=S[k];
  $('briV').textContent=Math.round(S.bri/2.55)+'%';
  $('speedV').textContent=(S.speed/50).toFixed(1)+'×';
  document.querySelectorAll('.mode').forEach(b=>b.setAttribute('aria-pressed',+b.dataset.m===S.mode));
  $('capMode').textContent=S.on?mode:'Off';
  $('capLeds').textContent=S.num+' LEDs';
  buildMix();
  S.lm.forEach((m,i)=>{const e=$('lm'+i);if(e&&document.activeElement!==e)e.value=m});
  // Wi-Fi
  drawReach();
  document.querySelectorAll('.apn').forEach(e=>e.textContent=S.apSsid);
  // System
  if(document.activeElement!==$('maxma'))$('pWarn').hidden=!(S.maxma>S.safeMa);
  $('dFw').textContent=S.fw;$('dLeds').textContent=S.num;$('dUp').textContent=fmtUp(S.up);
  const need=amps(S.setup?S.num:0),lim=S.maxma/1000;
  $('pBar').querySelector('i').style.width=S.setup?Math.min(100,lim/Math.max(need,.01)*100)+'%':'0';
  $('pText').textContent=S.setup?(need>lim?'Full white would need '+fmtA(need)+'. Brightness is reduced to stay at '+fmtA(lim)+'.':'Full white needs '+fmtA(need)+', within the '+fmtA(lim)+' limit.'):'Set up the glyphs to see the current they need.';
}

function buildMix(){
  const key=S.sym.join('|')+M.length;
  if(!M.length||key===mixKey)return;
  mixKey=key;
  $('mixWrap').hidden=!S.setup;
  const box=$('mix');box.innerHTML='';
  S.sym.forEach((name,i)=>{
    const row=el('div','row');
    const lab=el('div','lab');lab.append(el('span','faint num',(i+1)+'  '),el('span','',name));
    const sel=el('select');sel.id='lm'+i;sel.setAttribute('aria-label','Effect for glyph '+(i+1));
    M.forEach((x,m)=>{if(!x.t&&m!==S.mixed)sel.add(new Option(x.n,m))});
    sel.onchange=e=>set({['lm'+i]:e.target.value});
    row.append(lab,sel);box.appendChild(row);
  });
}

// ---------- sign preview: the real LED layout, running a copy of the effect ----------
let dots=[],T=0,last=0;
function buildPreview(){
  const svg=$('sign');svg.innerHTML='';dots=[];
  if(!L||!L.x.length)return;
  const W=1000,H=Math.max(60,W/L.aspect),pad=S&&S.setup?H*.04:12;
  svg.setAttribute('viewBox','0 0 '+W+' '+(H+pad*2));
  const r=S&&S.setup?Math.max(5,H*.03):Math.max(2.5,W/L.x.length*.32);
  for(let i=0;i<L.x.length;i++){
    const c=document.createElementNS('http://www.w3.org/2000/svg','circle');
    c.setAttribute('cx',pad+L.x[i]/255*(W-pad*2));c.setAttribute('cy',pad+L.y[i]/255*H);c.setAttribute('r',r);
    svg.appendChild(c);dots.push(c);
  }
}
const sin8=v=>(Math.sin(v/256*Math.PI*2)+1)*127.5;
function hsv(h,s,v){h=((h%256)+256)%256/256*6;s/=255;v/=255;const i=h|0,f=h-i,p=v*(1-s),q=v*(1-s*f),t=v*(1-s*(1-f));
  return [[v,t,p],[q,v,p],[p,v,t],[p,q,v],[t,p,v],[v,p,q]][i%6].map(x=>x*255)}
const hex=h=>[1,3,5].map(i=>parseInt(h.substr(i,2),16));
const mix=(a,b,t)=>a.map((x,i)=>x+(b[i]-x)*t);
const pulse=(x,len)=>x<0||x>=len?0:Math.pow(1-x/len,2);
function noise(x,y,z){return (Math.sin(x*.9+z*1.3)+Math.sin(y*1.7-z*2.1)+Math.sin((x+y)*.6+z*.7))/6+.5}
function heat(h){return h<.33?[h*3*255,0,0]:h<.66?[255,(h-.33)*3*255,0]:[255,255,(h-.66)*3*255]}
function colour(name,i,n){
  const x=L.x[i],y=L.y[i],s=L.s[i],ns=Math.max(1,S.sym.length);
  switch(name){
    case 'Solid':return hex(S.c1);
    case 'Rainbow':return hsv(T/16-i*256/n,255,255);
    case 'Rainbow sweep':return hsv(T/16-x,255,255);
    case 'Rainbow rise':return hsv(T/16+y,255,255);
    case 'Glyph colors':return hsv(T/32+s*256/ns,230,255);
    case 'Gradient wave':return mix(hex(S.c1),hex(S.c2),sin8(x/2+T/16)/255);
    case 'Heartbeat':{const p=T%1100,h=Math.min(1,pulse(p,350)+pulse(p-280,350)*.75);
      return L.p[s]?hex(S.c2).map(v=>v*(.16+.84*h)):hex(S.c1).map(v=>v*.35)}
    case 'Fire':{const h=noise(x/40,(y+T/6)/40,T/900)*(.27+y/255*.7);return heat(Math.min(1,h*1.6))}
    case 'Mixed':{const m=M[S.lm[s]];return colour(m&&m.n!=='Mixed'?m.n:'Rainbow',i,n)}
    default:return hsv(s*256/ns,255,255);   // the glyph test
  }
}
function frame(now){
  requestAnimationFrame(frame);
  if(!S||!L||!dots.length||document.hidden||now-last<33)return;
  T+=(now-last)*S.speed/50;last=now;
  const name=M[S.mode]?M[S.mode].n:'Rainbow',n=dots.length,b=.35+.65*S.bri/255;
  for(let i=0;i<n;i++){
    let c=S.on?colour(name,i,n).map(v=>Math.round(v*b)):null;
    dots[i].setAttribute('fill',c?'rgb('+c+')':'var(--led-off)');
  }
}
requestAnimationFrame(frame);

// ---------- glyph setup ----------
const parseSign=t=>t.split(',').filter(Boolean).map(x=>{const[k,n]=x.split(':');return{k,n:+n}});
const glyph=k=>G.find(x=>x.k===k)||{k,l:k,n:0};
function drawGlyphs(){
  const list=$('glyphList');list.innerHTML='';
  $('glyphPanel').hidden=!sign.length;
  $('gEmpty').hidden=!!sign.length;
  sign.forEach((it,i)=>{
    const row=el('div','glyph');
    const st=el('div','step');
    const minus=el('button',null,'−'),plus=el('button',null,'+'),num=el('input','num');
    minus.setAttribute('aria-label','Fewer LEDs');plus.setAttribute('aria-label','More LEDs');
    num.type='number';num.inputMode='numeric';num.min=S.minL;num.max=S.maxL;num.value=it.n;num.setAttribute('aria-label','LEDs in glyph '+(i+1));
    const setN=v=>{it.n=v;num.value=v;glyphState()};
    minus.onclick=()=>setN(Math.max(S.minL,it.n-1));plus.onclick=()=>setN(Math.min(S.maxL,it.n+1));
    num.oninput=e=>{it.n=+e.target.value;glyphState()};
    st.append(minus,num,plus);
    const btn=(t,label,f)=>{const b=el('button','icon-btn',t);b.setAttribute('aria-label',label);b.onclick=f;return b};
    const sw=(a,b)=>{if(b<0||b>=sign.length)return;[sign[a],sign[b]]=[sign[b],sign[a]];drawGlyphs()};
    row.append(el('span','idx num',i+1),el('span','ch',glyph(it.k).l),el('span','lab','LEDs'),st,
      btn('↑','Move up',()=>sw(i,i-1)),btn('×','Remove',()=>{sign.splice(i,1);drawGlyphs()}));
    list.appendChild(row);
  });
  glyphState();
}
function glyphState(){
  const n=sign.length,full=n>=S.maxG,leds=sign.reduce((a,b)=>a+(b.n||0),0);
  document.querySelectorAll('#keys button').forEach(b=>b.disabled=full);
  $('gCount').textContent=n+' of '+S.maxG+' glyphs';
  $('gLeds').textContent=leds+' LEDs · limited to '+fmtA(S.maxma/1000);
  $('gWarn').hidden=n<=S.warnG;
  $('gWarn').textContent='More than '+S.warnG+' glyphs need a very stable power supply: a good quality 5 V supply rated for the full current, ideally feeding power into both ends of the chain.';
  const bad=sign.find(x=>!(x.n>=S.minL&&x.n<=S.maxL));
  $('gErr').hidden=!full&&!bad;
  $('gErr').textContent=bad?'Each glyph needs between '+S.minL+' and '+S.maxL+' LEDs.':
    S.maxG+' glyphs is the maximum. More would overload the current the controller and power supply can carry.';
  $('gSave').disabled=!n||!!bad;
}
$('gUndo').onclick=()=>{sign=parseSign(S.sign);drawGlyphs()};
$('gTest').onclick=()=>{set({mode:M.findIndex(x=>x.t)});location.hash='light'};
$('gSave').onclick=()=>{
  post('/api/sign',new URLSearchParams({s:sign.map(x=>x.k+':'+x.n).join(',')})).then(r=>{
    if(!r.ok){$('gErr').hidden=false;$('gErr').textContent=r.err||'Could not save the glyphs.';return}
    $('gSave').disabled=true;$('gSave').textContent='Restarting…';
    setTimeout(()=>{location.hash='light';location.reload()},7000);
  });
};

// ---------- controls ----------
$('power').onclick=()=>set({on:S.on?0:1});
['bri','speed'].forEach(k=>$(k).oninput=e=>{S[k]=+e.target.value;render();live(k,e.target.value)});
['c1','c2'].forEach(k=>$(k).oninput=e=>{S[k]=e.target.value;live(k,e.target.value)});
$('auto').onchange=e=>set({auto:e.target.value});
$('maxma').oninput=e=>{$('pWarn').hidden=!(+e.target.value>S.safeMa)};
$('maxma').onchange=e=>set({maxma:Math.max(100,+e.target.value||0)});

$('scan').onclick=()=>{
  $('scan').disabled=true;$('scan').textContent='Scanning…';
  getj('/api/wifi/scan').then(l=>{
    const box=$('nets');box.innerHTML='';
    if(!l.length)box.append(el('div','row faint','No networks found.'));
    l.sort((a,b)=>b.r-a.r).forEach(n=>{
      const d=el('div','net');
      const bars=n.r>-60?'Strong':n.r>-72?'Good':'Weak';
      d.append(el('span','',n.s),el('span','faint',(n.l?'Secured · ':'Open · ')+bars));
      d.onclick=()=>{$('ssid').value=n.s;$('pass').focus()};
      box.appendChild(d);
    });
  });
};
// how to reach the page: the hotspot now, or the sign's address on the home network
function link(host){return '<a class="big" href="http://'+host+'/">http://'+host+'</a>'}
let lastJoin=null;
function drawReach(join=lastJoin){
  const box=$('reach');
  $('tip').hidden=S.ap&&!(join&&join.join==='ok');
  if(!S.ap){
    box.innerHTML='<div class="reach"><div class="faint">On any phone or computer connected to <b></b></div>'+link(S.host+'.local')+'</div>'+
      '<div class="reach"><div class="faint">If that does not open (some Android phones), use the number</div>'+link(S.ip)+'</div>';
    box.querySelector('b').textContent=S.ssid;
  }else if(join&&join.join==='ok'){
    box.innerHTML='<div class="reach"><div class="faint">On your Wi-Fi <b></b></div>'+link(join.host+'.local')+
      '<div class="faint" style="margin-top:8px">or</div>'+link(join.ip)+'</div>';
    box.querySelector('b').textContent=join.ssid;
  }else{
    box.innerHTML='<div class="reach muted">You are connected through the setup hotspot, at '+link('192.168.4.1')+
      '<div class="faint" style="margin-top:6px">After you join your Wi-Fi below, the sign\'s address on it shows here.</div></div>';
  }
}
function joinMsg(cls,html){const m=$('joinMsg');m.hidden=false;m.className='notice '+cls;m.innerHTML=html}
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function watchJoin(ssid,tries){
  getj('/api/wifi/status').then(j=>{
    if(j.join==='trying'&&tries>0)return setTimeout(()=>watchJoin(ssid,tries-1),1500);
    $('wsave').disabled=false;$('wsave').textContent='Save and connect';
    if(j.join==='ok'){
      lastJoin=j;drawReach(j);
      joinMsg('ok','<b>Connected to “'+esc(j.ssid)+'”.</b><br>Now connect your phone back to “'+esc(j.ssid)+'” and open'+
        link(j.host+'.local')+'If that does not open, use'+link(j.ip)+'The setup hotspot turns off in about 2 minutes.');
    }else{
      joinMsg('err','Could not join “'+esc(ssid)+'”. Check the network name and password, then try again.');
    }
  }).catch(()=>setTimeout(()=>watchJoin(ssid,tries-1),1500));
}
$('wsave').onclick=()=>{
  const s=$('ssid').value.trim();if(!s)return $('ssid').focus();
  $('wsave').disabled=true;$('wsave').textContent='Joining “'+s+'”…';
  joinMsg('info','Joining “'+esc(s)+'”. This takes up to 20 seconds; keep this page open.');
  post('/api/wifi',new URLSearchParams({ssid:s,pass:$('pass').value})).then(r=>{
    if(r.restart){
      joinMsg('ok','Saved. The sign restarts and joins “'+esc(s)+'”. On that network open '+link(S.host+'.local'));
    }else watchJoin(s,16);
  });
};
$('wforget').onclick=()=>{if(confirm('Forget the saved network and restart into the setup hotspot?'))post('/api/wifi/forget')};
$('restart').onclick=()=>{if(confirm('Restart the sign?'))post('/api/restart')};

// ---------- start ----------
Promise.all([getj('/api/glyphs'),getj('/api/modes'),getj('/api/state'),getj('/api/layout')]).then(([g,m,s,l])=>{
  G=g;M=m;S=s;L=l;
  g.forEach(x=>{
    const b=el('button',null,x.l);b.setAttribute('aria-label','Add '+x.k+', '+x.n+' LEDs');
    b.onclick=()=>{if(sign.length<S.maxG){sign.push({k:x.k,n:x.n});drawGlyphs()}};
    $('keys').appendChild(b);
  });
  m.forEach((x,i)=>{
    if(x.t)return;
    const b=el('button','mode',x.n);b.dataset.m=i;b.onclick=()=>set({mode:i});
    $('modes').appendChild(b);
  });
  $('firstRun').hidden=s.setup;
  sign=parseSign(s.sign);drawGlyphs();
  buildPreview();render();
  show(location.hash.slice(1)||(s.setup?'light':'glyphs'));
});
setInterval(()=>{if(!document.hidden&&S)getj('/api/state').then(s=>{S=s;render()}).catch(()=>{})},4000);
</script></body></html>)rawliteral";
