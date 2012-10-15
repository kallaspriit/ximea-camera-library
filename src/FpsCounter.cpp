#include "FpsCounter.h"
#include "Util.h"

FpsCounter::FpsCounter(int interval) : interval(interval) {
    startTime = -1;
    frames = 0;
    fps = 0;
    changed = false;
}

void FpsCounter::step() {
    if (startTime == -1) {
        startTime = Util::millitime();
        frames = 1;

        return;
    }

    if (frames >= interval) {
        double currentTime = Util::millitime();
        double elapsedTime = currentTime - startTime;

        fps = frames / elapsedTime;
        startTime = currentTime;
        frames = 0;

        changed = true;
    } else {
        frames++;
    }
}

bool FpsCounter::isChanged() {
    if (changed) {
        changed = false;

        return true;
    } else {
        return false;
    }
}

int FpsCounter::getFps() {
    return fps;
}
