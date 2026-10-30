#include "remote_browser.h"
#include "remote_browser_transport_policy.h"

#include "audio.h"
#include "browser.h"
#include "stations.h"
#include <WiFi.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_tls.h>
#include <esp_tls_errors.h>
#include <cstring>

// Arduino-ESP32 2.0.17 ships a private esp_crt_bundle.h with the same include
// guard and basename as ESP-IDF 4.4.7's native header. PlatformIO resolves the
// Arduino header first for this translation unit, so declare the exact native
// ESP-IDF callback signature here and bind the IDF-owned certificate bundle.
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);

namespace {
constexpr size_t kBodyCapacity = 32768;
constexpr uint8_t kMirrorLimit = RADIO_BROWSER_MIRROR_COUNT;
static_assert(RADIO_BROWSER_MIRROR_COUNT == RemoteBrowserDiagnostics::kMirrorSlots,
              "browser diagnostic mirror slots must match configured mirrors");
}

RemoteBrowserService remoteBrowser;

void* RemoteBrowserService::PsramAllocator::allocate(size_t size) {
    return size ? heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : nullptr;
}
void RemoteBrowserService::PsramAllocator::deallocate(void* pointer) { heap_caps_free(pointer); }
void* RemoteBrowserService::PsramAllocator::reallocate(void* pointer, size_t size) {
    if (!size) { heap_caps_free(pointer); return nullptr; }
    return heap_caps_realloc(pointer, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

bool RemoteBrowserService::allocateStorage() {
    if (!psramFound()) return false;
    if (!countries_) countries_ = static_cast<Country*>(heap_caps_calloc(
        MAX_BROWSE_COUNTRIES, sizeof(Country), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!genres_) genres_ = static_cast<Genre*>(heap_caps_calloc(
        MAX_BROWSE_TAGS, sizeof(Genre), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!results_) results_ = static_cast<Result*>(heap_caps_calloc(
        MAX_BROWSE_RESULTS, sizeof(Result), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!body_) body_ = static_cast<uint8_t*>(heap_caps_malloc(
        kBodyCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    return countries_ && genres_ && results_ && body_;
}

void RemoteBrowserService::advanceRevision() {
    if (revision_ != UINT64_MAX) ++revision_;
    if (!revision_) revision_ = 1;
}

bool RemoteBrowserService::start(uint32_t now_ms) {
    cancel(RemoteBrowserCancelReason::restart);
    if (WiFi.status() != WL_CONNECTED || !allocateStorage()) return false;
    if (generation_ != UINT64_MAX) ++generation_; else generation_ = 1;
    if (!generation_) generation_ = 1;
    active_ = true; revision_ = 1; country_count_ = 0; genre_count_ = result_count_ = 0;
    country_status_ = rtcp_browser::Status::loading;
    genre_status_ = result_status_ = rtcp_browser::Status::idle;
    country_error_ = genre_error_ = result_error_ = 0;
    selected_country_ = selected_genre_ = rtcp_browser::kAnyToken;
    selected_name_ = 0;
    diagnostics_.active = 1;
    diagnostics_.generation = generation_;
    diagnostics_.revision = revision_;
    return beginJob(Job::countries,
        "/json/countries?order=name&hidebroken=true&limit=250", now_ms);
}

void RemoteBrowserService::releaseHttp() {
    if (client_) { esp_http_client_cleanup(client_); client_ = nullptr; }
    body_size_ = 0; body_overflow_ = false;
}

void RemoteBrowserService::cancel(RemoteBrowserCancelReason reason) {
    if (active_ || job_ != Job::none) {
        RemoteBrowserDiagnostics::increment(diagnostics_.cancellations);
        diagnostics_.cancel_reason = reason;
    }
    releaseHttp(); job_ = Job::none; retry_waiting_ = false;
    country_status_ = genre_status_ = result_status_ = rtcp_browser::Status::idle;
    country_count_ = 0; genre_count_ = result_count_ = 0;
    active_ = false; revision_ = 0;
    diagnostics_.active = 0;
    diagnostics_.job = 0;
    diagnostics_.status = uint8_t(rtcp_browser::Status::idle);
    diagnostics_.error = 0;
    diagnostics_.revision = 0;
}

esp_err_t RemoteBrowserService::onHttpEvent(esp_http_client_event_t* event) {
    auto* self = static_cast<RemoteBrowserService*>(event->user_data);
    if (!self) return ESP_OK;
    switch (event->event_id) {
        case HTTP_EVENT_ERROR:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::event_error;
            self->diagnostics_.elapsed_ms = millis() - self->job_started_ms_;
            if (event->data) {
                const auto handle = static_cast<esp_tls_error_handle_t>(event->data);
                int system_error = 0, mbedtls_error = 0, cert_flags = 0, esp_error = 0;
                uint8_t available = 0;
                if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_SYSTEM,
                                                     &system_error) == ESP_OK) available |= 1u;
                if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_MBEDTLS,
                                                     &mbedtls_error) == ESP_OK) available |= 2u;
                if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_MBEDTLS_CERT_FLAGS,
                                                     &cert_flags) == ESP_OK) available |= 4u;
                if (esp_tls_get_and_clear_error_type(handle, ESP_TLS_ERR_TYPE_ESP,
                                                     &esp_error) == ESP_OK) available |= 8u;
                self->diagnostics_.recordTls(system_error, mbedtls_error,
                                             uint32_t(cert_flags), esp_error,
                                             available);
            }
            return ESP_OK;
        case HTTP_EVENT_ON_CONNECTED:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::connected;
            return ESP_OK;
        case HTTP_EVENT_HEADERS_SENT:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::headers_sent;
            return ESP_OK;
        case HTTP_EVENT_ON_HEADER:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::header_received;
            return ESP_OK;
        case HTTP_EVENT_ON_FINISH:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::finished;
            return ESP_OK;
        case HTTP_EVENT_DISCONNECTED:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::disconnected;
            return ESP_OK;
        case HTTP_EVENT_ON_DATA:
            self->diagnostics_.transport_stage = RemoteBrowserTransportStage::body_received;
            break;
        default:
            return ESP_OK;
    }
    if (event->data_len <= 0) return ESP_OK;
    const size_t amount = size_t(event->data_len);
    if (!self->body_ || amount > kBodyCapacity - self->body_size_) {
        self->body_overflow_ = true;
        return ESP_FAIL;
    }
    std::memcpy(self->body_ + self->body_size_, event->data, amount);
    self->body_size_ += amount;
    return ESP_OK;
}

bool RemoteBrowserService::beginMirror(uint32_t now_ms) {
    releaseHttp();
    mirror_started_ms_ = now_ms;
    char url[640]{};
    if (mirror_ >= RADIO_BROWSER_MIRROR_COUNT ||
        snprintf(url, sizeof url, "https://%s%s", RADIO_BROWSER_MIRRORS[mirror_], path_) >= int(sizeof url))
        return false;
    esp_http_client_config_t config{};
    config.url = url; config.method = HTTP_METHOD_GET;
    config.user_agent = RADIO_BROWSER_USER_AGENT;
    config.timeout_ms = remote_browser_transport::kTransportTimeoutMs;
    config.event_handler = onHttpEvent;
    config.user_data = this; config.is_async = true;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.disable_auto_redirect = false;
    config.max_redirection_count = remote_browser_transport::kMaxRedirects;
    config.buffer_size = 1024; config.buffer_size_tx = 512;
    client_ = esp_http_client_init(&config);
    if (attempt_count_ != UINT8_MAX) ++attempt_count_;
    diagnostics_.attempt_count = attempt_count_;
    diagnostics_.retry_waiting = 0;
    diagnostics_.mirror = mirror_;
    diagnostics_.transport_stage = client_
        ? RemoteBrowserTransportStage::client_initialized
        : RemoteBrowserTransportStage::event_error;
    return client_ != nullptr;
}

bool RemoteBrowserService::beginJob(Job job, const char* path, uint32_t now_ms) {
    if (!path || std::strlen(path) >= sizeof path_) return false;
    if (path != path_) std::strcpy(path_, path);
    job_ = job; mirror_ = 0; attempt_count_ = 0;
    delayed_retry_used_ = false; retry_waiting_ = false; retry_at_ms_ = 0;
    job_started_ms_ = now_ms;
    job_deadline_ms_ = now_ms + remote_browser_transport::kJobDeadlineMs;
    diagnostics_.job = uint8_t(job_);
    diagnostics_.status = uint8_t(rtcp_browser::Status::loading);
    diagnostics_.error = 0;
    diagnostics_.elapsed_ms = 0;
    diagnostics_.transport_error = 0;
    diagnostics_.first_transport_error = 0;
    diagnostics_.resetMirrorTransport();
    diagnostics_.http_status = 0;
    diagnostics_.first_http_status = 0;
    diagnostics_.tls_error = 0;
    diagnostics_.tls_flags = 0;
    diagnostics_.tls_system_error = diagnostics_.tls_mbedtls_error = 0;
    diagnostics_.tls_esp_error = 0; diagnostics_.tls_cert_flags = 0;
    diagnostics_.first_tls_system_error = diagnostics_.first_tls_mbedtls_error = 0;
    diagnostics_.first_tls_esp_error = 0; diagnostics_.first_tls_cert_flags = 0;
    diagnostics_.tls_available = diagnostics_.first_tls_available = 0;
    diagnostics_.deadline_expired = 0;
    diagnostics_.first_failure_stage = RemoteBrowserTransportStage::idle;
    diagnostics_.first_failure_mirror = 0;
    diagnostics_.attempt_count = 0;
    diagnostics_.retry_count = 0;
    diagnostics_.retry_waiting = 0;
    if (!beginMirror(now_ms)) { finishJob(false, rtcp_browser::internal); return false; }
    return true;
}

void RemoteBrowserService::finishJob(bool success, uint8_t error) {
    rtcp_browser::Status* status = job_ == Job::countries ? &country_status_
        : job_ == Job::genres ? &genre_status_ : &result_status_;
    uint8_t* target_error = job_ == Job::countries ? &country_error_
        : job_ == Job::genres ? &genre_error_ : &result_error_;
    uint16_t count = job_ == Job::countries ? country_count_
        : job_ == Job::genres ? genre_count_ : result_count_;
    *status = success ? (count ? rtcp_browser::Status::ready : rtcp_browser::Status::empty)
                      : rtcp_browser::Status::failed;
    *target_error = success ? 0 : error;
    RemoteBrowserDiagnostics::increment(success ? diagnostics_.jobs_completed
                                                : diagnostics_.jobs_failed);
    diagnostics_.status = uint8_t(*status);
    diagnostics_.error = *target_error;
    diagnostics_.elapsed_ms = millis() - job_started_ms_;
    releaseHttp(); job_ = Job::none; advanceRevision();
    diagnostics_.job = 0;
    diagnostics_.revision = revision_;
}

bool RemoteBrowserService::parseJob() {
    JsonDocument doc(&allocator_);
    const auto error = deserializeJson(doc, body_, body_size_);
    if (error || !doc.is<JsonArray>()) return false;
    if (job_ == Job::countries) {
        country_count_ = 0;
        for (JsonObject object : doc.as<JsonArray>()) {
            if (country_count_ >= MAX_BROWSE_COUNTRIES) break;
            const char* name = object["name"] | "";
            if (!name[0]) continue;
            strlcpy(countries_[country_count_].name, name, sizeof countries_[0].name);
            ++country_count_;
        }
    } else if (job_ == Job::genres) {
        genre_count_ = 0;
        for (JsonObject object : doc.as<JsonArray>()) {
            if (genre_count_ >= MAX_BROWSE_TAGS) break;
            const char* name = object["name"] | ""; if (!name[0]) continue;
            strlcpy(genres_[genre_count_++].name, name, sizeof genres_[0].name);
        }
    } else {
        result_count_ = 0;
        for (JsonObject object : doc.as<JsonArray>()) {
            if (result_count_ >= MAX_BROWSE_RESULTS) break;
            const char* name = object["name"] | ""; const char* url = object["url_resolved"] | "";
            String codec = object["codec"] | ""; codec.toUpperCase();
            if (!name[0] || !url[0]) continue;
            StationCodec type;
            if (codec.indexOf("MP3") >= 0) type = STATION_CODEC_MP3;
            else if (codec.indexOf("AAC") >= 0) type = STATION_CODEC_AAC;
            else continue;
            auto& result = results_[result_count_++];
            strlcpy(result.name, name, sizeof result.name); strlcpy(result.url, url, sizeof result.url);
            result.codec = type;
        }
    }
    return !doc.overflowed();
}

void RemoteBrowserService::poll(uint32_t now_ms) {
    if (job_ == Job::none) return;
    if (WiFi.status() != WL_CONNECTED) {
        diagnostics_.transport_error = ESP_ERR_INVALID_STATE;
        diagnostics_.elapsed_ms = now_ms - job_started_ms_;
        if (diagnostics_.first_failure_stage == RemoteBrowserTransportStage::idle) {
            diagnostics_.first_failure_stage = diagnostics_.transport_stage;
            diagnostics_.first_transport_error = diagnostics_.transport_error;
            diagnostics_.first_http_status = diagnostics_.http_status;
            diagnostics_.first_failure_mirror = mirror_;
        }
        finishJob(false, rtcp_browser::network); return;
    }
    if (retry_waiting_) {
        if (int32_t(now_ms - retry_at_ms_) < 0) return;
        retry_waiting_ = false;
        diagnostics_.retry_waiting = 0;
        mirror_ = 0;
        if (!beginMirror(now_ms)) finishJob(false, rtcp_browser::internal);
        return;
    }
    if (!client_) return;
    if (int32_t(now_ms - job_deadline_ms_) >= 0) {
        diagnostics_.transport_stage = RemoteBrowserTransportStage::deadline;
        diagnostics_.deadline_expired = 1;
        diagnostics_.elapsed_ms = now_ms - job_started_ms_;
        if (diagnostics_.first_failure_stage == RemoteBrowserTransportStage::idle) {
            diagnostics_.first_failure_stage = RemoteBrowserTransportStage::deadline;
            diagnostics_.first_failure_mirror = mirror_;
        }
        finishJob(false, rtcp_browser::network); return;
    }
    if (diagnostics_.transport_stage == RemoteBrowserTransportStage::client_initialized)
        diagnostics_.transport_stage = RemoteBrowserTransportStage::connecting;
    const esp_err_t result = esp_http_client_perform(client_);
    if (result == ESP_ERR_HTTP_EAGAIN) return;
    // Capture errno while the client handle is still valid. Cleanup and the
    // next mirror can overwrite the transport's session-local error state.
    const int socket_error = result == ESP_OK ? 0 : esp_http_client_get_errno(client_);
    const int status = esp_http_client_get_status_code(client_);
    const uint32_t mirror_elapsed = now_ms - mirror_started_ms_;
    const bool mirror_failed = result != ESP_OK || status != 200 || body_overflow_;
    diagnostics_.recordMirrorTerminal(mirror_, socket_error, mirror_elapsed,
                                      mirror_failed);
    diagnostics_.transport_error = result;
    diagnostics_.http_status = status > INT16_MAX ? INT16_MAX
        : status < INT16_MIN ? INT16_MIN : int16_t(status);
    diagnostics_.elapsed_ms = now_ms - job_started_ms_;
    if (result == ESP_OK && status == 200 && !body_overflow_) {
        finishJob(parseJob(), rtcp_browser::parse_failed); return;
    }
    if (body_overflow_) { finishJob(false, rtcp_browser::response_too_large); return; }
    if (diagnostics_.first_failure_stage == RemoteBrowserTransportStage::idle) {
        diagnostics_.first_failure_stage = diagnostics_.transport_stage;
        diagnostics_.first_transport_error = diagnostics_.transport_error;
        diagnostics_.first_http_status = diagnostics_.http_status;
        diagnostics_.first_failure_mirror = mirror_;
    }
    const auto action = remote_browser_transport::failureAction(
        result != ESP_OK, status, mirror_, kMirrorLimit, delayed_retry_used_);
    if (action == remote_browser_transport::FailureAction::next_mirror) {
        ++mirror_;
        if (beginMirror(now_ms)) return;
        finishJob(false, rtcp_browser::internal); return;
    }
    if (action == remote_browser_transport::FailureAction::delayed_canonical_retry &&
        attempt_count_ < remote_browser_transport::kMaximumAttempts) {
        releaseHttp();
        delayed_retry_used_ = true;
        retry_waiting_ = true;
        retry_at_ms_ = now_ms + remote_browser_transport::kRetryDelayMs;
        diagnostics_.transport_stage = RemoteBrowserTransportStage::retry_wait;
        diagnostics_.retry_count = 1;
        diagnostics_.retry_waiting = 1;
        return;
    }
    finishJob(false, rtcp_browser::network);
}

bool RemoteBrowserService::encode(const char* input, char* output, size_t capacity) {
    static const char hex[] = "0123456789ABCDEF"; size_t at = 0;
    if (!input || !output || !capacity) return false;
    for (const uint8_t* p = reinterpret_cast<const uint8_t*>(input); *p; ++p) {
        const bool plain = (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                           (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.';
        const size_t need = plain ? 1 : 3; if (at + need >= capacity) return false;
        if (plain) output[at++] = char(*p);
        else { output[at++]='%'; output[at++]=hex[*p>>4]; output[at++]=hex[*p&15]; }
    }
    output[at] = 0; return true;
}

uint8_t RemoteBrowserService::execute(const rtcp_browser::Command& command, uint32_t now_ms) {
    using namespace rtcp_browser;
    if (command.operation == Operation::start) {
        RemoteBrowserDiagnostics::increment(diagnostics_.start_requests);
        const uint8_t result = start(now_ms) ? success : unavailable;
        RemoteBrowserDiagnostics::increment(diagnostics_.start_results);
        diagnostics_.active = active_;
        diagnostics_.generation = generation_;
        diagnostics_.revision = revision_;
        return result;
    }
    if (!active_ || command.expected_generation != generation_) return stale;
    if (command.operation == Operation::cancel) {
        cancel(RemoteBrowserCancelReason::remote_command);
        return success;
    }
    if (command.expected_revision != revision_) return stale;
    if (job_ != Job::none) return busy;
    if (command.operation == Operation::select_country) {
        if (command.token != kAnyToken && command.token >= country_count_) return invalid_argument;
        selected_country_ = command.token; return success;
    }
    if (command.operation == Operation::select_name) {
        if (command.token > 26 || !country_count_) return invalid_argument;
        selected_name_ = uint8_t(command.token); genre_status_ = Status::loading;
        advanceRevision();
        return beginJob(Job::genres,
            "/json/tags?order=stationcount&reverse=true&limit=40&fields=name", now_ms)
            ? success : internal;
    }
    if (command.operation == Operation::select_genre) {
        if (command.token != kAnyToken && command.token >= genre_count_) return invalid_argument;
        selected_genre_ = command.token; result_status_ = Status::loading; result_count_ = 0;
        char country[BROWSE_COUNTRY_NAME_LEN * 3 + 32]{}, genre[128]{}, name[16]{};
        if (selected_country_ != kAnyToken) {
            char encoded[BROWSE_COUNTRY_NAME_LEN * 3]{};
            if (!encode(countries_[selected_country_].name, encoded, sizeof encoded))
                return invalid_argument;
            snprintf(country, sizeof country, "&country=%s&countryExact=true", encoded);
        }
        if (selected_genre_ != kAnyToken) {
            char encoded[96]{}; if (!encode(genres_[selected_genre_].name, encoded, sizeof encoded)) return invalid_argument;
            snprintf(genre, sizeof genre, "&tag=%s", encoded);
        }
        if (selected_name_) snprintf(name, sizeof name, "&name=%c", char('A' + selected_name_ - 1));
        snprintf(path_, sizeof path_, "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=20&fields=name,url_resolved,codec%s%s%s", country, genre, name);
        advanceRevision();
        return beginJob(Job::results, path_, now_ms) ? success : internal;
    }
    return (command.operation == Operation::play || command.operation == Operation::add_favorite) &&
           command.token < result_count_ ? success : invalid_argument;
}

bool RemoteBrowserService::takePlay(uint16_t token, char* name, size_t name_size,
                                    char* url, size_t url_size, StationCodec& codec) const {
    if (!results_ || token >= result_count_ || !name || !url) return false;
    strlcpy(name, results_[token].name, name_size); strlcpy(url, results_[token].url, url_size);
    codec = results_[token].codec; return true;
}

bool RemoteBrowserService::view(const rtcp_browser::Query& query,
                                rtcp_browser::StateView& out) const {
    using namespace rtcp_browser;
    RemoteBrowserDiagnostics::increment(diagnostics_.page_queries);
    if (!active_ || (query.generation && query.generation != generation_)) return false;
    out = {}; out.stage = query.stage; out.generation = generation_; out.revision = revision_;
    const char* current = audioPlayer.getCurrentURL();
    if (query.stage == Stage::countries) {
        out.status = country_status_; out.error = country_error_;
        uint16_t matches = 0;
        for (uint16_t i=0;i<country_count_;++i)
            if (!query.filter || uint8_t(toupper(uint8_t(countries_[i].name[0]))) == query.filter) ++matches;
        out.count = matches; out.start = query.offset < matches ? query.offset : matches;
        uint16_t seen=0;
        for (uint16_t i=0;i<country_count_ && out.item_count<query.limit;++i) {
            if (query.filter && uint8_t(toupper(uint8_t(countries_[i].name[0]))) != query.filter) continue;
            if (seen++ < out.start) continue;
            auto &item=out.items[out.item_count++];item.token=i;
            item.active=selected_country_==i;item.name=countries_[i].name;
        }
    } else if (query.stage == Stage::genres) {
        out.status=genre_status_; out.error=genre_error_; out.count=genre_count_;
        out.start=query.offset<out.count?query.offset:out.count;
        out.item_count=uint8_t(min<uint16_t>(query.limit,out.count-out.start));
        for(uint8_t i=0;i<out.item_count;++i){const uint16_t token=out.start+i;
            out.items[i].token=token;out.items[i].active=selected_genre_==token;out.items[i].name=genres_[token].name;}
    } else {
        out.status=result_status_; out.error=result_error_; out.count=result_count_;
        out.start=query.offset<out.count?query.offset:out.count;
        out.item_count=uint8_t(min<uint16_t>(query.limit,out.count-out.start));
        for(uint8_t i=0;i<out.item_count;++i){const uint16_t token=out.start+i;
            out.items[i].token=token;out.items[i].active=current&&current[0]&&strcmp(current,results_[token].url)==0;
            out.items[i].name=results_[token].name;}
    }
    RemoteBrowserDiagnostics::increment(diagnostics_.pages_produced);
    diagnostics_.active = active_;
    diagnostics_.job = uint8_t(job_);
    diagnostics_.status = uint8_t(out.status);
    diagnostics_.error = out.error;
    diagnostics_.generation = generation_;
    diagnostics_.revision = revision_;
    return true;
}

RemoteBrowserDiagnostics RemoteBrowserService::diagnostics() const {
    auto value = diagnostics_;
    value.active = active_;
    value.job = uint8_t(job_);
    value.generation = generation_;
    value.revision = revision_;
    return value;
}

