#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include <esp_log.h>
#include "muse.h"
#include "web_service.h"

static WebServer server(80);
static const char* TAG = "web_service";
static String apSsid;
static String apPassword;
static const size_t LOG_CAPACITY = 50;
static String eventLog[LOG_CAPACITY];
static size_t eventLogCount = 0;
static size_t eventLogNext = 0;

void web_service_log(const char* message) {
  String line = String(millis() / 1000) + "s | " + String(message ? message : "(empty event)");
  eventLog[eventLogNext] = line;
  eventLogNext = (eventLogNext + 1) % LOG_CAPACITY;
  if (eventLogCount < LOG_CAPACITY) ++eventLogCount;
  Serial.println(line);
}

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#101117"><title>LVS Gateway Debug</title>
<style>
:root{color-scheme:dark;--bg:#101117;--panel:#1b1e29;--line:#35394a;--text:#f3f4fa;--muted:#a5a9bb;--accent:#a6f36b;--danger:#ff818a}*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at 20% 0%,#25263a 0,transparent 45%),var(--bg);color:var(--text);font:15px/1.5 system-ui,sans-serif}main{max-width:760px;margin:auto;padding:26px 15px 48px}.brand{display:flex;gap:12px;align-items:center}.logo{display:grid;place-items:center;width:44px;height:44px;border-radius:13px;background:var(--accent);color:#111;font-weight:900}.muted{color:var(--muted)}h1{font-size:27px;letter-spacing:-.04em;margin:0}h2{font-size:17px;margin:0 0 12px}.card{margin-top:16px;background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:17px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}.kv{padding:10px;border-radius:10px;background:#131620;overflow-wrap:anywhere}.k{font-size:12px;color:var(--muted)}.v{font-weight:700;overflow-wrap:anywhere}button{font:inherit;font-weight:700;border-radius:10px;border:1px solid var(--line);padding:10px 13px;cursor:pointer;background:#252938;color:var(--text)}button.primary{background:var(--accent);border-color:var(--accent);color:#10130d}button.stop{color:var(--danger)}button:disabled{opacity:.5}.buttons{display:flex;gap:9px;flex-wrap:wrap}.status{font-size:13px;color:var(--muted);margin-top:10px;min-height:20px}.warning{font-size:13px;color:#e9dcb7;border-left:3px solid #d6aa57;padding-left:12px;margin-top:12px}.device{padding:12px 0;border-top:1px solid var(--line);overflow-wrap:anywhere}.device:first-child{border-top:0}input[type=range]{width:100%;accent-color:var(--accent);margin:17px 0}
</style></head><body><main><div class="brand"><div class="logo">LVS</div><div><h1>Gateway debug</h1><p class="muted">One ESP32 · radio diagnostics</p></div></div>
<section class="card"><h2>ESP32 details</h2><div id="info" class="grid"><div class="muted">Loading diagnostics…</div></div><div class="buttons" style="margin-top:13px"><button id="refresh" class="primary">Refresh details</button></div><div id="debugStatus" class="status" role="status"></div></section>
<section class="card"><h2>Nearby Bluetooth LE devices</h2><p class="muted">Scan nearby BLE advertisements and inspect names, addresses, signal strength, and advertised services. Some toys may use another radio protocol or hide/rotate their addresses.</p><div class="buttons" style="margin-top:12px"><button id="scan" class="primary">Scan for 5 seconds</button></div><div id="scanStatus" class="status" role="status">Not scanned yet</div><div id="devices"></div></section>
<section class="card"><h2>Connection event log</h2><p class="muted">Shows BLE client connects/disconnects, commands received, scan findings, and gateway test events. Keep this page open while you connect the app or toy, then copy the log and send it back.</p><div class="buttons"><button id="refreshLogs" class="primary">Refresh log</button><button id="copyLogs">Copy log text</button></div><div id="logStatus" class="status">Waiting for events</div><pre id="logs" style="white-space:pre-wrap;overflow-wrap:anywhere;max-height:360px;overflow:auto;background:#131620;padding:12px;border-radius:10px;font:12px/1.5 ui-monospace,monospace">No events recorded yet.</pre></section>
<section class="card"><h2>Broadcast test</h2><p class="muted">This sends the existing gateway-wide MuSE command. Start at zero and only test while you can observe the toys.</p><div style="display:flex;justify-content:space-between;gap:10px"><strong>Intensity</strong><strong id="value">0 / 20</strong></div><input id="speed" type="range" min="0" max="20" value="0" aria-label="Intensity"><div class="buttons"><button id="send" class="primary">Send test command</button><button id="stop" class="stop">Stop broadcast</button></div><div id="controlStatus" class="status" role="status">Ready</div><p class="warning">This firmware currently broadcasts MuSE commands and does not target a toy by MAC address. A discovered BLE MAC is evidence for debugging, not proof that individual control is supported.</p></section>
</main><script>
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
async function req(url,options){const r=await fetch(url,options);const j=await r.json();if(!r.ok)throw new Error(j.error||'Request failed');return j}
async function refresh(){const el=document.querySelector('#info'),st=document.querySelector('#debugStatus');st.textContent='Refreshing…';try{const j=await req('/api/status');const rows=[['BLE name',j.bleName],['Wi-Fi AP name',j.name],['AP address',j.ip],['AP MAC',j.apMac],['Wi-Fi station MAC',j.staMac],['Bluetooth MAC',j.btMac],['Uptime',j.uptime],['Free heap',j.freeHeap+' bytes'],['Chip model',j.chip],['CPU frequency',j.cpuMHz+' MHz']];el.innerHTML=rows.map(([k,v])=>'<div class="kv"><div class="k">'+esc(k)+'</div><div class="v">'+esc(v||'Unavailable')+'</div></div>').join('');st.textContent='Details refreshed'}catch(e){st.textContent='Error: '+e.message}}
document.querySelector('#refresh').addEventListener('click',refresh);
async function refreshLogs(){const st=document.querySelector('#logStatus'),el=document.querySelector('#logs');try{const j=await req('/api/logs');el.textContent=j.logs.length?j.logs.join('\\n'):'No events recorded yet.';st.textContent='Showing '+j.logs.length+' event(s) · refreshes automatically'}catch(e){st.textContent='Log error: '+e.message}}
document.querySelector('#refreshLogs').addEventListener('click',refreshLogs);
document.querySelector('#copyLogs').addEventListener('click',async()=>{try{await navigator.clipboard.writeText(document.querySelector('#logs').textContent);document.querySelector('#logStatus').textContent='Log copied. Paste it into your reply.'}catch(e){document.querySelector('#logStatus').textContent='Clipboard unavailable; select and copy the log text manually.'}});

document.querySelector('#scan').addEventListener('click',async()=>{const b=document.querySelector('#scan'),st=document.querySelector('#scanStatus'),list=document.querySelector('#devices');b.disabled=true;st.textContent='Scanning for 5 seconds…';list.innerHTML='';try{const j=await req('/api/scan',{method:'POST'});st.textContent='Scan complete: '+j.count+' device(s) found';if(!j.devices.length){list.innerHTML='<p class="muted">No BLE advertisements found. The toys may use another radio mode or may not be advertising.</p>';return}list.innerHTML=j.devices.map(d=>'<div class="device"><strong>'+esc(d.name||'(no advertised name)')+'</strong><div class="muted">'+esc(d.address)+' · RSSI '+esc(d.rssi)+' dBm</div><div class="muted">Services: '+esc(d.services||'not advertised')+'</div></div>').join('')}catch(e){st.textContent='Scan error: '+e.message}finally{b.disabled=false}});
const slider=document.querySelector('#speed');slider.addEventListener('input',()=>document.querySelector('#value').textContent=slider.value+' / 20');
async function command(speed){const st=document.querySelector('#controlStatus');st.textContent='Sending…';try{const j=await req('/api/vibrate',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'speed='+encodeURIComponent(speed)});st.textContent=j.message||'Command sent'}catch(e){st.textContent='Error: '+e.message}}
document.querySelector('#send').addEventListener('click',()=>command(slider.value));document.querySelector('#stop').addEventListener('click',()=>{slider.value='0';document.querySelector('#value').textContent='0 / 20';command(0)});refresh();refreshLogs();setInterval(refreshLogs,2000);
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

