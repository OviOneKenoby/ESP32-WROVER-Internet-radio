#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include "config.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

struct WebPortalDiagnostics {
    uint32_t handleCount;
    uint32_t lastHandleDurationMs;
    uint32_t maxHandleDurationMs;
    uint32_t lastHandleCompletedMs;
};

class WebPortal {
public:
    void begin();
    void beginConfigPortal();
    void handle();
    void getDiagnostics(WebPortalDiagnostics& diagnostics) const;
    bool isConfigPortalActive() const { return configPortalActive; }
    const char* getPortalSSID() const { return portalSSID; }
    const char* getPortalPassword() const { return portalPassword; }

private:
    WebServer server{WEB_SERVER_PORT};
    DNSServer dnsServer;
    bool started = false;
    bool configPortalActive = false;
    char portalSSID[33] = "";
    char portalPassword[17] = "";
    uint32_t handleCount = 0;
    uint32_t lastHandleDurationMs = 0;
    uint32_t maxHandleDurationMs = 0;
    uint32_t lastHandleCompletedMs = 0;

    void registerRoutes();
    void sendJsonError(int code, const char* message);
    void sendJson(int code, JsonDocument& document);
    void handleRoot();
    void handleStatus();
    void handleDiagnostics();
    void handleStations();
    void handleAddStation();
    void handleDeleteStation();
    void handleDeleteFavorite();
    void handleWiFiSave();
    void handleTimezoneSave();
};

extern WebPortal webPortal;

#endif
