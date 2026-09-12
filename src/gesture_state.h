// The gesture decision machine: consumes hand_classifier + face_signals +
// optical_flow outputs and names what's being seen. Plain data plus free
// functions rather than a class with methods - update_flow/update_face take
// the previous state by value and return the next one, so state transitions
// are explicit at the call site instead of hidden inside method bodies.
#pragma once

#include <optional>

#include "face_signals.h"
#include "gesture.h"
#include "miaucam_bridge.h"
#include "optical_flow.h"

namespace miaucam {

struct GestureState {
    std::optional<FaceSnapshot> last_face;
    bool face_seen_this_frame = false;
    SpinTrackerState spin;

    // debug fields, surfaced on the HUD.
    double last_yaw_debug = 0.0;
    double last_pitch_debug = 0.0;
    double last_jaw_open_debug = 0.0;
    double last_smile_debug = 0.0;
    double last_brow_raise_debug = 0.0;
    double last_wink_debug = 0.0;
    double last_eye_wide_debug = 0.0;
};

GestureState update_flow(GestureState state, double magnitude, double coherence, double now_ms);
GestureState update_face(GestureState state, const FaceResult& face_result, double now_ms);

// Names the current gesture. Priority order matches gesture_meme.py exactly
// (spin beats everything; mouth-open-with-a-hand beats hand-shape reads;
// etc) - see gesture_state.cpp for the reasoning behind each branch.
Gesture decide(const GestureState& state, const HandResult& hand_result, double now_ms);

}  // namespace miaucam
