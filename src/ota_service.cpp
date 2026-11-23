#include "ota_service.h"
#include "ota_policy.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <strings.h>
#include <esp_app_desc.h>
#include <esp_app_format.h>
#include <esp_crt_bundle.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs.h>
#include <psa/crypto.h>

#include "app_config.h"

namespace {
constexpr char tag[] = "OTA";
constexpr EventBits_t gotIp = BIT0;
constexpr EventBits_t disconnected = BIT1;
EventGroupHandle_t networkEvents;
esp_netif_t *station;
portMUX_TYPE statusMux = portMUX_INITIALIZER_UNLOCKED;
OtaService::Status status;
OtaPolicy::Manifest candidate;

uint32_t nowMs() { return xTaskGetTickCount() * portTICK_PERIOD_MS; }

void report(OtaService::Phase phase, const char *message, unsigned progress = 0,
            const char *version = "") {
    portENTER_CRITICAL(&statusMux);
    status.phase = phase;
    status.progress = progress;
    strlcpy(status.message, message, sizeof(status.message));
    strlcpy(status.version, version, sizeof(status.version));
    ++status.revision;
    portEXIT_CRITICAL(&statusMux);
}

void onNetworkEvent(void *, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        const auto *event = static_cast<wifi_event_sta_disconnected_t *>(data);
        ESP_LOGI(tag, "Station disconnected (reason %u)", event->reason);
        xEventGroupClearBits(networkEvents, gotIp);
        xEventGroupSetBits(networkEvents, disconnected);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        uint8_t channel = 0;
        wifi_second_chan_t secondary;
        if (esp_wifi_get_channel(&channel, &secondary) == ESP_OK && channel == ESP_NOW_CHANNEL) {
            xEventGroupSetBits(networkEvents, gotIp);
        } else {
            esp_wifi_disconnect();
        }
    }
}

// A connection exists only for a check/install. Keep the ESP-NOW radio initialized.
class NetworkSession {
public:
    ~NetworkSession() {
        if (timeStarted) esp_netif_sntp_deinit();
        xEventGroupClearBits(networkEvents, disconnected);
        if (esp_wifi_disconnect() == ESP_OK) {
            xEventGroupWaitBits(networkEvents, disconnected, pdFALSE, pdFALSE, pdMS_TO_TICKS(1500));
        }
        esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    }

