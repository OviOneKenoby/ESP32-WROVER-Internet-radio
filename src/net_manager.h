#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

enum WiFiState {
    WIFI_DISCONNECTED,
    WIFI_SCANNING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR
};

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();
    
    // Connection management
    bool connect(const char* ssid, const char* password);
    void disconnect();
    void reconnect();
    
    // State queries
    WiFiState getState() { return currentState; }
    bool isConnected() { return currentState == WIFI_CONNECTED; }
    const char* getSSID() { return connectedSSID; }
    const char* getIP() { return ipAddress; }
    int16_t getSignal() { return signalStrength; }
    
    // Network scanning
    bool startScan();
    uint8_t getNetworkCount() { return networkCount; }
    void getNetwork(uint8_t idx, char* ssid, int8_t* rssi);
    
    // Configuration
    bool loadConfig();
    bool saveConfig();
    
private:
    WiFiState currentState;
    char connectedSSID[MAX_SSID_LENGTH];
    char connectedPassword[MAX_PASS_LENGTH];
    char ipAddress[16];
    int16_t signalStrength;
    
    // Network list
    struct Network {
        char ssid[MAX_SSID_LENGTH];
        int8_t rssi;
    };
    Network networks[20];
    uint8_t networkCount;
    
    // Status update
    void updateStatus();
    
    // WiFi event handler
    static void wifiEventHandler(WiFiEvent_t event);
};

extern WiFiManager wifiManager;

#endif // NET_MANAGER_H
