#include "web_portal.h"
#include "config.h"
#include "net_manager.h"
#include "stations.h"
#include "time_service.h"
#include "audio.h"
#include "system_info.h"
#include "correctness_guards.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

WebPortal webPortal;

static const char WEB_PAGE[] PROGMEM = R"HTML(
<!doctype html><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>ESP32 Radio</title><style>body{font:16px system-ui;max-width:760px;margin:2em auto;padding:0 1em}input,select,button{font:inherit;padding:.45em;margin:.2em}input{width:95%}li{margin:.45em 0}small{color:#555}</style>
<h1>ESP32 Internet Radio</h1><p id=status>Loading&hellip;</p>
<h2>Wi-Fi</h2><form id=wifi><input name=ssid placeholder="Wi-Fi name" required><input name=password type=password placeholder="Wi-Fi password"><button>Save and connect</button></form>
<h2>Time zone</h2><form id=timezone><select id=tzpreset><option value="">Choose a common region&hellip;</option><option value="EST5EDT,M3.2.0,M11.1.0">US/Canada Eastern</option><option value="CST6CDT,M3.2.0,M11.1.0">US/Canada Central</option><option value="MST7MDT,M3.2.0,M11.1.0">US/Canada Mountain</option><option value="PST8PDT,M3.2.0,M11.1.0">US/Canada Pacific</option><option value="GMT0BST,M3.5.0/1,M10.5.0">United Kingdom</option><option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe</option><option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Romania / Eastern Europe</option><option value="JST-9">Japan</option><option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia Eastern</option></select><input id=tzrule name=timezone placeholder="POSIX timezone rule for another location" required><button>Save time zone</button></form><small>Choose a region or enter a POSIX rule. The choice is stored on the radio.</small>
<h2>Saved stations</h2><form id=station><input name=name placeholder="Station name" required><input name=url placeholder="Direct http(s) stream URL" required><select name=codec><option value=mp3>MP3</option><option value=aac>AAC / AAC+</option></select><button>Add station</button></form><ul id=stations></ul>
<h2>Favorites</h2><ul id=favorites></ul><small>Changes are saved in the ESP32's NVS flash immediately.</small>
<script>
async function api(path,opt){let r=await fetch(path,opt);let j=await r.json();if(!r.ok)throw Error(j.error||'Request failed');return j}
function esc(s){let e=document.createElement('span');e.textContent=s;return e.innerHTML}
async function load(){let d=await api('/api/status');status.textContent=d.connected?'Connected to '+d.ssid+' at http://'+d.ip:'Setup AP: '+d.portal+' (open http://192.168.4.1)';tzrule.value=d.timezone||'';let x=await api('/api/stations');stations.innerHTML=x.stations.map((s,i)=>`<li><b>${esc(s.name)}</b> <small>${esc(s.codec)} &middot; ${esc(s.url)}</small> <button onclick="del('station',${i})">Delete</button></li>`).join('')||'<li>No saved stations</li>';favorites.innerHTML=x.favorites.map((s,i)=>`<li><b>${esc(s.name)}</b> <button onclick="del('favorite',${i})">Remove</button></li>`).join('')||'<li>No favorites</li>'}
async function del(kind,index){if(confirm('Remove this item?')){await api('/api/'+kind+'/'+index,{method:'DELETE'});load()}}
wifi.onsubmit=async e=>{e.preventDefault();try{await api('/api/wifi',{method:'POST',body:new URLSearchParams(new FormData(wifi))});alert('Saved. If it connects, open the address shown on the radio.');load()}catch(e){alert(e.message)}};
tzpreset.onchange=()=>{if(tzpreset.value)tzrule.value=tzpreset.value};timezone.onsubmit=async e=>{e.preventDefault();try{await api('/api/timezone',{method:'POST',body:new URLSearchParams(new FormData(timezone))});alert('Time zone saved.');load()}catch(e){alert(e.message)}};
station.onsubmit=async e=>{e.preventDefault();try{await api('/api/stations',{method:'POST',body:new URLSearchParams(new FormData(station))});station.reset();load()}catch(e){alert(e.message)}};load();
</script>)HTML";

void WebPortal::registerRoutes() {
    server.on("/", HTTP_GET, [this] { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this] { handleStatus(); });
    server.on("/api/diagnostics", HTTP_GET, [this] { handleDiagnostics(); });
    server.on("/api/stations", HTTP_GET, [this] { handleStations(); });
    server.on("/api/stations", HTTP_POST, [this] { handleAddStation(); });
    server.on("/api/wifi", HTTP_POST, [this] { handleWiFiSave(); });
    server.on("/api/timezone", HTTP_POST, [this] { handleTimezoneSave(); });
    server.onNotFound([this] {
        String uri = server.uri();
        if (server.method() == HTTP_DELETE && uri.startsWith("/api/station/")) { handleDeleteStation(); return; }
        if (server.method() == HTTP_DELETE && uri.startsWith("/api/favorite/")) { handleDeleteFavorite(); return; }
        if (configPortalActive) { server.sendHeader("Location", "http://192.168.4.1/"); server.send(302, "text/plain", ""); }
        else sendJsonError(404, "Not found");
    });
}

