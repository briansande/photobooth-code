#include <Arduino.h>
#include <PhotoboothShutter.h>
#include <PhotoboothWiFi.h>
#include <WebServer.h>

namespace {
WebServer server(80);
PhotoboothShutter shutter;
bool ready = false;
uint32_t openDurationUs = 1000000;
uint32_t closedAngle = 90;
uint32_t openAngle = 100;
uint32_t lastStatusMs = 0;
uint32_t reportedShots = 0;

// Reject signs, fractions, suffixes and overflow before changing any settings.
bool parseNumber(const String &text, uint32_t minimum, uint32_t maximum,
                 uint32_t &value) {
  if (text.isEmpty() || text.length() > 10) return false;
  uint32_t parsed = 0;
  for (unsigned int i = 0; i < text.length(); ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    const uint32_t digit = text[i] - '0';
    if (parsed > (maximum - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  if (parsed < minimum || parsed > maximum) return false;
  value = parsed;
  return true;
}

// Accept plain decimal seconds to microsecond precision without floating-point drift.
// A shot may last from 0.001 to 30 seconds.
bool parseSeconds(const String &text, uint32_t &durationUs) {
  if (text.isEmpty() || text.length() > 16) return false;
  uint32_t whole = 0, fraction = 0, scale = 100000;
  bool dot = false, digits = false, fractionDigits = false;
  for (unsigned int i = 0; i < text.length(); ++i) {
    const char c = text[i];
    if (c == '.' && !dot) { dot = true; continue; }
    if (c < '0' || c > '9') return false;
    digits = true;
    const uint32_t digit = c - '0';
    if (!dot) {
      if (whole > (30 - digit) / 10) return false;
      whole = whole * 10 + digit;
    } else {
      if (scale == 0) return false;  // No more than six decimal places.
      fraction += digit * scale;
      scale /= 10;
      fractionDigits = true;
    }
  }
  if (!digits || (dot && !fractionDigits)) return false;
  const uint32_t parsed = whole * 1000000 + fraction;
  if (parsed < 1000 || parsed > 30000000) return false;
  durationUs = parsed;
  return true;
}

String formatSeconds(uint32_t durationUs) {
  char result[16];
  snprintf(result, sizeof(result), "%lu.%06lu",
           static_cast<unsigned long>(durationUs / 1000000),
           static_cast<unsigned long>(durationUs % 1000000));
  String seconds(result);
  while (seconds.endsWith("0")) seconds.remove(seconds.length() - 1);
  if (seconds.endsWith(".")) seconds.remove(seconds.length() - 1);
  return seconds;
}

const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Shutter | Photobooth</title><style>
:root{color-scheme:dark;font-family:"Segoe UI",Arial,sans-serif;color:#ecebe7;background:#141516}
*{box-sizing:border-box}
body{margin:0;min-height:100vh;display:grid;place-items:center;padding:28px 18px}
main{width:min(100%,480px)}
header{display:flex;align-items:flex-end;justify-content:space-between;border-bottom:1px solid #3b3e3f;padding-bottom:20px}
.kicker,.section-title,.status-label,.unit,.footer-note{font-size:11px;line-height:1.4;letter-spacing:.1em;text-transform:uppercase}
.kicker{color:#989c9a;margin:0 0 11px}.tag{color:#9ca19e;border:1px solid #555a58;padding:5px 7px;margin-bottom:4px}
h1{font-size:36px;font-weight:500;letter-spacing:-.04em;line-height:1;margin:0}
.status-row{display:flex;align-items:center;justify-content:space-between;gap:16px;padding:17px 0;border-bottom:1px solid #3b3e3f}
.status-label{color:#929895}.status-value{display:flex;align-items:center;gap:9px;text-align:right;font-size:13px;color:#d2d3cf}
.status-value::before{content:"";display:block;flex:none;width:7px;height:7px;border-radius:50%;background:#7f8581}
.status-value[data-state="shooting"]::before{background:#c8a970}.status-value[data-state="open"]::before{background:#a9ba9b}
.section-title{color:#929895;margin:26px 0 18px}
label{display:block;color:#d7d9d5;font-size:13px;margin-bottom:9px}
.field{position:relative}.field input{display:block;width:100%;height:56px;border:1px solid #45494a;border-radius:3px;background:#1c1e1f;color:#f3f2ee;padding:0 46px 0 15px;font:500 21px/1 Consolas,"SFMono-Regular",monospace;font-variant-numeric:tabular-nums;outline:none}
.field input:focus{border-color:#b9bdb9;box-shadow:0 0 0 1px #b9bdb9}
.field input::-webkit-inner-spin-button,.field input::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}
.field input[type=number]{-moz-appearance:textfield;appearance:textfield}
.unit{position:absolute;right:15px;top:20px;color:#898f8c;pointer-events:none}
.angles{display:grid;grid-template-columns:1fr 1fr;gap:13px;margin-top:2px}
button{min-height:52px;border-radius:3px;font:600 13px "Segoe UI",Arial,sans-serif;letter-spacing:.02em;cursor:pointer}
button:focus-visible{outline:2px solid #ecebe7;outline-offset:3px}button:disabled{opacity:.45;cursor:default}
.shoot{width:100%;border:1px solid #e2e0d9;background:#e2e0d9;color:#171818;margin:29px 0 12px}
.shoot:hover:not(:disabled){background:#f5f3ed}
.tests{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.tests button{border:1px solid #646968;background:transparent;color:#e0e0da}
.tests button:hover:not(:disabled){background:#26292a;border-color:#a6aca8}
.footer-note{color:#898f8c;letter-spacing:0;text-transform:none;margin:23px 0 0}
#message{min-height:21px;margin:14px 0 0;color:#d5ae87;font-size:13px}
@media(max-width:380px){h1{font-size:32px}.angles{gap:9px}.field input{font-size:19px}}
</style></head><body><main>
<header><div><p class="kicker">Photobooth / GPIO 01</p><h1>Shutter</h1></div><span class="tag kicker">TEST</span></header>
<div class="status-row"><span class="status-label">Status</span><div id="status" class="status-value" role="status" data-state="idle">Connecting...</div></div>
<form id="controls">
<p class="section-title">Exposure</p>
<label for="open_seconds">Open time</label>
<div class="field"><input id="open_seconds" name="open_seconds" type="number" inputmode="decimal" min="0.001" max="30" step="any" value="{{OPEN_SECONDS}}" required><span class="unit">sec</span></div>
<p class="section-title">Servo position</p>
<div class="angles">
<div><label for="closed_angle">Closed angle</label>
 <div class="field"><input id="closed_angle" name="closed_angle" type="number" min="0" max="180" step="1" value="{{CLOSED}}" required><span class="unit">deg</span></div>
</div>
<div><label for="open_angle">Open angle</label>
 <div class="field"><input id="open_angle" name="open_angle" type="number" min="0" max="180" step="1" value="{{OPEN}}" required><span class="unit">deg</span></div>
</div>
</div>
<button class="shoot" type="submit" id="shoot">Shoot</button>
<div class="tests"><button type="button" id="open">Open</button><button type="button" id="close">Close</button></div>
</form>
<p class="footer-note">Open and Close cancel a shot. Settings reset when the board restarts.</p>
<div id="message" role="alert"></div>
<script>
const form=document.querySelector('#controls'),statusBox=document.querySelector('#status'),message=document.querySelector('#message');
let busy=false,shooting=false;
function buttons(){document.querySelector('#shoot').disabled=busy||shooting;document.querySelector('#open').disabled=busy;document.querySelector('#close').disabled=busy;}
function show(s){shooting=s.shooting;statusBox.dataset.state=s.shooting?'shooting':s.state;
 statusBox.textContent=s.state==='idle'?'Idle':(s.shooting?'Exposing · '+s.remaining_ms+' ms':s.state==='open'?'Open · '+s.angle+'°':'Closed · '+s.angle+'°');buttons();}
async function refresh(){try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();if(!busy)show(s);}catch(e){if(!busy){statusBox.dataset.state='idle';statusBox.textContent='Connection lost'};}}
async function command(action){
  if(busy)return;
  const relevant=action==='shoot'?[...form.querySelectorAll('input')]:[document.querySelector(action==='open'?'#open_angle':'#closed_angle')];
  if(!relevant.every(input=>input.reportValidity()))return;
  busy=true;buttons();message.textContent='';
  try{const body=new URLSearchParams();body.set('action',action);for(const input of relevant)body.set(input.name,input.value);
    const r=await fetch('/action',{method:'POST',body});if(!r.ok)throw Error(await r.text());show(await r.json());
  }catch(e){message.textContent=e.message||'Command failed; check connection.';}
  finally{busy=false;buttons();refresh();}
}
form.addEventListener('submit',e=>{e.preventDefault();command('shoot');});
document.querySelector('#open').onclick=()=>command('open');document.querySelector('#close').onclick=()=>command('close');
async function poll(){await refresh();setTimeout(poll,500);}poll();
</script></main></body></html>)HTML";

void sendStatus() {
  const auto state = shutter.status();
  const char *position = state.position == PhotoboothShutter::Position::Idle
      ? "idle" : state.position == PhotoboothShutter::Position::Open ? "open" : "closed";
  String json = "{\"state\":\"" + String(position) + "\",\"shooting\":";
  json += state.shooting ? "true" : "false";
  json += ",\"angle\":" + String(state.angle) + ",\"remaining_ms\":" + String(state.remainingMs);
  json += ",\"completed_shots\":" + String(state.completedShots);
  json += ",\"open_seconds\":\"" + formatSeconds(openDurationUs) + "\",\"closed_angle\":" + String(closedAngle);
  json += ",\"open_angle\":" + String(openAngle) + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleAction() {
  const String action = server.arg("action");
  uint32_t requestedDurationUs = openDurationUs, requestedClosed = closedAngle, requestedOpen = openAngle;
  if (action != "shoot" && action != "open" && action != "close") {
    server.send(400, "text/plain", "Unknown action."); return;
  }
  if ((action != "close" && !parseNumber(server.arg("open_angle"), 0, 180, requestedOpen)) ||
      (action != "open" && !parseNumber(server.arg("closed_angle"), 0, 180, requestedClosed)) ||
      (action == "shoot" && !parseSeconds(server.arg("open_seconds"), requestedDurationUs))) {
    server.send(400, "text/plain", "Use whole angles from 0 to 180 and a decimal time from 0.001 to 30 seconds (up to 6 decimal places)."); return;
  }
  if (action == "shoot" && shutter.status().shooting) {
    server.send(409, "text/plain", "A shot is in progress. Press Close to cancel it."); return;
  }
  if (action == "shoot") {
    if (!shutter.shoot(requestedOpen, requestedClosed, requestedDurationUs)) {
      server.send(503, "text/plain", "Could not start the shutter timer."); return;
    }
  } else {
    shutter.move(action == "open", action == "open" ? requestedOpen : requestedClosed);
  }
  openDurationUs = requestedDurationUs; closedAngle = requestedClosed; openAngle = requestedOpen;
  Serial.printf("SHUTTER TEST action=%s: open=%s s, closed=%lu deg, open=%lu deg\n",
                action.c_str(), formatSeconds(openDurationUs).c_str(),
                static_cast<unsigned long>(closedAngle), static_cast<unsigned long>(openAngle));
  sendStatus();
}
}  // namespace

void setup() {
  Serial.begin(115200);
  if (!shutter.begin(1)) {
    Serial.println("SHUTTER TEST ERROR: servo initialization failed"); return;
  }
  if (!PhotoboothWiFi::begin("ESP32-Shutter-Test", "shuttertest")) return;
  server.on("/", HTTP_GET, []() {
    String page = FPSTR(kPage);
    page.replace("{{OPEN_SECONDS}}", formatSeconds(openDurationUs));
    page.replace("{{CLOSED}}", String(closedAngle));
    page.replace("{{OPEN}}", String(openAngle));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html", page);
  });
  server.on("/status", HTTP_GET, sendStatus);
  server.on("/action", HTTP_POST, handleAction);
  server.onNotFound([](){server.send(404,"text/plain","Not found");});
  server.begin();
  ready = true;
  Serial.println("SHUTTER TEST ready: GPIO1, servo idle until first command");
}

void loop() {
  if (!ready) { delay(10); return; }
  server.handleClient();
  PhotoboothWiFi::loop();
  const auto state = shutter.status();
  if (state.completedShots != reportedShots) {
    Serial.printf("SHUTTER TEST shot complete: closed=%u deg\n", state.angle);
    reportedShots = state.completedShots;
  }
  if (millis() - lastStatusMs >= 5000) {
    Serial.printf("SHUTTER TEST running: GPIO1, open=%s s, closed=%lu deg, open=%lu deg, shooting=%d, URL=%s\n",
                  formatSeconds(openDurationUs).c_str(), static_cast<unsigned long>(closedAngle),
                  static_cast<unsigned long>(openAngle), state.shooting, PhotoboothWiFi::pageUrl().c_str());
    lastStatusMs = millis();
  }
  delay(1);
}
