#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include "muse.h"
#include "web_service.h"

static WebServer server(80);
static const char* TAG = "web_service";
static String apSsid;
static String apPassword;

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#101117"><title>LVS Gateway</title>
<style>
:root{color-scheme:dark;--bg:#101117;--panel:#1b1e29;--line:#35394a;--text:#f3f4fa;--muted:#a5a9bb;--accent:#a6f36b;--danger:#ff818a}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at 20% 0%,#25263a 0,transparent 45%),var(--bg);color:var(--text);font:16px/1.5 system-ui,sans-serif}
main{max-width:620px;margin:auto;padding:28px 16px 50px}.brand{display:flex;gap:12px;align-items:center}.logo{display:grid;place-items:center;width:44px;height:44px;border-radius:13px;background:var(--accent);color:#111;font-weight:900}.muted{color:var(--muted)}h1{font-size:28px;letter-spacing:-.04em;margin:0}p{margin:5px 0}.card{margin-top:20px;background:var(--panel);border:1px solid var(--line);border-radius:17px;padding:20px}.row{display:flex;justify-content:space-between;gap:12px;align-items:center}.value{font-size:25px;font-weight:800;color:var(--accent);font-variant-numeric:tabular-nums}input[type=range]{width:100%;accent-color:var(--accent);margin:22px 0;cursor:pointer}button{font:inherit;font-weight:700;border-radius:11px;border:1px solid var(--line);padding:12px 16px;cursor:pointer;background:#252938;color:var(--text)}button.primary{background:var(--accent);border-color:var(--accent);color:#10130d}button.stop{color:var(--danger)}.buttons{display:flex;gap:10px}.buttons button{flex:1}.tag{font-size:12px;border:1px solid var(--line);padding:5px 9px;border-radius:9px;color:var(--muted)}.warning{font-size:13px;color:#e9dcb7;border-left:3px solid #d6aa57;padding-left:12px;margin-top:18px}.status{font-size:13px;color:var(--muted);margin-top:12px;min-height:20px}a{color:var(--accent)}
</style></head><body><main>
<div class="brand"><div class="logo">LVS</div><div><h1>Gateway control</h1><p class="muted">Local Wi-Fi control panel</p></div></div>
<section class="card"><div class="row"><strong>Broadcast intensity</strong><span class="value" id="value">0%</span></div>
<input id="speed" type="range" min="0" max="20" step="1" value="0" aria-label="Intensity">
<div class="row muted" style="font-size:12px"><span>Off</span><span>Maximum</span></div>
<div class="buttons" style="margin-top:18px"><button id="send" class="primary">Send intensity</button><button id="stop" class="stop">Stop</button></div>
<div id="status" class="status" role="status">Ready</div></section>
<section class="card"><div class="row"><strong>Gateway</strong><span class="tag" id="gateway">Checking…</span></div><p class="muted" style="font-size:13px;margin-top:10px">This page is served directly by the ESP32. Internet access is not required.</p>
<p class="warning">The LoveSpouse/MuSE command protocol broadcasts over radio without a unique toy address. Any compatible toy in range may respond. This page controls this gateway, not a guaranteed individually addressed physical toy.</p></section>
</main>
<script>
const slider=document.querySelector('#speed'),value=document.querySelector('#value'),status=document.querySelector('#status');
slider.addEventListener('input',()=>value.textContent=Math.round(Number(slider.value)/20*100)+'%');
async function send(path,body){status.textContent='Sending…';try{const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const j=await r.json();if(!r.ok)throw new Error(j.error||'Request failed');status.textContent=j.message||'Command sent';}catch(e){status.textContent='Error: '+e.message;}}
document.querySelector('#send').addEventListener('click',()=>send('/api/vibrate','speed='+encodeURIComponent(slider.value)));
document.querySelector('#stop').addEventListener('click',()=>{slider.value='0';value.textContent='0%';send('/api/vibrate','speed=0')});
fetch('/api/status').then(r=>r.json()).then(j=>{document.querySelector('#gateway').textContent=j.name||'Connected';}).catch(()=>{document.querySelector('#gateway').textContent='Local gateway';});
</script></body></html>
)HTML";

static void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", body);
}

static void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

static void handleStatus() {
  String json = "{\"name\":\"" + String(WiFi.softAPSSID()) + "\",\"mode\":\"access-point\",\"ip\":\"" + WiFi.softAPIP().toString() + "\"}";
  sendJson(200, json);
}

static void handleVibrate() {
  if (!server.hasArg("speed")) {
    sendJson(400, "{\"error\":\"Missing speed parameter\"}");
    return;
  }
  int speed = server.arg("speed").toInt();
  if (speed < 0 || speed > 20) {
    sendJson(400, "{\"error\":\"Speed must be between 0 and 20\"}");
    return;
  }
  muse_set_intensity(static_cast<float>(speed) / 20.0f);
  ESP_LOGI(TAG, "Local web command: speed %d/20", speed);
  sendJson(200, String("{\"ok\":true,\"speed\":") + speed + ",\"message\":\"" + (speed == 0 ? "Stop command sent" : "Intensity command sent") + "\"}");
}

void web_service_start() {
  uint8_t mac[6];
  WiFi.mode(WIFI_AP);
  WiFi.macAddress(mac);
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  apSsid = String("LVS-Gateway-") + suffix;
  char pass[9];
  snprintf(pass, sizeof(pass), "%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
  apPassword = String(pass);

  if (!WiFi.softAP(apSsid.c_str(), apPassword.c_str())) {
    Serial.println("ERROR: Could not start Wi-Fi access point.");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/vibrate", HTTP_POST, handleVibrate);
  server.onNotFound([]() {
    sendJson(404, "{\"error\":\"Not found\"}");
  });
  server.begin();

  Serial.println();
  Serial.println("Local dashboard Wi-Fi started:");
  Serial.print("  Network: ");
  Serial.println(apSsid);
  Serial.print("  Password: ");
  Serial.println(apPassword);
  Serial.println("  Open: http://192.168.4.1/");
}

void web_service_loop() {
  server.handleClient();
}