void WebPortal::begin() {
    if (started) return;
    registerRoutes();
    server.begin();
    started = true;
    Serial.printf("[WEB] Ready: http://%s/\n", wifiManager.getIP());
}

void WebPortal::beginConfigPortal() {
    if (!started) { registerRoutes(); server.begin(); started = true; }
    uint64_t chip = ESP.getEfuseMac();
    snprintf(portalSSID, sizeof(portalSSID), "%s%04X", PORTAL_AP_PREFIX, (uint16_t)chip);
    snprintf(portalPassword, sizeof(portalPassword), "radio-%04X", (uint16_t)(chip >> 16));
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(portalSSID, portalPassword)) { Serial.println("[WEB] Setup AP failed"); return; }
    dnsServer.start(53, "*", WiFi.softAPIP());
    configPortalActive = true;
    Serial.printf("[WEB] Setup AP '%s', password '%s', portal http://192.168.4.1/\n", portalSSID, portalPassword);
}

void WebPortal::handle() { if (started) server.handleClient(); if (configPortalActive) dnsServer.processNextRequest(); }
void WebPortal::sendJsonError(int code, const char* message) { server.send(code, "application/json", String("{\"error\":\"") + message + "\"}"); }
void WebPortal::handleRoot() { server.send_P(200, "text/html; charset=utf-8", WEB_PAGE); }

