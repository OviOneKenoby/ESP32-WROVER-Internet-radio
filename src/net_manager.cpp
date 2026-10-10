#include "net_manager.h"
#include "config.h"
#include <EEPROM.h>

// Global WiFi manager
WiFiManager wifiManager;
QueueHandle_t WiFiManager::eventQueue = nullptr;
volatile uint32_t WiFiManager::droppedEventCount = 0;

// ============================================
// Constructor
// ============================================
WiFiManager::WiFiManager()
    : currentState(WIFI_DISCONNECTED),
      signalStrength(-100),
      networkCount(0),
      initialized(false),
      connectAttemptInProgress(false),
      eventHandlerId(0),
      lastSignalUpdateMs(0) {
    
    memset(connectedSSID, 0, sizeof(connectedSSID));
    memset(connectedPassword, 0, sizeof(connectedPassword));
    memset(ipAddress, 0, sizeof(ipAddress));
    memset(lastDisconnectReasonName, 0, sizeof(lastDisconnectReasonName));
    strcpy(ipAddress, "0.0.0.0");
    strcpy(lastDisconnectReasonName, "none");
}

WiFiManager::~WiFiManager() {
    if (initialized) {
        WiFi.removeEvent(eventHandlerId);
        initialized = false;
    }
    disconnect();
    if (eventQueue) {
        vQueueDelete(eventQueue);
        eventQueue = nullptr;
    }
}

bool WiFiManager::begin() {
    if (initialized) return true;

    eventQueue = xQueueCreate(12, sizeof(QueuedWiFiEvent));
    if (!eventQueue) {
        Serial.println("[WIFI] Failed to allocate event queue");
        return false;
    }

    droppedEventCount = 0;
    eventHandlerId = WiFi.onEvent(&WiFiManager::wifiEventHandler);
    initialized = true;
    updateStatus();
    Serial.printf("[WIFI] Event tracking ready; framework autoReconnect=%s\n",
                  WiFi.getAutoReconnect() ? "true" : "false");
    return true;
}

void WiFiManager::update() {
    QueuedWiFiEvent event{};
    if (eventQueue) {
        while (xQueueReceive(eventQueue, &event, 0) == pdTRUE) {
            processEvent(event);
        }
    }
    updateStatus();
}

// ============================================
// Connection Management
// ============================================
bool WiFiManager::connect(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) {
        Serial.println("[WIFI] Invalid SSID");
        return false;
    }
    
    // Store credentials
    strncpy(connectedSSID, ssid, MAX_SSID_LENGTH - 1);
    strncpy(connectedPassword, password ? password : "", MAX_PASS_LENGTH - 1);
    connectedSSID[MAX_SSID_LENGTH - 1] = '\0';
    connectedPassword[MAX_PASS_LENGTH - 1] = '\0';
    
    currentState = WIFI_CONNECTING;
    connectAttemptInProgress = true;
    
    Serial.printf("[WIFI] Connecting to: %s\n", ssid);
    
    // Begin WiFi connection
    if (password) {
        WiFi.begin(ssid, password);
    } else {
        WiFi.begin(ssid);
    }
    
    // Wait for connection (with timeout)
    uint32_t timeout = millis() + 20000; // 20 second timeout
    int connectionAttempts = 0;
    
    while (WiFi.status() != WL_CONNECTED &&
           (int32_t)(millis() - timeout) < 0) {
        delay(500);
        update();
        Serial.print(".");
        
        if (++connectionAttempts % 6 == 0) {
            Serial.printf(" %d%%\n", (connectionAttempts * 5));
        }
    }
    
    Serial.println();
    connectAttemptInProgress = false;
    
    if (WiFi.status() == WL_CONNECTED) {
        update();
        
        Serial.printf("[WIFI] Connected!\n");
        Serial.printf("[WIFI] IP: %s\n", ipAddress);
        Serial.printf("[WIFI] Signal: %d dBm\n", signalStrength);
        
        // Save configuration
        saveConfig();
        return true;
    } else {
        currentState = WIFI_ERROR;
        clearLiveNetworkData();
        Serial.println("[WIFI] Connection failed");
        return false;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true); // true = turn off WiFi
    currentState = WIFI_DISCONNECTED;
    connectivity.reconcile(WiFiObservedState::DOWN, millis());
    clearLiveNetworkData();
    Serial.println("[WIFI] Disconnected");
}

void WiFiManager::reconnect() {
    if (strlen(connectedSSID) == 0) {
        Serial.println("[WIFI] No saved credentials");
        return;
    }
    
    Serial.printf("[WIFI] Attempting to reconnect to: %s\n", connectedSSID);
    connect(connectedSSID, connectedPassword);
}

