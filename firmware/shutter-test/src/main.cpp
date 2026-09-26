#include <Arduino.h>
#include <PhotoboothShutter.h>
#include <PhotoboothWiFi.h>
#include <WebServer.h>

namespace {
WebServer server(80);
PhotoboothShutter shutter;
bool ready = false;
struct Speed { const char *label; uint32_t durationUs; };
constexpr Speed kSpeeds[] = {
  {"1/1000", 1000}, {"1/500", 2000}, {"1/250", 4000},
  {"1/125", 8000}, {"1/60", 16667}, {"1/30", 33333},
  {"1/15", 66667}, {"1/8", 125000}, {"1/4", 250000},
  {"1/2", 500000}, {"1s", 1000000}, {"2s", 2000000},
  {"3s", 3000000}, {"4s", 4000000}, {"8s", 8000000},
  {"15s", 15000000}, {"30s", 30000000}
};
constexpr uint32_t kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);
uint32_t speedIndex = 10;
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

const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Shutter test</title><style>
:root{font-family:system-ui,sans-serif;color:#eaf2fb;background:#111c29;color-scheme:dark}
body{margin:0;min-height:100vh;display:grid;place-items:center}
main{box-sizing:border-box;width:min(94vw,460px);margin:24px 0;padding:28px;background:#203247;border-radius:18px}
h1{margin:0 0 8px}p{color:#bdcedf;line-height:1.5}label{display:block;margin:20px 0;font-weight:600}
input{box-sizing:border-box;display:block;width:100%;margin-top:8px;padding:12px;border:1px solid #7c93ab;border-radius:8px;font:inherit;background:#132438}
button{padding:14px;border:0;border-radius:9px;font:inherit;font-weight:700;cursor:pointer;background:#cee2f3;color:#102435}
button:disabled{opacity:.45;cursor:wait}
.dial-wrap{text-align:center;margin:22px 0 28px}.dial-wrap>label{margin:0 0 12px}
.dial{position:relative;width:220px;height:220px;margin:auto;border-radius:50%;
  border:8px solid #627e95;background:radial-gradient(circle at 40% 33%,#34546b,#152537 78%);
  box-shadow:inset 0 7px 14px #0008,0 12px 20px #0005;touch-action:none;cursor:grab;outline:none}
.dial:active{cursor:grabbing}.dial:focus-visible{box-shadow:0 0 0 4px #51d6b0,inset 0 7px 14px #0008}
.tick{position:absolute;left:50%;top:50%;width:3px;height:14px;background:#93b4c9;border-radius:2px;
  transform:rotate(var(--a)) translateY(-91px);transform-origin:center center}
.tick.active{height:19px;width:5px;background:#51d6b0}
.needle{position:absolute;left:calc(50% - 3px);top:calc(50% - 105px);width:6px;height:29px;
  background:#51d6b0;border-radius:4px;transform-origin:3px 105px;transform:rotate(var(--a))}
.dial-value{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;
  font-size:2rem;font-weight:800;pointer-events:none}.dial-help{margin:10px 0 0;font-size:.86rem}
.shoot{width:100%;background:#51d6b0}.tests{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:12px}
#status{padding:12px;background:#132438;border-radius:8px}#message{min-height:1.5em;color:#ffd398}.hint{font-size:.87rem}
</style></head><body><main>
<h1>Shutter test</h1><p>Set the servo positions, then take a shot.</p>
<div id="status" role="status">Connecting...</div>
<form id="controls">
<div class="dial-wrap"><label id="speed-label">Shutter speed</label>
<div class="dial" id="dial" role="slider" tabindex="0" aria-labelledby="speed-label"
 aria-valuemin="0" aria-valuemax="16" aria-valuenow="{{SPEED_INDEX}}" aria-valuetext="{{SPEED_LABEL}}">
<div id="ticks"></div><div class="needle" id="needle"></div><div class="dial-value" id="dial-value">{{SPEED_LABEL}}</div>
</div><p class="dial-help">Turn the dial, scroll, or use arrow keys. Fast speeds may end before the servo reaches open.</p>
<input type="hidden" id="speed_index" name="speed_index" value="{{SPEED_INDEX}}"></div>
<label>Closed servo angle (&deg;)<input id="closed_angle" name="closed_angle" type="number" min="0" max="180" step="1" value="{{CLOSED}}" required></label>
<label>Open servo angle (&deg;)<input id="open_angle" name="open_angle" type="number" min="0" max="180" step="1" value="{{OPEN}}" required></label>
<button class="shoot" type="submit" id="shoot">Shoot</button>
<div class="tests"><button type="button" id="open">Open</button><button type="button" id="close">Close</button></div>
</form>
<p class="hint">Open and Close cancel a shot and immediately command that position. Each button uses the angles above.</p>
<p class="hint">Servo idle at startup. Calibrate with small angle changes. Settings reset after power off.</p>
<div id="message" role="alert"></div>
<script>
const form=document.querySelector('#controls'),statusBox=document.querySelector('#status'),message=document.querySelector('#message');
let busy=false,shooting=false;
const speeds=['1/1000','1/500','1/250','1/125','1/60','1/30','1/15','1/8','1/4','1/2','1s','2s','3s','4s','8s','15s','30s'];
const dial=document.querySelector('#dial'),ticks=document.querySelector('#ticks'),needle=document.querySelector('#needle');
const speedInput=document.querySelector('#speed_index');
let speed=Number(speedInput.value);
for(let i=0;i<speeds.length;i++){const t=document.createElement('span');t.className='tick';t.style.setProperty('--a',(-135+i*270/(speeds.length-1))+'deg');ticks.append(t);}
function setSpeed(i){speed=Math.max(0,Math.min(speeds.length-1,i));speedInput.value=String(speed);
 dial.setAttribute('aria-valuenow',String(speed));dial.setAttribute('aria-valuetext',speeds[speed]);
 document.querySelector('#dial-value').textContent=speeds[speed];needle.style.setProperty('--a',(-135+speed*270/(speeds.length-1))+'deg');
 [...ticks.children].forEach((t,n)=>t.classList.toggle('active',n===speed));}
function angleFromPointer(e){const b=dial.getBoundingClientRect(),x=e.clientX-b.left-b.width/2,y=e.clientY-b.top-b.height/2;
 let a=Math.atan2(y,x)*180/Math.PI+90;if(a>180)a-=360;
 return Math.max(0,Math.min(speeds.length-1,Math.round((a+135)*(speeds.length-1)/270)));}
dial.addEventListener('pointerdown',e=>{dial.setPointerCapture(e.pointerId);setSpeed(angleFromPointer(e));dial.focus();});
dial.addEventListener('pointermove',e=>{if(dial.hasPointerCapture(e.pointerId))setSpeed(angleFromPointer(e));});
dial.addEventListener('wheel',e=>{e.preventDefault();setSpeed(speed+(e.deltaY>0?1:-1));},{passive:false});
dial.addEventListener('keydown',e=>{if(['ArrowRight','ArrowDown','ArrowLeft','ArrowUp','Home','End'].includes(e.key)){
 e.preventDefault();setSpeed(e.key==='Home'?0:e.key==='End'?speeds.length-1:speed+(['ArrowRight','ArrowDown'].includes(e.key)?1:-1));}});
setSpeed(speed);
function buttons(){document.querySelector('#shoot').disabled=busy||shooting;document.querySelector('#open').disabled=busy;document.querySelector('#close').disabled=busy;}
function show(s){shooting=s.shooting;statusBox.textContent=s.state==='idle'?'Idle - servo has not moved':(s.shooting?'Shooting - '+s.remaining_ms+' ms remaining':s.state==='open'?'Open':'Closed')+' | commanded '+s.angle+'\u00b0';buttons();}
async function refresh(){try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();if(!busy)show(s);}catch(e){if(!busy)statusBox.textContent='Connection lost - checking again...';}}
async function command(action){
  if(busy)return;
  const relevant=action==='shoot'?[...form.querySelectorAll('input:not([type=hidden])')]:[document.querySelector(action==='open'?'#open_angle':'#closed_angle')];
  if(!relevant.every(input=>input.reportValidity()))return;
  busy=true;buttons();message.textContent='';
  try{const body=new URLSearchParams();body.set('action',action);for(const input of relevant)body.set(input.name,input.value);if(action==='shoot')body.set('speed_index',speedInput.value);
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
  json += ",\"speed_index\":" + String(speedIndex) + ",\"speed\":\"" + String(kSpeeds[speedIndex].label) + "\",\"closed_angle\":" + String(closedAngle);
  json += ",\"open_angle\":" + String(openAngle) + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleAction() {
  const String action = server.arg("action");
  uint32_t requestedSpeed = speedIndex, requestedClosed = closedAngle, requestedOpen = openAngle;
  if (action != "shoot" && action != "open" && action != "close") {
    server.send(400, "text/plain", "Unknown action."); return;
  }
  if ((action != "close" && !parseNumber(server.arg("open_angle"), 0, 180, requestedOpen)) ||
      (action != "open" && !parseNumber(server.arg("closed_angle"), 0, 180, requestedClosed)) ||
      (action == "shoot" && !parseNumber(server.arg("speed_index"), 0, kSpeedCount - 1, requestedSpeed))) {
    server.send(400, "text/plain", "Use whole angles from 0 to 180 and a valid shutter speed."); return;
  }
  if (action == "shoot" && shutter.status().shooting) {
    server.send(409, "text/plain", "A shot is in progress. Press Close to cancel it."); return;
  }
  if (action == "shoot") {
    if (!shutter.shoot(requestedOpen, requestedClosed, kSpeeds[requestedSpeed].durationUs)) {
      server.send(503, "text/plain", "Could not start the shutter timer."); return;
    }
  } else {
    shutter.move(action == "open", action == "open" ? requestedOpen : requestedClosed);
  }
  speedIndex = requestedSpeed; closedAngle = requestedClosed; openAngle = requestedOpen;
  Serial.printf("SHUTTER TEST action=%s: speed=%s, closed=%lu deg, open=%lu deg\n",
                action.c_str(), kSpeeds[speedIndex].label,
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
    page.replace("{{SPEED_INDEX}}", String(speedIndex));
    page.replace("{{SPEED_LABEL}}", String(kSpeeds[speedIndex].label));
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
    Serial.printf("SHUTTER TEST running: GPIO1, speed=%s, closed=%lu deg, open=%lu deg, shooting=%d, URL=%s\n",
                  kSpeeds[speedIndex].label, static_cast<unsigned long>(closedAngle),
                  static_cast<unsigned long>(openAngle), state.shooting, PhotoboothWiFi::pageUrl().c_str());
    lastStatusMs = millis();
  }
  delay(1);
}
