#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include "config.h"
#include <WebServer.h>
#include <DNSServer.h>

class WebPortal {
public:
    void begin();
    void beginConfigPortal();
    void handle();
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

    void registerRoutes();
    void sendJsonError(int code, const char* message);
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