// ============================================
// Network Scanning
// ============================================
bool WiFiManager::startScan() {
    currentState = WIFI_SCANNING;
    
    Serial.println("[WIFI] Starting network scan...");
    
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("[WIFI] No networks found");
        networkCount = 0;
        updateStatus();
        return false;
    }
    
    networkCount = min(n, 20);
    
    Serial.printf("[WIFI] Found %d networks:\n", networkCount);
    
    for (uint8_t i = 0; i < networkCount; i++) {
        strncpy(networks[i].ssid, WiFi.SSID(i).c_str(), MAX_SSID_LENGTH - 1);
        networks[i].ssid[MAX_SSID_LENGTH - 1] = '\0';
        networks[i].rssi = WiFi.RSSI(i);
        Serial.printf("[WIFI] %d. %s (%d dBm)\n", i + 1, networks[i].ssid, networks[i].rssi);
    }

    updateStatus();
    
    return true;
}

void WiFiManager::getNetwork(uint8_t idx, char* ssid, int8_t* rssi) {
    if (idx >= networkCount) {
        strcpy(ssid, "");
        *rssi = -100;
        return;
    }
    
    strcpy(ssid, networks[idx].ssid);
    *rssi = networks[idx].rssi;
}

// ============================================
// Configuration Management
// ============================================
#define EEPROM_SIZE 512
#define EEPROM_ADDR_SSID 0
#define EEPROM_ADDR_PASS 32
#define EEPROM_ADDR_CHECKSUM 96

bool WiFiManager::loadConfig() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Read SSID and password
    char ssid[MAX_SSID_LENGTH];
    char pass[MAX_PASS_LENGTH];
    
    for (int i = 0; i < MAX_SSID_LENGTH; i++) {
        ssid[i] = EEPROM.read(EEPROM_ADDR_SSID + i);
    }
    
    for (int i = 0; i < MAX_PASS_LENGTH; i++) {
        pass[i] = EEPROM.read(EEPROM_ADDR_PASS + i);
    }
    
    // Verify checksum
    uint8_t checksum = EEPROM.read(EEPROM_ADDR_CHECKSUM);
    uint8_t calculated = 0;
    for (int i = 0; i < MAX_SSID_LENGTH; i++) calculated += ssid[i];
    for (int i = 0; i < MAX_PASS_LENGTH; i++) calculated += pass[i];
    
    if (checksum != (calculated & 0xFF)) {
        Serial.println("[WIFI] Config checksum failed");
        EEPROM.end();
        return false;
    }
    
    strncpy(connectedSSID, ssid, MAX_SSID_LENGTH - 1);
    strncpy(connectedPassword, pass, MAX_PASS_LENGTH - 1);
    connectedSSID[MAX_SSID_LENGTH - 1] = '\0';
    connectedPassword[MAX_PASS_LENGTH - 1] = '\0';
    
    Serial.printf("[WIFI] Loaded config: %s\n", connectedSSID);
    
    EEPROM.end();
    return true;
}

bool WiFiManager::saveConfig() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Write SSID
    for (size_t i = 0; i < MAX_SSID_LENGTH; i++) {
        EEPROM.write(EEPROM_ADDR_SSID + i,
                     i < strlen(connectedSSID) ? connectedSSID[i] : 0);
    }
    
    // Write Password
    for (size_t i = 0; i < MAX_PASS_LENGTH; i++) {
        EEPROM.write(EEPROM_ADDR_PASS + i,
                     i < strlen(connectedPassword) ? connectedPassword[i] : 0);
    }
    
    // Calculate and write checksum
    uint8_t checksum = 0;
    for (int i = 0; i < MAX_SSID_LENGTH; i++) checksum += EEPROM.read(EEPROM_ADDR_SSID + i);
    for (int i = 0; i < MAX_PASS_LENGTH; i++) checksum += EEPROM.read(EEPROM_ADDR_PASS + i);
    EEPROM.write(EEPROM_ADDR_CHECKSUM, checksum);
    
    EEPROM.commit();
    EEPROM.end();
    
    Serial.printf("[WIFI] Config saved: %s\n", connectedSSID);
    return true;
}

