#pragma once

#include <cstdint>

namespace OtaService {
enum class Phase { Idle, Checking, Available, Current, Installing, Restarting, Error };
struct Status {
    Phase phase = Phase::Idle;
    unsigned progress = 0;
    uint32_t revision = 0;
    char message[192] = "Hold Update for 3 seconds to open this menu.";
    char version[32] = "";
};
struct Credentials {
    char ssid[33] = {};
    char password[64] = {};
};
void initializeNetwork();
Status snapshot();
bool busy(Phase phase);
bool loadCredentials(Credentials &out);
bool saveCredentials(const Credentials &credentials);
void requestCheck();
void requestInstall();
} // namespace OtaService