static String jsonEscape(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if ((uint8_t)c >= 0x20) out += c;
  }
  return out;
}

static String macString(esp_mac_type_t type) {
  uint8_t mac[6] = {};
  if (esp_read_mac(mac, type) != ESP_OK) return "unavailable";
  char out[18];
  snprintf(out, sizeof(out), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(out);
}

static void handleStatus() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char bleName[32];
  snprintf(bleName, sizeof(bleName), "LVS-Gateway-%02X%02X%02X", mac[3], mac[4], mac[5]);
  String json = "{";
  json += "\"name\":\"" + jsonEscape(WiFi.softAPSSID()) + "\",";
  json += "\"mode\":\"access-point\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"apMac\":\"" + jsonEscape(WiFi.softAPmacAddress()) + "\",";
  json += "\"staMac\":\"" + macString(ESP_MAC_WIFI_STA) + "\",";
  json += "\"btMac\":\"" + macString(ESP_MAC_BT) + "\",";
  json += "\"bleName\":\"" + String(bleName) + "\",";
  json += "\"uptime\":\"" + String(millis() / 1000) + " s\",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"chip\":\"" + jsonEscape(ESP.getChipModel()) + "\",";
  json += "\"cpuMHz\":" + String(ESP.getCpuFreqMHz());
  json += "}";
  sendJson(200, json);
}