void WebPortal::handleStatus() {
    JsonDocument doc; doc["connected"] = wifiManager.isConnected(); doc["ssid"] = wifiManager.getSSID(); doc["ip"] = wifiManager.getIP(); doc["portal"] = configPortalActive ? portalSSID : ""; doc["timezone"] = timeService.getTimezone();
    String out; serializeJson(doc, out); server.send(200, "application/json", out);
}
void WebPortal::handleDiagnostics() {
    AudioDiagnostics audio{};
    bool haveAudioSnapshot = audioPlayer.getDiagnostics(audio);
    WiFiDiagnostics wifi{};
    wifiManager.getDiagnostics(wifi);

    JsonDocument doc;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["build_git_id"] = BUILD_GIT_ID;
    doc["uptime_ms"] = millis();
    doc["reset_reason"] = SystemInfo::resetReasonName(esp_reset_reason());
    doc["free_internal_heap"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    doc["largest_internal_heap_block"] = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t allocatorRegionMinimum =
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // ESP-IDF defines this value as the sum of low-water marks for all
    // currently registered matching heap regions. It is not a single global
    // minimum: adding a region can make the aggregate increase in one boot.
    doc["minimum_free_internal_heap"] = allocatorRegionMinimum;
    doc["allocator_region_minimum_free_internal_heap"] = allocatorRegionMinimum;
    doc["minimum_free_internal_heap_scope"] =
        "sum_of_current_region_low_watermarks";
    doc["total_psram"] = ESP.getPsramSize();
    doc["free_psram"] = ESP.getFreePsram();
    doc["wifi_connected"] = wifi.connected;
    doc["wifi_state"] = WiFiManager::stateName(wifi.state);
    doc["wifi_framework_status"] =
        WiFiManager::frameworkStatusName(wifi.frameworkStatus);
    doc["wifi_framework_status_code"] = wifi.frameworkStatus;
    doc["wifi_ssid"] = wifi.ssid;
    doc["wifi_ip"] = wifi.ip;
    doc["wifi_auto_reconnect"] = wifi.autoReconnect;
    if (wifi.connected) doc["wifi_rssi_dbm"] = wifi.rssi;
    else doc["wifi_rssi_dbm"] = nullptr;
    doc["wifi_disconnect_count"] = wifi.disconnectCount;
    doc["wifi_got_ip_count"] = wifi.gotIpCount;
    doc["wifi_lost_ip_count"] = wifi.lostIpCount;
    doc["wifi_processed_event_count"] = wifi.processedEventCount;
    doc["wifi_dropped_event_count"] = wifi.droppedEventCount;
    if (wifi.hasDisconnected) {
        doc["wifi_last_disconnect_reason"] = wifi.lastDisconnectReason;
        doc["wifi_last_disconnect_reason_name"] =
            wifi.lastDisconnectReasonName;
        doc["wifi_last_disconnect_ms"] = wifi.lastDisconnectMs;
        doc["wifi_last_disconnect_age_ms"] =
            (uint32_t)(millis() - wifi.lastDisconnectMs);
    } else {
        doc["wifi_last_disconnect_reason"] = nullptr;
        doc["wifi_last_disconnect_reason_name"] = "none";
        doc["wifi_last_disconnect_ms"] = nullptr;
        doc["wifi_last_disconnect_age_ms"] = nullptr;
    }
    doc["wifi_state_since_ms"] = wifi.stateSinceMs;
    doc["wifi_state_age_ms"] = (uint32_t)(millis() - wifi.stateSinceMs);
    if (wifi.hasConnectedSince) {
        doc["wifi_connected_since_ms"] = wifi.connectedSinceMs;
        doc["wifi_connected_duration_ms"] =
            (uint32_t)(millis() - wifi.connectedSinceMs);
    } else {
        doc["wifi_connected_since_ms"] = nullptr;
        doc["wifi_connected_duration_ms"] = nullptr;
    }
    if (wifi.hasGotIp) doc["wifi_last_got_ip_ms"] = wifi.lastGotIpMs;
    else doc["wifi_last_got_ip_ms"] = nullptr;
    if (wifi.hasLostIp) doc["wifi_last_lost_ip_ms"] = wifi.lastLostIpMs;
    else doc["wifi_last_lost_ip_ms"] = nullptr;

    if (haveAudioSnapshot) {
        doc["audio_source"] = SystemInfo::audioSourceName(audio.source);
        doc["playback_state"] = SystemInfo::playbackStateName(audio.state);
        doc["codec"] = SystemInfo::audioCodecName(audio.codec);
        doc["bluetooth_lifecycle"] = SystemInfo::bluetoothLifecycleName(audio.bluetoothLifecycle);
        doc["bluetooth_restartable"] = audio.bluetoothRestartable;
        doc["stream_url"] = audio.streamURL;
        doc["audio_task_stack_high_water_mark"] = audio.taskStackHighWaterMark;
    } else {
        doc["audio_source"] = "unavailable";
        doc["playback_state"] = "unavailable";
        doc["codec"] = "unavailable";
        doc["bluetooth_lifecycle"] = "unavailable";
        doc["bluetooth_restartable"] = false;
        doc["stream_url"] = "";
        doc["audio_task_stack_high_water_mark"] = 0;
    }

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}
void WebPortal::handleStations() {
    JsonDocument doc; JsonArray stations = doc["stations"].to<JsonArray>(); JsonArray favorites = doc["favorites"].to<JsonArray>();
    for (uint8_t i=0;i<stationManager.getStationCount();i++) { RadioStation* s=stationManager.getStation(i); JsonObject o=stations.add<JsonObject>(); o["name"]=s->name; o["url"]=s->url; o["codec"]=s->codec==STATION_CODEC_AAC?"AAC":"MP3"; }
    for (uint8_t i=0;i<stationManager.getFavoriteCount();i++) { RadioStation* s=stationManager.getFavorite(i); JsonObject o=favorites.add<JsonObject>(); o["name"]=s->name; o["url"]=s->url; o["codec"]=s->codec==STATION_CODEC_AAC?"AAC":"MP3"; }
    String out; serializeJson(doc,out); server.send(200,"application/json",out);
}
void WebPortal::handleAddStation() {
    String name=server.arg("name"), url=server.arg("url"), codec=server.arg("codec");
    if (name.isEmpty() || name.length() >= MAX_NAME_LENGTH || url.length() >= MAX_URL_LENGTH || !(url.startsWith("http://") || url.startsWith("https://"))) { sendJsonError(400,"Use a name and direct http(s) URL within the field limits"); return; }
    StationCodec c = codec == "aac" ? STATION_CODEC_AAC : STATION_CODEC_MP3;
    if (stationManager.addStation(name.c_str(),url.c_str(),c)==0xFF) { sendJsonError(409,"Station list is full"); return; }
    server.send(201,"application/json","{\"ok\":true}");
}
void WebPortal::handleDeleteStation() {
    const String prefix = "/api/station/";
    const String suffix = server.uri().substring(prefix.length());
    uint8_t index;
    if (!CorrectnessGuards::parseListIndex(
            suffix.c_str(), suffix.length(), stationManager.getStationCount(), index) ||
        !stationManager.removeStation(index)) {
        sendJsonError(404, "Station not found");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

void WebPortal::handleDeleteFavorite() {
    const String prefix = "/api/favorite/";
    const String suffix = server.uri().substring(prefix.length());
    uint8_t index;
    if (!CorrectnessGuards::parseListIndex(
            suffix.c_str(), suffix.length(), stationManager.getFavoriteCount(), index) ||
        !stationManager.removeFavorite(index)) {
        sendJsonError(404, "Favorite not found");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true}");
}
void WebPortal::handleWiFiSave() {
    String ssid=server.arg("ssid"), pass=server.arg("password");
    if (ssid.isEmpty() || ssid.length() >= MAX_SSID_LENGTH || pass.length() >= MAX_PASS_LENGTH) { sendJsonError(400,"Invalid Wi-Fi name or password length"); return; }
    bool connected=wifiManager.connect(ssid.c_str(),pass.c_str());
    if (connected) { if (configPortalActive) { dnsServer.stop(); WiFi.softAPdisconnect(true); configPortalActive=false; } begin(); server.send(200,"application/json","{\"ok\":true,\"connected\":true}"); }
    else sendJsonError(503,"Could not connect; credentials were not saved");
}
void WebPortal::handleTimezoneSave() {
    String timezone = server.arg("timezone");
    if (!timeService.setTimezone(timezone.c_str())) { sendJsonError(400, "Invalid POSIX timezone rule"); return; }
    server.send(200, "application/json", "{\"ok\":true}");
}
