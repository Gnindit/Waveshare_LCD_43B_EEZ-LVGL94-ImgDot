#include <cassert>
#include <cmath>
#include <iostream>
#include "gauge_smoothing_math.h"

static void near(float actual, float expected) {
    assert(std::fabs(actual - expected) < 0.001f);
}

int main() {
    // A full-scale movement finishes in one second regardless of timer cadence.
    near(timedGaugeValue(0, 750, 100, 100, 1000), 0);
    near(timedGaugeValue(0, 750, 100, 600, 1000), 375);
    near(timedGaugeValue(0, 750, 100, 1100, 1000), 750);
    near(timedGaugeValue(0, 750, 100, 9000, 1000), 750);
    // Falling readings and interruption by a new target have no position jump.
    const float halfway = timedGaugeValue(80, 20, 100, 600, 1000);
    near(halfway, 50);
    near(timedGaugeValue(halfway, 70, 600, 600, 1000), halfway);
    near(timedGaugeValue(halfway, 70, 600, 1100, 1000), 60);
    // Tick rollover, no-change targets, and disabled animation.
    near(timedGaugeValue(0, 100, UINT32_MAX - 499, 0, 1000), 50);
    near(timedGaugeValue(40, 40, 0, 500, 1000), 40);
    near(timedGaugeValue(0, 100, 10, 10, 0), 100);
    std::cout << "Gauge transition timing, settling, reversal and tick wrap tests passed\n";
}
