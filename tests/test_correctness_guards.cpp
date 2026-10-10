#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "correctness_guards.h"
#include "current_station_context.h"
#include "station_codec.h"
#include "wifi_connectivity_model.h"

namespace {

int checks = 0;

void expect(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

bool deleteBySuffix(std::vector<int>& items, const std::string& suffix) {
    uint8_t index = 0xA5;
    if (!CorrectnessGuards::parseListIndex(
            suffix.c_str(), suffix.size(), items.size(), index)) {
        return false;
    }
    items.erase(items.begin() + index);
    return true;
}

void testDeleteParsingAndMutation() {
    const std::vector<std::string> invalid = {
        "", "+1", "-1", " 1", "1 ", "1x", "x1", "0000", "99999999999999999999", "256"
    };
    for (const auto& suffix : invalid) {
        std::vector<int> items = {10, 20, 30};
        const auto before = items;
        expect(!deleteBySuffix(items, suffix), "invalid delete suffix accepted");
        expect(items == before, "rejected delete mutated the list");
    }

    uint8_t index = 17;
    expect(!CorrectnessGuards::parseListIndex("0", 1, 0, index),
           "empty list accepted index zero");
    expect(!CorrectnessGuards::parseListIndex("3", 1, 3, index),
           "actual-count upper boundary accepted");
    expect(!CorrectnessGuards::parseListIndex("255", 3, 255, index),
           "index equal to actual count accepted");
    expect(CorrectnessGuards::parseListIndex("255", 3, 256, index) && index == 255,
           "valid uint8 maximum rejected");

    std::vector<int> items = {10, 20, 30};
    expect(deleteBySuffix(items, "1"), "valid deletion rejected");
    expect(items == std::vector<int>({10, 30}), "valid deletion removed wrong item");
    expect(deleteBySuffix(items, "0"), "zero-index deletion rejected");
    expect(items == std::vector<int>({30}), "zero-index deletion removed wrong item");
}

void testTraversal() {
    uint8_t result = 99;
    expect(!CorrectnessGuards::nextStationIndex(0, 0, result),
           "next accepted empty list");
    expect(!CorrectnessGuards::previousStationIndex(0, 0, result),
           "previous accepted empty list");
    expect(CorrectnessGuards::nextStationIndex(0, 1, result) && result == 0,
           "one-item next did not remain at zero");
    expect(CorrectnessGuards::previousStationIndex(0, 1, result) && result == 0,
           "one-item previous did not remain at zero");
    expect(CorrectnessGuards::nextStationIndex(0, 3, result) && result == 1,
           "many-item next failed");
    expect(CorrectnessGuards::nextStationIndex(2, 3, result) && result == 0,
           "many-item next wrap failed");
    expect(CorrectnessGuards::previousStationIndex(0, 3, result) && result == 2,
           "many-item previous wrap failed");
    expect(CorrectnessGuards::previousStationIndex(2, 3, result) && result == 1,
           "many-item previous failed");
    expect(!CorrectnessGuards::nextStationIndex(3, 3, result),
           "stale current index accepted");
}

void testVolume() {
    for (uint16_t current = 0; current <= 100; ++current) {
        const uint8_t expected = current > 5 ? (uint8_t)(current - 5) : 0;
        const uint8_t actual = CorrectnessGuards::volumeDown((uint8_t)current);
        expect(actual == expected, "volume-down saturation mismatch");
        expect(actual <= current, "volume-down increased volume");
    }
    for (uint8_t current = 1; current <= 4; ++current) {
        expect(CorrectnessGuards::volumeDown(current) == 0,
               "volume 1..4 did not saturate to zero");
    }
    expect(CorrectnessGuards::volumeDown(0) == 0,
           "zero volume wrapped");
}

void testFavoriteCodecPropagation() {
    CurrentStationContext context;
    const char* opaqueAAC = "https://example.invalid/live?id=opaque";
    context.set(STATION_CODEC_AAC, true);
    expect(context.discovered, "discovered context not retained");
    expect(stationCodecForURL(opaqueAAC, context.codec) == STATION_CODEC_AAC,
           "opaque AAC codec was replaced by URL inference");

    const uint8_t persisted = (uint8_t)stationCodecForURL(opaqueAAC, context.codec);
    const StationCodec reloaded = stationCodecForURL(opaqueAAC, (StationCodec)persisted);
    expect(reloaded == STATION_CODEC_AAC, "opaque AAC codec did not survive reload");

    context.set(STATION_CODEC_MP3, true);
    expect(stationCodecForURL("https://example.invalid/mp3", context.codec) == STATION_CODEC_MP3,
           "opaque MP3 codec changed");
    expect(stationCodecForURL("https://example.invalid/LIVE.AAC?x=1", context.codec) == STATION_CODEC_AAC,
           "legacy .aac URL upgrade failed");
}

void testWiFiConnectivityTransitions() {
    WiFiConnectivityModel model;
    auto snapshot = model.get();
    expect(snapshot.state == WiFiObservedState::DOWN,
           "Wi-Fi model did not start down");

    model.apply(WiFiObservedEvent::STA_STARTED, 0, 10);
    expect(model.get().state == WiFiObservedState::CONNECTING,
           "STA_START did not enter connecting");
    model.apply(WiFiObservedEvent::STA_ASSOCIATED, 0, 20);
    expect(model.get().state == WiFiObservedState::CONNECTING,
           "association incorrectly implied an IP connection");

    model.apply(WiFiObservedEvent::STA_GOT_IP, 0, 30);
    snapshot = model.get();
    expect(snapshot.state == WiFiObservedState::CONNECTED,
           "GOT_IP did not enter connected");
    expect(snapshot.gotIpCount == 1 && snapshot.lastGotIpMs == 30,
           "GOT_IP metrics mismatch");
    expect(snapshot.hasConnectedSince && snapshot.connectedSinceMs == 30,
           "connected-since timestamp mismatch");

    model.apply(WiFiObservedEvent::STA_LOST_IP, 0, 40);
    snapshot = model.get();
    expect(snapshot.state == WiFiObservedState::CONNECTING,
           "LOST_IP left stale connected state");
    expect(!snapshot.hasConnectedSince && snapshot.lostIpCount == 1 &&
               snapshot.lastLostIpMs == 40,
           "LOST_IP metrics mismatch");

    model.apply(WiFiObservedEvent::STA_DISCONNECTED, 201, 50);
    snapshot = model.get();
    expect(snapshot.state == WiFiObservedState::DOWN,
           "disconnect did not enter down state");
    expect(snapshot.disconnectCount == 1 &&
               snapshot.lastDisconnectReason == 201 &&
               snapshot.lastDisconnectMs == 50,
           "disconnect metrics mismatch");

    model.apply(WiFiObservedEvent::STA_GOT_IP, 0, 80);
    expect(model.get().gotIpCount == 2 &&
               model.get().state == WiFiObservedState::CONNECTED,
           "recovery GOT_IP was not recorded");

    const uint32_t disconnectsBeforeReconcile = model.get().disconnectCount;
    const uint32_t gotIpBeforeReconcile = model.get().gotIpCount;
    model.reconcile(WiFiObservedState::DOWN, 90);
    expect(model.get().state == WiFiObservedState::DOWN,
           "framework reconciliation did not correct stale connected state");
    expect(model.get().disconnectCount == disconnectsBeforeReconcile &&
               model.get().gotIpCount == gotIpBeforeReconcile,
           "framework reconciliation invented event counters");

    model.reconcile(WiFiObservedState::CONNECTED, 100);
    snapshot = model.get();
    expect(snapshot.state == WiFiObservedState::CONNECTED &&
               snapshot.hasConnectedSince &&
               snapshot.connectedSinceMs == 100,
           "framework recovery reconciliation mismatch");
    expect(snapshot.processedEventCount == 6,
           "processed Wi-Fi event count mismatch");
}

} // namespace

int main() {
    testDeleteParsingAndMutation();
    testTraversal();
    testVolume();
    testFavoriteCodecPropagation();
    testWiFiConnectivityTransitions();
    std::cout << "PASS: " << checks << " deterministic correctness checks\n";
    return 0;
}
