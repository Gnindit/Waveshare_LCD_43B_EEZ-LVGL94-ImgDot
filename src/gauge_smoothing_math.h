#pragma once
#include <stdint.h>

// Unsigned subtraction keeps animation timing correct across tick wraparound.
inline float timedGaugeValue(float start, float target, uint32_t started,
                             uint32_t now, uint32_t duration)
{
    const uint32_t elapsed = now - started;
    if (duration == 0 || elapsed >= duration) return target;
    return start + (target - start) * (static_cast<float>(elapsed) / duration);
}
