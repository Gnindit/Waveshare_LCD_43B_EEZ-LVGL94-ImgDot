#include "ota_policy.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace OtaPolicy;
    for (const char *url : {
        "http://github.com/file", "https://github.com.evil.test/file",
        "https://github.com@evil.test/file", "https://github.com:443/file",
        "https://github.com\\@evil.test/file", "https://github.com/file#fragment",
        "https://github.com/file\r\nInjected: yes", "https://example.com/file",
        "https://release-assets.githubusercontent.com.evil.test/file"}) assert(!allowedUrl(url));
    assert(allowedUrl("https://github.com/Gnindit/project/releases/download/v1.0.0/firmware.bin"));
    assert(allowedUrl("https://release-assets.githubusercontent.com/file?signature=123"));
    assert(!allowedUrl("https://github.com/" + std::string(8192, 'x')));
    assert(newerVersion("v1.1.0", "v1.0.9"));
    assert(newerVersion("v1.0.0", "dev-local"));
    assert(!newerVersion("1.0.0", "v1.0.0"));
    assert(!newerVersion("v1.0.0", "v2.0.0"));
    assert(!newerVersion("v1.0.0-rc1", "v0.1.0"));
    assert(!newerVersion("v65536.0.0", "dev-local"));
    assert(!newerVersion("v01.0.0", "dev-local"));
    const std::string valid = "format=1\ndevice=waveshare-43b-imgdot\nversion=v1.2.3\nsize=4096\nsha256=" +
                               std::string(64, 'a') + "\nimage=firmware.bin\n";
    Manifest manifest;
    assert(parseManifest(valid, 0x640000, manifest));
    assert(manifest.version == "v1.2.3" && manifest.size == 4096);
    assert(imageUrl(manifest) == std::string(releaseBase) + "download/v1.2.3/firmware.bin");
    assert(!parseManifest(valid, 4000, manifest));
    assert(!parseManifest(valid + "extra=1\n", 0x640000, manifest));
    assert(!parseManifest(valid.substr(0, valid.size() - 1), 0x640000, manifest));
    for (const auto &pair : {std::pair<std::string, std::string>{"waveshare-43b-imgdot", "other-board"},
                            {"v1.2.3", "../other"}, {"size=4096", "size=-1"},
                            {"size=4096", "size=99999999999"}, {"size=4096", "size=0"},
                            {"image=firmware.bin", "image=factory.bin"}, {"format=1", "format=2"},
                            {std::string(64, 'a'), std::string(64, 'z')}}) {
        auto invalid = valid;
        invalid.replace(invalid.find(pair.first), pair.first.size(), pair.second);
        assert(!parseManifest(invalid, 0x640000, manifest));
    }
    uint8_t prefix[288] = {};
    prefix[0] = 0xe9;
    prefix[12] = 9;
    prefix[32] = 0x32; prefix[33] = 0x54; prefix[34] = 0xcd; prefix[35] = 0xab;
    std::memcpy(prefix + 48, "v1.2.3", 7);
    std::memcpy(prefix + 80, "waveshare_lcd_43", 16);
    assert(validImageIdentity(prefix, sizeof(prefix), "v1.2.3"));
    assert(!validImageIdentity(prefix, 287, "v1.2.3"));
    assert(!validImageIdentity(prefix, sizeof(prefix), "v1.2.4"));
    prefix[12] = 0; // ESP32 rather than ESP32-S3.
    assert(!validImageIdentity(prefix, sizeof(prefix), "v1.2.3"));
    prefix[12] = 9; prefix[80] = 'X';
    assert(!validImageIdentity(prefix, sizeof(prefix), "v1.2.3"));
    HoldGesture gesture;
    assert(!gesture.release(9999)); // No prior press.
    gesture.press(100); assert(!gesture.release(3099)); // Short by 1 ms.
    gesture.press(100); assert(gesture.release(3100));
    assert(!gesture.release(10000)); // Same touch cannot activate twice.
    gesture.press(100); gesture.cancel(); assert(!gesture.release(10000));
    gesture.press(UINT32_MAX - 1000); assert(gesture.release(1999)); // Tick wrap.
    std::cout << "OTA URL, manifest, board, version and size policy tests passed\n";
}
