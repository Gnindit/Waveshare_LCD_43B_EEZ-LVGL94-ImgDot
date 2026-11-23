#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace OtaPolicy {
inline constexpr char deviceId[] = "waveshare-43b-imgdot";
inline constexpr char imageName[] = "firmware.bin";
inline constexpr char projectName[] = "waveshare_lcd_43";
inline constexpr char releaseBase[] =
    "https://github.com/Gnindit/Waveshare_LCD_43B_EEZ-LVGL94-ImgDot/releases/";

inline bool allowedUrl(const std::string &url) {
    if (url.size() > 8192 || url.compare(0, 8, "https://") != 0) return false;
    for (unsigned char c : url) if (c <= 32 || c >= 127 || c == '\\' || c == '#') return false;
    const auto end = url.find_first_of("/?", 8);
    const auto host = url.substr(8, end == std::string::npos ? end : end - 8);
    return host == "github.com" || host == "release-assets.githubusercontent.com" ||
           host == "objects.githubusercontent.com";
}

inline bool versionParts(const std::string &text, uint32_t (&parts)[3]) {
    if (text.empty() || text.size() > 31) return false;
    size_t pos = text[0] == 'v' ? 1 : 0;
    for (unsigned i = 0; i < 3; ++i) {
        const size_t start = pos;
        uint32_t value = 0;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
            value = value * 10 + static_cast<unsigned>(text[pos++] - '0');
            if (value > 65535) return false;
        }
        if (pos == start || (pos - start > 1 && text[start] == '0')) return false;
        parts[i] = value;
        if (i < 2 && (pos == text.size() || text[pos++] != '.')) return false;
    }
    return pos == text.size();
}

inline bool newerVersion(const std::string &candidate, const std::string &current) {
    uint32_t a[3], b[3];
    if (!versionParts(candidate, a)) return false;
    if (!versionParts(current, b)) return true; // Allow the first release over a development build.
    for (unsigned i = 0; i < 3; ++i) if (a[i] != b[i]) return a[i] > b[i];
    return false;
}

struct Manifest {
    std::string version;
    std::string sha256;
    uint32_t size = 0;
};

inline bool parseManifest(const std::string &text, uint32_t maxSize, Manifest &out) {
    if (text.size() > 1024) return false;
    const char *keys[] = {"format=", "device=", "version=", "size=", "sha256=", "image="};
    std::string values[6];
    size_t pos = 0;
    for (unsigned i = 0; i < 6; ++i) {
        const std::string key = keys[i];
        const auto end = text.find('\n', pos);
        if (end == std::string::npos || text.compare(pos, key.size(), key) != 0) return false;
        values[i] = text.substr(pos + key.size(), end - pos - key.size());
        pos = end + 1;
    }
    uint32_t parts[3];
    if (pos != text.size() || values[0] != "1" || values[1] != deviceId ||
        values[5] != imageName || !versionParts(values[2], parts)) return false;
    if (values[3].empty() || values[3].size() > 8) return false;
    uint32_t size = 0;
    for (char c : values[3]) {
        if (c < '0' || c > '9') return false;
        size = size * 10 + static_cast<unsigned>(c - '0');
    }
    if (size < 288 || size > maxSize || values[4].size() != 64) return false;
    for (char c : values[4]) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    out = {values[2], values[4], size};
    return true;
}

inline std::string imageUrl(const Manifest &manifest) {
    return std::string(releaseBase) + "download/" + manifest.version + "/" + imageName;
}

inline bool validImageIdentity(const uint8_t *prefix, size_t length, const std::string &version) {
    // ESP image header (24), first segment header (8), app descriptor (256).
    if (length < 288 || prefix[0] != 0xe9 || prefix[12] != 9 || prefix[13] != 0 ||
        prefix[32] != 0x32 || prefix[33] != 0x54 || prefix[34] != 0xcd || prefix[35] != 0xab) return false;
    return version.size() <= 31 &&
           std::strncmp(reinterpret_cast<const char *>(prefix + 48), version.c_str(), 32) == 0 &&
           std::strncmp(reinterpret_cast<const char *>(prefix + 80), projectName, 32) == 0;
}

class HoldGesture {
public:
    void press(uint32_t now) { active = true; started = now; }
    void cancel() { active = false; }
    bool release(uint32_t now) {
        const bool accepted = active && static_cast<uint32_t>(now - started) >= 3000;
        active = false;
        return accepted;
    }
private:
    bool active = false;
    uint32_t started = 0;
};
} // namespace OtaPolicy
