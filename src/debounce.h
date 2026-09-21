// Turns the jumpy per-frame gesture into the one that is actually shown: a
// new gesture must repeat for a few frames, and no gesture for a while falls
// back to the default.
#pragma once

#include "gesture.h"

namespace miaucam {

struct DebounceState {
    Gesture current = Gesture::Default;
    Gesture candidate = Gesture::Default;
    int streak = 0;
    double last_non_default_at = 0.0;
};

struct DebounceStep {
    DebounceState state;
    bool changed;  // true only on the frame the shown gesture switched
};

DebounceStep debounce(DebounceState state, Gesture detected, double now_ms);

}  // namespace miaucam
