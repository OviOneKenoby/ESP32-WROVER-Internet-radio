#include "web_portal.h"
#include "config.h"
#include "net_manager.h"
#include "stations.h"
#include "time_service.h"
#include "audio.h"
#include "system_info.h"
#include "correctness_guards.h"
#include "rtcp_server.h"
#include "remote_browser.h"
#include "diagnostics_psram_allocator.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

namespace {
DiagnosticsPsramAllocator diagnosticsPsramAllocator;
}  // namespace

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
    server.on("/", HTTP_GET, [this] { server.beginResponse("root"); handleRoot(); });
    server.on("/api/status", HTTP_GET, [this] { server.beginResponse("status"); handleStatus(); });
    server.on("/api/diagnostics", HTTP_GET, [this] { server.beginResponse("diagnostics"); handleDiagnostics(); });
    server.on("/api/stations", HTTP_GET, [this] { server.beginResponse("stations"); handleStations(); });
    server.on("/api/stations", HTTP_POST, [this] { server.beginResponse("add_station"); handleAddStation(); });
    server.on("/api/wifi", HTTP_POST, [this] { server.beginResponse("wifi_save"); handleWiFiSave(); });
    server.on("/api/timezone", HTTP_POST, [this] { server.beginResponse("timezone_save"); handleTimezoneSave(); });
    server.onNotFound([this] {
        server.beginResponse("other");
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

void WebPortal::handle() {
    const uint32_t startedMs = millis();
    if (started) server.handleClient();
    if (server.diagnostics.responses != loggedResponseCount) {
        server.finishResponse();
        const auto& r = server.diagnostics;
        Serial.printf("[WEB_RESPONSE] count=%lu route=%s build_ms=%lu header_ms=%lu body_ms=%lu expected=%u sent=%u header_sent=%u calls=%lu waits=%lu error=%d failed=%u overflow=%u heap_start=%u/%u heap_built=%u/%u heap_end=%u/%u alloc_fail=%lu/%lu/%lu\n",
            (unsigned long)r.responses, r.route, (unsigned long)r.buildMs,
            (unsigned long)r.headerMs, (unsigned long)r.bodyMs,
            (unsigned)r.expectedBody, (unsigned)r.sentBody, (unsigned)r.sentHeader,
            (unsigned long)r.writeCalls, (unsigned long)r.retryWaits,
            r.socketError, r.failed, r.jsonOverflow,
            (unsigned)r.freeStart, (unsigned)r.largestStart,
            (unsigned)r.freeBuilt, (unsigned)r.largestBuilt,
            (unsigned)r.freeEnd, (unsigned)r.largestEnd,
            (unsigned long)r.allocStart, (unsigned long)r.allocBuilt,
            (unsigned long)r.allocEnd);
        loggedResponseCount = r.responses;
    }
    if (configPortalActive) dnsServer.processNextRequest();
    lastHandleDurationMs = millis() - startedMs;
    if (lastHandleDurationMs > maxHandleDurationMs) {
        maxHandleDurationMs = lastHandleDurationMs;
    }
    handleCount++;
    lastHandleCompletedMs = millis();
}

void WebPortal::getDiagnostics(WebPortalDiagnostics& diagnostics) const {
    diagnostics.handleCount = handleCount;
    diagnostics.lastHandleDurationMs = lastHandleDurationMs;
    diagnostics.maxHandleDurationMs = maxHandleDurationMs;
    diagnostics.lastHandleCompletedMs = lastHandleCompletedMs;
    diagnostics.response = server.diagnostics;
}

void WebPortal::sendJson(int code, JsonDocument& document) {
    server.diagnostics.jsonOverflow = document.overflowed();
    if (document.overflowed()) {
        sendJsonError(503, "JSON allocation failed");
        return;
    }
    const size_t expectedBytes = measureJson(document);
    server.diagnostics.expectedBody = expectedBytes;
    server.setContentLength(expectedBytes);
    server.send(code, "application/json", "");
    BufferedWebWriter<ObservedWebServer> output(server);
    serializeJson(document, output);
    output.flush();
    const size_t writtenBytes = server.diagnostics.sentBody;
    if (writtenBytes != expectedBytes) {
        Serial.printf("[WEB] JSON response truncated: expected=%u, written=%u\n",
                      (unsigned)expectedBytes, (unsigned)writtenBytes);
    }
}
void WebPortal::sendJsonError(int code, const char* message) {
    String body = String("{\"error\":\"") + message + "\"}";
    server.diagnostics.expectedBody = body.length();
    server.send(code, "application/json", body);
}
void WebPortal::handleRoot() {
    server.diagnostics.expectedBody = sizeof(WEB_PAGE) - 1;
    server.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
}

void WebPortal::handleStatus() {
    JsonDocument doc; doc["connected"] = wifiManager.isConnected(); doc["ssid"] = wifiManager.getSSID(); doc["ip"] = wifiManager.getIP(); doc["portal"] = configPortalActive ? portalSSID : ""; doc["timezone"] = timeService.getTimezone();
    sendJson(200, doc);
}
void WebPortal::handleDiagnostics() {
    AudioDiagnostics audio{};
    bool haveAudioSnapshot = audioPlayer.getDiagnostics(audio);
    WiFiDiagnostics wifi{};
    wifiManager.getDiagnostics(wifi);

    // The full diagnostic schema needs multiple ArduinoJson pools. Keep those
    // non-DMA objects in PSRAM so observing a low-memory audio session does not
    // consume the contiguous INTERNAL|DMA storage needed by the Wi-Fi driver.
    // No internal fallback: overflow retains the existing bounded 503 path.
    JsonDocument doc(&diagnosticsPsramAllocator);
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["build_git_id"] = BUILD_GIT_ID;
    doc["uptime_ms"] = millis();
    const auto rtcpTiming = rtcpServer.timing();
    const auto& rtcpDiscovery = rtcpServer.discoveryDiagnostics();
    const auto rtcpDiagnostic = rtcpServer.diagnostics();
    const auto browserDiagnostic = remoteBrowser.diagnostics();
    doc["rtcp_mdns_active"] = rtcpDiscovery.active;
    doc["rtcp_mdns_starts"] = rtcpDiscovery.starts;
    doc["rtcp_mdns_stops"] = rtcpDiscovery.stops;
    doc["rtcp_mdns_failures"] = rtcpDiscovery.failures;
    doc["rtcp_volume_commands"] = rtcpTiming.commands;
    doc["rtcp_last_apply_us"] = rtcpTiming.lastApplyUs;
    doc["rtcp_max_apply_us"] = rtcpTiming.maxApplyUs;
    doc["rtcp_last_response_us"] = rtcpTiming.lastResponseUs;
    doc["rtcp_max_response_us"] = rtcpTiming.maxResponseUs;
    doc["rtcp_source_last_response_us"] = rtcpTiming.sourceLastResponseUs;
    doc["rtcp_source_max_response_us"] = rtcpTiming.sourceMaxResponseUs;
    doc["rtcp_catalog_last_response_us"] = rtcpTiming.catalogLastResponseUs;
    doc["rtcp_catalog_max_response_us"] = rtcpTiming.catalogMaxResponseUs;
    doc["rtcp_browser_last_response_us"] = rtcpTiming.browserLastResponseUs;
    doc["rtcp_browser_max_response_us"] = rtcpTiming.browserMaxResponseUs;
    doc["rtcp_connections"] = rtcpDiagnostic.connections;
    doc["rtcp_closes"] = rtcpDiagnostic.closes;
    doc["rtcp_close_reason"] = uint8_t(rtcpDiagnostic.close_reason);
    doc["rtcp_transport_error"] = rtcpDiagnostic.transport_error;
    doc["rtcp_request"] = uint8_t(rtcpDiagnostic.request);
    doc["rtcp_request_counter"] = rtcpDiagnostic.request_counter;
    doc["rtcp_request_ms"] = rtcpDiagnostic.request_ms;
    doc["rtcp_response_request"] = uint8_t(rtcpDiagnostic.response_request);
    doc["rtcp_response_counter"] = rtcpDiagnostic.response_counter;
    doc["rtcp_response_ms"] = rtcpDiagnostic.response_ms;
    doc["rtcp_close_history_count"] = rtcpDiagnostic.close_history_count;
    JsonArray closeHistory = doc["rtcp_close_history"].to<JsonArray>();
    for (uint8_t age = 0; age < rtcpDiagnostic.close_history_count &&
                          age < rtcpDiagnostic.close_history.size(); ++age) {
        const uint8_t index = uint8_t((rtcpDiagnostic.close_history_next +
            rtcpDiagnostic.close_history.size() - 1 - age) %
            rtcpDiagnostic.close_history.size());
        const auto& event = rtcpDiagnostic.close_history[index];
        JsonObject row = closeHistory.add<JsonObject>();
        row["uptime_ms"] = event.uptime_ms;
        row["session"] = event.session;
        row["state"] = event.state;
        row["reason"] = uint8_t(event.reason);
        row["socket_error"] = event.socket_error;
        row["request"] = uint8_t(event.request);
        row["request_counter"] = event.request_counter;
        row["request_ms"] = event.request_ms;
        row["response_request"] = uint8_t(event.response_request);
        row["response_counter"] = event.response_counter;
        row["response_ms"] = event.response_ms;
        row["stage"] = uint8_t(event.stage);
    }
    doc["browser_start_requests"] = browserDiagnostic.start_requests;
    doc["browser_start_results"] = browserDiagnostic.start_results;
    doc["browser_page_queries"] = browserDiagnostic.page_queries;
    doc["browser_pages_produced"] = browserDiagnostic.pages_produced;
    doc["browser_jobs_completed"] = browserDiagnostic.jobs_completed;
    doc["browser_jobs_failed"] = browserDiagnostic.jobs_failed;
    doc["browser_cancellations"] = browserDiagnostic.cancellations;
    doc["browser_generation"] = browserDiagnostic.generation;
    doc["browser_revision"] = browserDiagnostic.revision;
    doc["browser_active"] = browserDiagnostic.active;
    doc["browser_job"] = browserDiagnostic.job;
    doc["browser_status"] = browserDiagnostic.status;
    doc["browser_error"] = browserDiagnostic.error;
    doc["browser_cancel_reason"] = uint8_t(browserDiagnostic.cancel_reason);
    doc["browser_mirror"] = browserDiagnostic.mirror;
    doc["browser_transport_stage"] = uint8_t(browserDiagnostic.transport_stage);
    doc["browser_transport_error"] = browserDiagnostic.transport_error;
    doc["browser_first_transport_error"] = browserDiagnostic.first_transport_error;
    doc["browser_socket_errno"] = browserDiagnostic.socket_errno;
    doc["browser_first_socket_errno"] = browserDiagnostic.first_socket_errno;
    doc["browser_final_socket_errno"] = browserDiagnostic.final_socket_errno;
    doc["browser_failed_mirror_count"] = browserDiagnostic.failed_mirror_count;
    doc["browser_attempt_count"] = browserDiagnostic.attempt_count;
    doc["browser_retry_count"] = browserDiagnostic.retry_count;
    doc["browser_retry_waiting"] = browserDiagnostic.retry_waiting;
    doc["browser_mirror_0_elapsed_ms"] = browserDiagnostic.mirror_elapsed_ms[0];
    doc["browser_mirror_1_elapsed_ms"] = browserDiagnostic.mirror_elapsed_ms[1];
    doc["browser_mirror_2_elapsed_ms"] = browserDiagnostic.mirror_elapsed_ms[2];
    doc["browser_http_status"] = browserDiagnostic.http_status;
    doc["browser_first_http_status"] = browserDiagnostic.first_http_status;
    doc["browser_tls_error"] = browserDiagnostic.tls_error;
    doc["browser_tls_flags"] = browserDiagnostic.tls_flags;
    doc["browser_tls_available"] = browserDiagnostic.tls_available;
    doc["browser_tls_system_error"] = browserDiagnostic.tls_system_error;
    doc["browser_tls_mbedtls_error"] = browserDiagnostic.tls_mbedtls_error;
    doc["browser_tls_esp_error"] = browserDiagnostic.tls_esp_error;
    doc["browser_tls_cert_flags"] = browserDiagnostic.tls_cert_flags;
    doc["browser_first_tls_available"] = browserDiagnostic.first_tls_available;
    doc["browser_first_tls_system_error"] = browserDiagnostic.first_tls_system_error;
    doc["browser_first_tls_mbedtls_error"] = browserDiagnostic.first_tls_mbedtls_error;
    doc["browser_first_tls_esp_error"] = browserDiagnostic.first_tls_esp_error;
    doc["browser_first_tls_cert_flags"] = browserDiagnostic.first_tls_cert_flags;
    doc["browser_elapsed_ms"] = browserDiagnostic.elapsed_ms;
    doc["browser_deadline_expired"] = browserDiagnostic.deadline_expired;
    doc["browser_first_failure_stage"] = uint8_t(browserDiagnostic.first_failure_stage);
    doc["browser_first_failure_mirror"] = browserDiagnostic.first_failure_mirror;
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
    doc["wifi_configured_ssid"] = wifi.configuredSsid;
    doc["wifi_ssid"] = wifi.ssid;
    doc["wifi_bssid"] = wifi.bssid;
    doc["wifi_ip"] = wifi.ip;
    doc["wifi_gateway"] = wifi.gateway;
    doc["wifi_subnet"] = wifi.subnet;
    doc["wifi_dns_1"] = wifi.dns1;
    doc["wifi_dns_2"] = wifi.dns2;
    if (wifi.associationInfoAvailable) doc["wifi_channel"] = wifi.channel;
    else doc["wifi_channel"] = nullptr;
    doc["wifi_association_info_available"] = wifi.associationInfoAvailable;
    doc["wifi_association_info_result"] = wifi.associationInfoResult;
    doc["wifi_auto_reconnect"] = wifi.autoReconnect;
    if (wifi.connected) doc["wifi_rssi_dbm"] = wifi.rssi;
    else doc["wifi_rssi_dbm"] = nullptr;
    doc["wifi_disconnect_count"] = wifi.disconnectCount;
    doc["wifi_got_ip_count"] = wifi.gotIpCount;
    doc["wifi_lost_ip_count"] = wifi.lostIpCount;
    doc["wifi_processed_event_count"] = wifi.processedEventCount;
    doc["wifi_dropped_event_count"] = wifi.droppedEventCount;
    doc["wifi_queued_event_count"] = wifi.queuedEventCount;
    if (wifi.hasLastEvent) {
        doc["wifi_last_event"] = wifi.lastEventName;
        doc["wifi_last_event_ms"] = wifi.lastEventMs;
        doc["wifi_last_event_age_ms"] = (uint32_t)(millis() - wifi.lastEventMs);
    } else {
        doc["wifi_last_event"] = "none";
        doc["wifi_last_event_ms"] = nullptr;
        doc["wifi_last_event_age_ms"] = nullptr;
    }
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

    sendJson(200, doc);
}
void WebPortal::handleStations() {
    JsonDocument doc; JsonArray stations = doc["stations"].to<JsonArray>(); JsonArray favorites = doc["favorites"].to<JsonArray>();
    for (uint8_t i=0;i<stationManager.getStationCount();i++) { RadioStation* s=stationManager.getStation(i); JsonObject o=stations.add<JsonObject>(); o["name"]=s->name; o["url"]=s->url; o["codec"]=s->codec==STATION_CODEC_AAC?"AAC":"MP3"; }
    for (uint8_t i=0;i<stationManager.getFavoriteCount();i++) { RadioStation* s=stationManager.getFavorite(i); JsonObject o=favorites.add<JsonObject>(); o["name"]=s->name; o["url"]=s->url; o["codec"]=s->codec==STATION_CODEC_AAC?"AAC":"MP3"; }
    sendJson(200, doc);
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