// ============================================
// Status Updates
// ============================================
void WiFiManager::updateStatus() {
    const uint32_t now = millis();
    const uint8_t frameworkStatus = WiFi.status();
    WiFiObservedState observedState = WiFiObservedState::DOWN;

    if (frameworkStatus == WL_CONNECTED) {
        observedState = WiFiObservedState::CONNECTED;
        const IPAddress ip = WiFi.localIP();
        snprintf(ipAddress, sizeof(ipAddress), "%u.%u.%u.%u",
                 ip[0], ip[1], ip[2], ip[3]);
        if (lastSignalUpdateMs == 0 || now - lastSignalUpdateMs >= 5000) {
            signalStrength = WiFi.RSSI();
            lastSignalUpdateMs = now;
        }
    } else {
        if (frameworkStatus == WL_IDLE_STATUS || connectAttemptInProgress) {
            observedState = WiFiObservedState::CONNECTING;
        }
        clearLiveNetworkData();
    }

    const WiFiObservedState previous = connectivity.get().state;
    connectivity.reconcile(observedState, now);
    setCurrentStateFromModel();

    if (frameworkStatus == WL_CONNECT_FAILED ||
        frameworkStatus == WL_NO_SSID_AVAIL) {
        currentState = WIFI_ERROR;
    }

    if (previous != connectivity.get().state) {
        Serial.printf("[WIFI] Framework state synchronized: %s, status=%s, ip=%s\n",
                      stateName(currentState),
                      frameworkStatusName(frameworkStatus), getIP());
    }
}

void WiFiManager::processEvent(const QueuedWiFiEvent& event) {
    connectivity.apply(event.type, event.reason, event.eventMs);
    setCurrentStateFromModel();

    switch (event.type) {
        case WiFiObservedEvent::STA_STARTED:
            Serial.printf("[WIFI] Event STA_START at %lu ms\n",
                          (unsigned long)event.eventMs);
            break;

        case WiFiObservedEvent::STA_ASSOCIATED:
            Serial.printf("[WIFI] Event STA_CONNECTED at %lu ms; awaiting IP\n",
                          (unsigned long)event.eventMs);
            break;

        case WiFiObservedEvent::STA_DISCONNECTED: {
            const char* reasonName =
                WiFi.disconnectReasonName((wifi_err_reason_t)event.reason);
            strncpy(lastDisconnectReasonName,
                    reasonName ? reasonName : "unknown",
                    sizeof(lastDisconnectReasonName) - 1);
            lastDisconnectReasonName[sizeof(lastDisconnectReasonName) - 1] = '\0';
            clearLiveNetworkData();
            Serial.printf(
                "[WIFI] Event STA_DISCONNECTED at %lu ms: reason=%u (%s), autoReconnect=%s\n",
                (unsigned long)event.eventMs, event.reason,
                lastDisconnectReasonName,
                WiFi.getAutoReconnect() ? "true" : "false");
            break;
        }

        case WiFiObservedEvent::STA_GOT_IP:
            Serial.printf("[WIFI] Event STA_GOT_IP at %lu ms\n",
                          (unsigned long)event.eventMs);
            break;

        case WiFiObservedEvent::STA_LOST_IP:
            clearLiveNetworkData();
            Serial.printf("[WIFI] Event STA_LOST_IP at %lu ms\n",
                          (unsigned long)event.eventMs);
            break;

        case WiFiObservedEvent::STA_STOPPED:
            clearLiveNetworkData();
            Serial.printf("[WIFI] Event STA_STOP at %lu ms\n",
                          (unsigned long)event.eventMs);
            break;
    }
}

void WiFiManager::setCurrentStateFromModel() {
    switch (connectivity.get().state) {
        case WiFiObservedState::CONNECTED:
            currentState = WIFI_CONNECTED;
            break;
        case WiFiObservedState::CONNECTING:
            currentState = WIFI_CONNECTING;
            break;
        case WiFiObservedState::DOWN:
        default:
            currentState = WIFI_DISCONNECTED;
            break;
    }
}

void WiFiManager::clearLiveNetworkData() {
    strcpy(ipAddress, "0.0.0.0");
    signalStrength = -100;
    lastSignalUpdateMs = 0;
}

