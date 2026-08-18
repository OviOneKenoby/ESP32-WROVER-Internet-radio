#include "net_manager.h"
#include "config.h"
#include <EEPROM.h>

// Global WiFi manager
WiFiManager wifiManager;

// ============================================
// Constructor
// ============================================
WiFiManager::WiFiManager()
    : currentState(WIFI_DISCONNECTED),
      signalStrength(-100),
      networkCount(0) {
    
    memset(connectedSSID, 0, sizeof(connectedSSID));
    memset(connectedPassword, 0, sizeof(connectedPassword));
    memset(ipAddress, 0, sizeof(ipAddress));
    strcpy(ipAddress, "0.0.0.0");
}

WiFiManager::~WiFiManager() {
    disconnect();
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
    
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(500);
        Serial.print(".");
        
        if (++connectionAttempts % 6 == 0) {
            Serial.printf(" %d%%\n", (connectionAttempts * 5));
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        currentState = WIFI_CONNECTED;
        strncpy(ipAddress, WiFi.localIP().toString().c_str(), sizeof(ipAddress) - 1);
        ipAddress[sizeof(ipAddress) - 1] = '\0';
        signalStrength = WiFi.RSSI();
        
        Serial.printf("[WIFI] Connected!\n");
        Serial.printf("[WIFI] IP: %s\n", ipAddress);
        Serial.printf("[WIFI] Signal: %d dBm\n", signalStrength);
        
        // Save configuration
        saveConfig();
        return true;
    } else {
        currentState = WIFI_ERROR;
        Serial.println("[WIFI] Connection failed");
        return false;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true); // true = turn off WiFi
    currentState = WIFI_DISCONNECTED;
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
    if (WiFi.status() == WL_CONNECTED) {
        signalStrength = WiFi.RSSI();
    }
}

void WiFiManager::wifiEventHandler(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_START:
            Serial.println("[WIFI] WiFi started");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("[WIFI] WiFi connected to AP");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.printf("[WIFI] Got IP: %s\n", WiFi.localIP().toString().c_str());
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("[WIFI] Disconnected from AP");
            break;
        default:
            break;
    }
}