static void handleLogs() {
  String json = "{\\"logs\\":[";
  size_t oldest = (eventLogCount == LOG_CAPACITY) ? eventLogNext : 0;
  for (size_t i = 0; i < eventLogCount; ++i) {
    if (i) json += ",";
    size_t index = (oldest + i) % LOG_CAPACITY;
    json += "\\\"" + jsonEscape(eventLog[index]) + "\\\"";
  }
  json += "]}";
  sendJson(200, json);
}

static void handleScan() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  if (scan == nullptr) {
    sendJson(500, "{\"error\":\"BLE scanner unavailable\"}");
    return;
  }
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  ESP_LOGI(TAG, "Starting 5-second BLE diagnostic scan");
  web_service_log("BLE diagnostic scan started (5 seconds)");
  NimBLEScanResults results = scan->getResults(5000, false);
  String json = "{\"count\":" + String(results.getCount()) + ",\"devices\":[";
  bool first = true;
  for (uint32_t i = 0; i < foundCount; ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device == nullptr) continue;
    if (!first) json += ",";
    first = false;
    String name = device->haveName() ? String(device->getName().c_str()) : "";
    String address = String(device->getAddress().toString().c_str());
    String services;
    for (int s = 0; s < device->getServiceUUIDCount(); ++s) {
      if (s) services += ", ";
      services += String(device->getServiceUUID(s).toString().c_str());
    }
    json += "{\"name\":\"" + jsonEscape(name) + "\",";
    json += "\"address\":\"" + jsonEscape(address) + "\",";
    json += "\"rssi\":" + String(device->getRSSI()) + ",";
    json += "\"services\":\"" + jsonEscape(services) + "\"}";
  }
  json += "]}";
  scan->clearResults();
  web_service_log((String("BLE scan complete: ") + String(foundCount) + " advertisement(s) detected").c_str());
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
  server.on("/api/scan", HTTP_POST, handleScan);
  server.on("/api/logs", HTTP_GET, handleLogs);
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
