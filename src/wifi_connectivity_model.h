#ifndef WIFI_CONNECTIVITY_MODEL_H
#define WIFI_CONNECTIVITY_MODEL_H

#include <cstdint>

enum class WiFiObservedState : uint8_t {
    DOWN,
    CONNECTING,
    CONNECTED
};

enum class WiFiObservedEvent : uint8_t {
    STA_STARTED,
    STA_ASSOCIATED,
    STA_DISCONNECTED,
    STA_GOT_IP,
    STA_LOST_IP,
    STA_STOPPED
};

struct WiFiConnectivitySnapshot {
    WiFiObservedState state = WiFiObservedState::DOWN;
    uint32_t stateSinceMs = 0;
    uint32_t connectedSinceMs = 0;
    uint32_t lastDisconnectMs = 0;
    uint32_t lastGotIpMs = 0;
    uint32_t lastLostIpMs = 0;
    uint32_t disconnectCount = 0;
    uint32_t gotIpCount = 0;
    uint32_t lostIpCount = 0;
    uint32_t processedEventCount = 0;
    uint8_t lastDisconnectReason = 0;
    bool hasConnectedSince = false;
    bool hasDisconnected = false;
    bool hasGotIp = false;
    bool hasLostIp = false;
};

class WiFiConnectivityModel {
public:
    void apply(WiFiObservedEvent event, uint8_t reason, uint32_t eventMs) {
        snapshot.processedEventCount++;
        switch (event) {
            case WiFiObservedEvent::STA_STARTED:
            case WiFiObservedEvent::STA_ASSOCIATED:
                setState(WiFiObservedState::CONNECTING, eventMs);
                break;

            case WiFiObservedEvent::STA_DISCONNECTED:
                snapshot.disconnectCount++;
                snapshot.lastDisconnectReason = reason;
                snapshot.lastDisconnectMs = eventMs;
                snapshot.hasDisconnected = true;
                snapshot.hasConnectedSince = false;
                setState(WiFiObservedState::DOWN, eventMs);
                break;

            case WiFiObservedEvent::STA_GOT_IP:
                snapshot.gotIpCount++;
                snapshot.lastGotIpMs = eventMs;
                snapshot.hasGotIp = true;
                snapshot.connectedSinceMs = eventMs;
                snapshot.hasConnectedSince = true;
                setState(WiFiObservedState::CONNECTED, eventMs);
                break;

            case WiFiObservedEvent::STA_LOST_IP:
                snapshot.lostIpCount++;
                snapshot.lastLostIpMs = eventMs;
                snapshot.hasLostIp = true;
                snapshot.hasConnectedSince = false;
                setState(WiFiObservedState::CONNECTING, eventMs);
                break;

            case WiFiObservedEvent::STA_STOPPED:
                snapshot.hasConnectedSince = false;
                setState(WiFiObservedState::DOWN, eventMs);
                break;
        }
    }

    // Reconciliation covers missed/overflowed events and makes the exposed
    // state follow the framework's current status. It deliberately does not
    // invent disconnect/GOT_IP events or increment their counters.
    void reconcile(WiFiObservedState actualState, uint32_t nowMs) {
        if (actualState == WiFiObservedState::CONNECTED &&
            snapshot.state != WiFiObservedState::CONNECTED) {
            snapshot.connectedSinceMs = nowMs;
            snapshot.hasConnectedSince = true;
        } else if (actualState != WiFiObservedState::CONNECTED) {
            snapshot.hasConnectedSince = false;
        }
        setState(actualState, nowMs);
    }

    const WiFiConnectivitySnapshot& get() const { return snapshot; }

private:
    void setState(WiFiObservedState state, uint32_t nowMs) {
        if (snapshot.state == state) return;
        snapshot.state = state;
        snapshot.stateSinceMs = nowMs;
    }

    WiFiConnectivitySnapshot snapshot;
};

#endif