    bool connect(std::string &error) {
        OtaService::Credentials credentials;
        if (!OtaService::loadCredentials(credentials)) {
            error = "Set up Wi-Fi first. Use the same router as the sensors (2.4 GHz, channel 1).";
            return false;
        }
        wifi_scan_config_t scan = {};
        scan.ssid = reinterpret_cast<uint8_t *>(credentials.ssid);
        scan.channel = ESP_NOW_CHANNEL;
        scan.show_hidden = true;
        scan.scan_time.active.min = 100;
        scan.scan_time.active.max = 150;
        if (esp_wifi_scan_start(&scan, true) != ESP_OK) {
            error = "Wi-Fi scan failed. Try again.";
            return false;
        }
        // The scan is SSID/channel filtered; get just the strongest matching record.
        wifi_ap_record_t accessPoint = {};
        uint16_t count = 1;
        const auto scanResult = esp_wifi_scan_get_ap_records(&count, &accessPoint);
        esp_wifi_clear_ap_list();
        if (scanResult != ESP_OK || count == 0 || accessPoint.primary != ESP_NOW_CHANNEL) {
            error = "Saved network not found on channel 1. Check router settings and range.";
            return false;
        }
        wifi_config_t config = {};
        memcpy(config.sta.ssid, credentials.ssid, strlen(credentials.ssid));
        memcpy(config.sta.password, credentials.password, strlen(credentials.password));
        config.sta.channel = ESP_NOW_CHANNEL;
        config.sta.bssid_set = true;
        memcpy(config.sta.bssid, accessPoint.bssid, sizeof(config.sta.bssid));
        config.sta.scan_method = WIFI_FAST_SCAN;
        config.sta.pmf_cfg.capable = true;
        const auto configured = esp_wifi_set_config(WIFI_IF_STA, &config);
        memset(&credentials, 0, sizeof(credentials));
        memset(&config.sta.password, 0, sizeof(config.sta.password));
        xEventGroupClearBits(networkEvents, gotIp | disconnected);
        if (configured != ESP_OK || esp_wifi_connect() != ESP_OK) {
            error = "Cannot start Wi-Fi connection.";
            return false;
        }
        const auto bits = xEventGroupWaitBits(networkEvents, gotIp | disconnected,
                                               pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
        if (!(bits & gotIp) || (bits & disconnected)) {
            error = "Wi-Fi connection failed. Check password, channel 1 and DHCP.";
            return false;
        }
        esp_sntp_config_t timeConfig = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        timeStarted = esp_netif_sntp_init(&timeConfig) == ESP_OK;
        const uint32_t start = nowMs();
        while (time(nullptr) < 1704067200) {
            if (!timeStarted || nowMs() - start > 20000 ||
                !(xEventGroupGetBits(networkEvents) & gotIp)) {
                error = "Cannot synchronize clock. Check router Internet/NTP access.";
                return false;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        return true;
    }
private:
    bool timeStarted = false;
};

// Reopen each redirect with certificate verification and an explicit host allowlist.
class Download {
public:
    ~Download() { close(); }
    bool open(std::string url, std::string &error) {
        for (unsigned hop = 0; hop <= 5; ++hop) {
            close();
            if (!OtaPolicy::allowedUrl(url)) { error = "Unsafe download URL or unexpected host."; return false; }
            location.clear();
            encoding.clear();
            esp_http_client_config_t config = {};
            config.url = url.c_str();
            config.crt_bundle_attach = esp_crt_bundle_attach;
            config.timeout_ms = 10000;
            config.disable_auto_redirect = true;
            config.buffer_size = 2048;
            config.buffer_size_tx = static_cast<int>(url.size() + 1024);
            config.event_handler = onEvent;
            config.user_data = this;
            config.user_agent = "Waveshare-43B-OTA";
            client = esp_http_client_init(&config);
            if (!client) { error = "Not enough memory for HTTPS."; return false; }
            esp_http_client_set_header(client, "Accept-Encoding", "identity");
            const auto result = esp_http_client_open(client, 0);
            if (result != ESP_OK) {
                error = std::string("HTTPS failed: ") + esp_err_to_name(result);
                return false;
            }
            length = esp_http_client_fetch_headers(client);
            const int code = esp_http_client_get_status_code(client);
            if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
                if (!location.empty() && location[0] == '/' && location.compare(0, 2, "//") != 0) {
                    location = url.substr(0, url.find('/', 8)) + location;
                }
                url = location;
                continue;
            }
            if (code != 200) {
                error = code == 404 ? "Release files not available yet. Wait for GitHub Actions to finish." :
                    "GitHub returned HTTP " + std::to_string(code);
                return false;
            }
            if (!encoding.empty() && encoding != "identity") { error = "Unexpected compressed response."; return false; }
            return true;
        }
        error = "Too many HTTPS redirects.";
        return false;
    }
    bool text(const std::string &url, std::string &body, std::string &error) {
        if (!open(url, error)) return false;
        if (length > 1024) { error = "Release manifest too large."; return false; }
        char buffer[256];
        const uint32_t start = nowMs();
        while (!complete()) {
            const int count = read(buffer, sizeof(buffer));
            if (count < 0 || nowMs() - start > 30000) { error = "Manifest download interrupted or timed out."; return false; }
            if (count == 0) { vTaskDelay(1); continue; }
            if (body.size() + count > 1024) { error = "Release manifest too large."; return false; }
            body.append(buffer, count);
        }
        return true;
    }
    int read(char *buffer, size_t size) { return esp_http_client_read(client, buffer, size); }
    bool complete() const { return esp_http_client_is_complete_data_received(client); }
    int64_t length = -1;
private:
    esp_http_client_handle_t client = nullptr;
    std::string location, encoding;
    void close() {
        if (client) { esp_http_client_close(client); esp_http_client_cleanup(client); client = nullptr; }
    }
    static esp_err_t onEvent(esp_http_client_event_t *event) {
        auto *self = static_cast<Download *>(event->user_data);
        if (event->event_id == HTTP_EVENT_ON_HEADER) {
            if (!strcasecmp(event->header_key, "Location")) self->location = event->header_value;
            if (!strcasecmp(event->header_key, "Content-Encoding")) self->encoding = event->header_value;
        }
        return ESP_OK;
    }
};

bool checkRelease(std::string &error) {
    candidate = {};
    std::string text;
    Download manifest;
    if (!manifest.text(std::string(OtaPolicy::releaseBase) + "latest/download/firmware.manifest", text, error)) return false;
    const auto *partition = esp_ota_get_next_update_partition(nullptr);
    if (!partition || !OtaPolicy::parseManifest(text, partition->size, candidate)) {
        error = "Invalid release manifest, wrong display model, or image too large.";
        return false;
    }
    return true;
}

bool installRelease(std::string &error) {
    Download firmware;
    if (!firmware.open(OtaPolicy::imageUrl(candidate), error)) return false;
    const auto *partition = esp_ota_get_next_update_partition(nullptr);
    if (!partition || candidate.size > partition->size || firmware.length != candidate.size) {
        error = "Firmware length differs from the checked release.";
        return false;
    }
    esp_ota_handle_t handle = 0;
    psa_hash_operation_t hash = PSA_HASH_OPERATION_INIT;
    if (psa_crypto_init() != PSA_SUCCESS || psa_hash_setup(&hash, PSA_ALG_SHA_256) != PSA_SUCCESS) {
        error = "Cannot initialize checksum.";
        psa_hash_abort(&hash);
        return false;
    }
    bool started = false;
    bool valid = true;
    size_t received = 0;
    uint32_t lastData = nowMs();
    const uint32_t start = lastData;
    // Validate the full image/app descriptor before erasing or writing any OTA slot.
    constexpr size_t prefixSize = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    static_assert(prefixSize == 288 && offsetof(esp_app_desc_t, version) == 16 &&
                  offsetof(esp_app_desc_t, project_name) == 48, "Update image identity offsets for this ESP-IDF");
    char prefix[prefixSize];
    size_t prefixBytes = 0;
    char buffer[4096];
    unsigned lastProgress = 101;
    while (received < candidate.size) {
        const int count = firmware.read(buffer, std::min<size_t>(sizeof(buffer), candidate.size - received));
        if (count < 0 || (count == 0 && (firmware.complete() || nowMs() - lastData > 15000)) ||
            nowMs() - start > 10 * 60 * 1000) {
            error = "Firmware download interrupted or timed out."; valid = false; break;
        }
        if (!count) { vTaskDelay(1); continue; }
        lastData = nowMs();
        size_t used = 0;
        if (prefixBytes < prefixSize) {
            used = std::min(static_cast<size_t>(count), prefixSize - prefixBytes);
            memcpy(prefix + prefixBytes, buffer, used);
            prefixBytes += used;
            if (prefixBytes == prefixSize) {
                if (!OtaPolicy::validImageIdentity(reinterpret_cast<uint8_t *>(prefix), prefixSize, candidate.version)) {
                    error = "Wrong board/project/version in firmware image."; valid = false; break;
                }
                auto result = esp_ota_begin(partition, candidate.size, &handle);
                if (result != ESP_OK) { error = "Cannot begin OTA write."; valid = false; break; }
                started = true;
                if (esp_ota_write(handle, prefix, prefixSize) != ESP_OK) {
                    error = "Firmware flash write failed."; valid = false; break;
                }
            }
        }
        if (started && static_cast<size_t>(count) > used &&
            esp_ota_write(handle, buffer + used, count - used) != ESP_OK) {
            error = "Firmware flash write failed."; valid = false; break;
        }
        if (psa_hash_update(&hash, reinterpret_cast<uint8_t *>(buffer), count) != PSA_SUCCESS) {
            error = "Cannot calculate firmware checksum."; valid = false; break;
        }
        received += count;
        const unsigned progress = received * 100 / candidate.size;
        if (progress != lastProgress) {
            report(OtaService::Phase::Installing, "Downloading and writing update. Keep power connected.", progress, candidate.version.c_str());
            lastProgress = progress;
        }
        vTaskDelay(1);
    }
    uint8_t digest[32];
    size_t digestSize = 0;
    if (valid && psa_hash_finish(&hash, digest, sizeof(digest), &digestSize) != PSA_SUCCESS) {
        error = "Cannot finish firmware checksum."; valid = false;
    }
    psa_hash_abort(&hash);
    char hex[65] = {};
    if (valid && digestSize == sizeof(digest)) {
        for (size_t i = 0; i < sizeof(digest); ++i) snprintf(hex + i * 2, 3, "%02x", digest[i]);
    }
    if (valid && (candidate.sha256 != hex || !started)) {
        error = "Firmware checksum mismatch."; valid = false;
    }
    if (!valid) { if (started) esp_ota_abort(handle); return false; }
    report(OtaService::Phase::Installing, "Verifying firmware...", 100, candidate.version.c_str());
    const auto result = esp_ota_end(handle);
    if (result != ESP_OK) { error = std::string("Image validation failed: ") + esp_err_to_name(result); return false; }
    if (esp_ota_set_boot_partition(partition) != ESP_OK) { error = "Cannot select updated firmware."; return false; }
    return true;
}

void operationTask(void *argument) {
    const bool install = argument != nullptr;
    std::string error;
    bool success;
    {
        NetworkSession network;
        success = network.connect(error) && (install ? installRelease(error) : checkRelease(error));
    } // Release HTTPS, SNTP and association before allowing another operation.
    if (!success) {
        report(OtaService::Phase::Error, error.c_str());
    } else if (install) {
        report(OtaService::Phase::Restarting, "Update verified. Restarting...", 100, candidate.version.c_str());
        vTaskDelay(pdMS_TO_TICKS(2500));
        esp_restart();
    } else if (OtaPolicy::newerVersion(candidate.version, esp_app_get_description()->version)) {
        report(OtaService::Phase::Available, "Update found. Install only when the engine is stopped; the display will restart.", 0, candidate.version.c_str());
    } else {
        report(OtaService::Phase::Current, "No newer release available.", 100, candidate.version.c_str());
    }
    vTaskDelete(nullptr);
}

void request(bool install) {
    esp_ota_img_states_t runningState;
    if (install && esp_ota_get_state_partition(esp_ota_get_running_partition(), &runningState) == ESP_OK &&
        runningState == ESP_OTA_IMG_PENDING_VERIFY) {
        report(OtaService::Phase::Error, "Startup checks still running. Wait 15 seconds, then check again.");
        return;
    }
    portENTER_CRITICAL(&statusMux);
    if (OtaService::busy(status.phase) || (install && status.phase != OtaService::Phase::Available)) {
        portEXIT_CRITICAL(&statusMux);
        return;
    }
    status.phase = install ? OtaService::Phase::Installing : OtaService::Phase::Checking;
    status.progress = 0;
    strlcpy(status.message, "Connecting to saved Wi-Fi on channel 1...", sizeof(status.message));
    ++status.revision;
    portEXIT_CRITICAL(&statusMux);
    if (xTaskCreate(operationTask, "github_ota", 12288, install ? reinterpret_cast<void *>(1) : nullptr, 2, nullptr) != pdPASS) {
        report(OtaService::Phase::Error, "Not enough memory to start update task.");
    }
}
} // namespace

namespace OtaService {
void initializeNetwork() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    station = esp_netif_create_default_wifi_sta();
    networkEvents = xEventGroupCreate();
    ESP_ERROR_CHECK(station && networkEvents ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, onNetworkEvent, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, onNetworkEvent, nullptr));
}
Status snapshot() {
    portENTER_CRITICAL(&statusMux);
    const auto copy = status;
    portEXIT_CRITICAL(&statusMux);
    return copy;
}
bool busy(Phase phase) { return phase == Phase::Checking || phase == Phase::Installing || phase == Phase::Restarting; }
bool loadCredentials(Credentials &out) {
    nvs_handle_t handle;
    if (nvs_open("dashboard_wifi", NVS_READONLY, &handle) != ESP_OK) return false;
    size_t size = sizeof(out);
    const auto result = nvs_get_blob(handle, "credentials", &out, &size);
    nvs_close(handle);
    return result == ESP_OK && size == sizeof(out) && out.ssid[0] &&
           strnlen(out.ssid, sizeof(out.ssid)) <= 32 && strnlen(out.password, sizeof(out.password)) <= 63;
}
bool saveCredentials(const Credentials &credentials) {
    if (busy(snapshot().phase) || !credentials.ssid[0] ||
        strnlen(credentials.ssid, sizeof(credentials.ssid)) > 32) return false;
    const size_t length = strnlen(credentials.password, sizeof(credentials.password));
    if (length != 0 && (length < 8 || length > 63)) return false;
    nvs_handle_t handle;
    if (nvs_open("dashboard_wifi", NVS_READWRITE, &handle) != ESP_OK) return false;
    auto result = nvs_set_blob(handle, "credentials", &credentials, sizeof(credentials));
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    if (result == ESP_OK) report(Phase::Idle, "Wi-Fi saved. Tap Check for updates to test the connection.");
    return result == ESP_OK;
}
void requestCheck() { request(false); }
void requestInstall() { request(true); }
} // namespace OtaService