void WiFiManager::getDiagnostics(WiFiDiagnostics& diagnostics) const {
    const WiFiConnectivitySnapshot& snapshot = connectivity.get();
    diagnostics.frameworkStatus = WiFi.status();
    diagnostics.connected = diagnostics.frameworkStatus == WL_CONNECTED;
    if (diagnostics.connected) {
        diagnostics.state = WIFI_CONNECTED;
    } else if (diagnostics.frameworkStatus == WL_IDLE_STATUS) {
        diagnostics.state = WIFI_CONNECTING;
    } else if (diagnostics.frameworkStatus == WL_CONNECT_FAILED ||
               diagnostics.frameworkStatus == WL_NO_SSID_AVAIL) {
        diagnostics.state = WIFI_ERROR;
    } else {
        // Never expose a cached CONNECTED state after the framework has
        // already lost the link, even in the small interval before the main
        // loop drains the queued event.
        diagnostics.state = WIFI_DISCONNECTED;
    }
    diagnostics.autoReconnect = WiFi.getAutoReconnect();
    strncpy(diagnostics.ssid, connectedSSID, sizeof(diagnostics.ssid) - 1);
    diagnostics.ssid[sizeof(diagnostics.ssid) - 1] = '\0';
    if (diagnostics.connected) {
        const IPAddress ip = WiFi.localIP();
        snprintf(diagnostics.ip, sizeof(diagnostics.ip), "%u.%u.%u.%u",
                 ip[0], ip[1], ip[2], ip[3]);
        diagnostics.rssi = WiFi.RSSI();
    } else {
        strcpy(diagnostics.ip, "0.0.0.0");
        diagnostics.rssi = -100;
    }
    diagnostics.lastDisconnectReason = snapshot.lastDisconnectReason;
    strncpy(diagnostics.lastDisconnectReasonName, lastDisconnectReasonName,
            sizeof(diagnostics.lastDisconnectReasonName) - 1);
    diagnostics.lastDisconnectReasonName[
        sizeof(diagnostics.lastDisconnectReasonName) - 1] = '\0';
    diagnostics.disconnectCount = snapshot.disconnectCount;
    diagnostics.gotIpCount = snapshot.gotIpCount;
    diagnostics.lostIpCount = snapshot.lostIpCount;
    diagnostics.processedEventCount = snapshot.processedEventCount;
    diagnostics.droppedEventCount = droppedEventCount;
    diagnostics.stateSinceMs = snapshot.stateSinceMs;
    diagnostics.connectedSinceMs = snapshot.connectedSinceMs;
    diagnostics.lastDisconnectMs = snapshot.lastDisconnectMs;
    diagnostics.lastGotIpMs = snapshot.lastGotIpMs;
    diagnostics.lastLostIpMs = snapshot.lastLostIpMs;
    diagnostics.hasConnectedSince = snapshot.hasConnectedSince;
    diagnostics.hasDisconnected = snapshot.hasDisconnected;
    diagnostics.hasGotIp = snapshot.hasGotIp;
    diagnostics.hasLostIp = snapshot.hasLostIp;
}

const char* WiFiManager::stateName(WiFiState state) {
    switch (state) {
        case WIFI_SCANNING: return "scanning";
        case WIFI_CONNECTING: return "connecting";
        case WIFI_CONNECTED: return "connected";
        case WIFI_ERROR: return "error";
        case WIFI_DISCONNECTED:
        default: return "disconnected";
    }
}

const char* WiFiManager::frameworkStatusName(uint8_t status) {
    switch (status) {
        case WL_IDLE_STATUS: return "idle";
        case WL_NO_SSID_AVAIL: return "no_ssid";
        case WL_SCAN_COMPLETED: return "scan_complete";
        case WL_CONNECTED: return "connected";
        case WL_CONNECT_FAILED: return "connect_failed";
        case WL_CONNECTION_LOST: return "connection_lost";
        case WL_DISCONNECTED: return "disconnected";
        case WL_NO_SHIELD: return "station_stopped";
        default: return "unknown";
    }
}

void WiFiManager::wifiEventHandler(arduino_event_t* event) {
    if (!event || !eventQueue) return;

    QueuedWiFiEvent queued{};
    queued.eventMs = millis();
    queued.reason = 0;

    switch (event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_START:
            queued.type = WiFiObservedEvent::STA_STARTED;
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            queued.type = WiFiObservedEvent::STA_ASSOCIATED;
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            queued.type = WiFiObservedEvent::STA_DISCONNECTED;
            queued.reason = event->event_info.wifi_sta_disconnected.reason;
            if (queued.reason == 0) queued.reason = WIFI_REASON_UNSPECIFIED;
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            queued.type = WiFiObservedEvent::STA_GOT_IP;
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            queued.type = WiFiObservedEvent::STA_LOST_IP;
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            queued.type = WiFiObservedEvent::STA_STOPPED;
            break;
        default:
            return;
    }

    // Arduino Wi-Fi callbacks run on the framework event task. Keep this
    // callback bounded: copy only POD data, never log, connect, touch UI or
    // flash. The main loop owns all state transitions and diagnostics.
    if (xQueueSend(eventQueue, &queued, 0) != pdTRUE) {
        droppedEventCount++;
    }
}
