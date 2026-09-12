// "What is this hand doing" - turns raw hand landmarks into a shape
// description, independent of what gesture that shape maps to.
#pragma once

#include <variant>

#include "geometry.h"
#include "miaucam_bridge.h"

namespace miaucam {

struct HandInfo {
    bool index_up = false, middle_up = false, ring_up = false, pinky_up = false;
    bool thumb_out = false;
    int curled_count = 0;
    float hand_scale = 1e-6f;
    Vec3 index_tip, wrist, palm_center;
};

HandInfo classify_hand(const Hand& hand);

inline bool is_pointing(const HandInfo& h) {
    return h.index_up && !h.middle_up && !h.ring_up && !h.pinky_up;
}

// A pattern-matchable view of "how many hands, and what shape are they":
// classify_hands() turns MediaPipe's flat vector<Hand> into exactly one of
// these three, so callers std::visit over the shape instead of branching on
// hand_result.hands.size().
struct NoHands {};
struct OneHand {
    HandInfo hand;
};
struct TwoHands {
    HandInfo first;
    HandInfo second;
};
using HandsView = std::variant<NoHands, OneHand, TwoHands>;

// Only ever produces NoHands/OneHand/TwoHands - MediaPipe is configured
// with num_hands = 2, so more than two is not a case that occurs.
HandsView classify_hands(const HandResult& hand_result);

}  // namespace miaucam
