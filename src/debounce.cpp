#include "debounce.h"

#include "tuning.h"

namespace miaucam {

DebounceStep debounce(DebounceState state, Gesture detected, double now_ms) {
    if (detected == state.candidate) {
        state.streak++;
    } else {
        state.candidate = detected;
        state.streak = 1;
    }

    bool changed = false;
    if (state.streak >= tuning::stability::frames_required && detected != state.current) {
        state.current = detected;
        changed = true;
    }

    if (detected != Gesture::Default) {
        state.last_non_default_at = now_ms;
    } else if (now_ms - state.last_non_default_at > tuning::stability::default_fallback_ms &&
               state.current != Gesture::Default) {
        state.current = Gesture::Default;
        changed = true;
    }
    return {state, changed};
}

}  // namespace miaucam
