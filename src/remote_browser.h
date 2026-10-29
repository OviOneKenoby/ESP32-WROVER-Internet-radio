#pragma once

#include "config.h"
#include "rtcp_browser.h"
#include "remote_browser_diagnostics.h"
#include "station_codec.h"
#include <ArduinoJson.h>
#include <esp_http_client.h>
#include <cstddef>
#include <cstdint>

class RemoteBrowserService {
public:
    bool start(uint32_t now_ms);
    void poll(uint32_t now_ms);
    void cancel(RemoteBrowserCancelReason reason = RemoteBrowserCancelReason::remote_command);
    bool view(const rtcp_browser::Query& query, rtcp_browser::StateView& out) const;
    uint8_t execute(const rtcp_browser::Command& command, uint32_t now_ms);
    bool takePlay(uint16_t token, char* name, size_t name_size,
                  char* url, size_t url_size, StationCodec& codec) const;
    uint64_t generation() const { return generation_; }
    uint64_t revision() const { return revision_; }
    bool active() const { return active_; }
    RemoteBrowserDiagnostics diagnostics() const;

private:
    struct PsramAllocator final : ArduinoJson::Allocator {
        void* allocate(size_t size) override;
        void deallocate(void* pointer) override;
        void* reallocate(void* pointer, size_t size) override;
    } allocator_;
    struct Country { char name[BROWSE_COUNTRY_NAME_LEN]; char code[BROWSE_COUNTRY_CODE_LEN]; };
    struct Genre { char name[BROWSE_TAG_NAME_LEN]; };
    struct Result { char name[MAX_NAME_LENGTH]; char url[MAX_URL_LENGTH]; StationCodec codec; };
    enum class Job : uint8_t { none, countries, genres, results };

    Country* countries_{nullptr}; Genre* genres_{nullptr}; Result* results_{nullptr};
    uint8_t* body_{nullptr}; size_t body_size_{0};
    uint16_t country_count_{0}; uint8_t genre_count_{0}, result_count_{0};
    rtcp_browser::Status country_status_{rtcp_browser::Status::idle};
    rtcp_browser::Status genre_status_{rtcp_browser::Status::idle};
    rtcp_browser::Status result_status_{rtcp_browser::Status::idle};
    uint8_t country_error_{0}, genre_error_{0}, result_error_{0};
    uint64_t generation_{0}, revision_{0};
    bool active_{false};
    uint16_t selected_country_{rtcp_browser::kAnyToken};
    uint16_t selected_genre_{rtcp_browser::kAnyToken};
    uint8_t selected_name_{0};
    Job job_{Job::none}; esp_http_client_handle_t client_{nullptr};
    uint32_t job_started_ms_{0}, job_deadline_ms_{0}, mirror_started_ms_{0}; uint8_t mirror_{0}; bool body_overflow_{false};
    char path_[512]{};
    mutable RemoteBrowserDiagnostics diagnostics_{};

    static esp_err_t onHttpEvent(esp_http_client_event_t* event);
    bool allocateStorage(); void releaseHttp();
    bool beginJob(Job job, const char* path, uint32_t now_ms);
    bool beginMirror(uint32_t now_ms);
    bool parseJob(); void finishJob(bool success, uint8_t error);
    static bool encode(const char* input, char* output, size_t capacity);
    void advanceRevision();
};

extern RemoteBrowserService remoteBrowser;

