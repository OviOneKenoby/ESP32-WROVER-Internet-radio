#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"
#include "wifi_connectivity_model.h"

enum WiFiState {
    WIFI_DISCONNECTED,
    WIFI_SCANNING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR
};

struct WiFiDiagnostics {
    WiFiState state;
    uint8_t frameworkStatus;
    bool connected;
    bool autoReconnect;
    char ssid[MAX_SSID_LENGTH];
    char ip[16];
    int16_t rssi;
    uint8_t lastDisconnectReason;
    char lastDisconnectReasonName[48];
    uint32_t disconnectCount;
    uint32_t gotIpCount;
    uint32_t lostIpCount;
    uint32_t processedEventCount;
    uint32_t droppedEventCount;
    uint32_t stateSinceMs;
    uint32_t connectedSinceMs;
    uint32_t lastDisconnectMs;
    uint32_t lastGotIpMs;
    uint32_t lastLostIpMs;
    bool hasConnectedSince;
    bool hasDisconnected;
    bool hasGotIp;
    bool hasLostIp;
};

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    bool begin();
    void update();
    
    // Connection management
    bool connect(const char* ssid, const char* password);
    void disconnect();
    void reconnect();
    
    // State queries
    WiFiState getState() const { return currentState; }
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    const char* getSSID() const { return connectedSSID; }
    const char* getIP() const {
        return isConnected() ? ipAddress : "0.0.0.0";
    }
    int16_t getSignal() const { return isConnected() ? signalStrength : -100; }
    void getDiagnostics(WiFiDiagnostics& diagnostics) const;
    static const char* stateName(WiFiState state);
    static const char* frameworkStatusName(uint8_t status);
    
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

    struct QueuedWiFiEvent {
        WiFiObservedEvent type;
        uint8_t reason;
        uint32_t eventMs;
    };

    bool initialized;
    bool connectAttemptInProgress;
    wifi_event_id_t eventHandlerId;
    uint32_t lastSignalUpdateMs;
    char lastDisconnectReasonName[48];
    WiFiConnectivityModel connectivity;

    static QueueHandle_t eventQueue;
    static volatile uint32_t droppedEventCount;
    
    // Status update
    void updateStatus();
    void processEvent(const QueuedWiFiEvent& event);
    void setCurrentStateFromModel();
    void clearLiveNetworkData();
    
    // WiFi event handler
    static void wifiEventHandler(arduino_event_t* event);
};

extern WiFiManager wifiManager;

#endif // NET_MANAGER_H
