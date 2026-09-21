// Per-frame perception state plus the priority rules that turn it into a
// gesture. Plain data plus free functions: update_flow/update_face take the
// previous state by value and return the next one; decide is a pure function.
#pragma once

#include <optional>

#include "face_signals.h"
#include "gesture.h"
#include "miaucam_bridge.h"
#include "optical_flow.h"

namespace miaucam {

struct GestureState {
    std::optional<FaceSignals> last_face;  // smoothed; stale once the face is lost
    bool face_seen_this_frame = false;
    SpinTrackerState spin;
};

GestureState update_flow(GestureState state, double magnitude, double coherence, double now_ms);
GestureState update_face(GestureState state, const FaceResult& face_result, double now_ms);

// The last face, only while it is recent enough to trust.
std::optional<FaceSignals> fresh_face(const GestureState& state, double now_ms);

// First matching rule wins, in a fixed priority order (see the rule list).
Gesture decide(const GestureState& state, const HandResult& hand_result, double now_ms);

}  // namespace miaucam
