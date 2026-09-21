// "What is this hand doing" - a shape description, independent of gesture.
#pragma once

#include <optional>
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

// nullopt when the detection isn't a plausible hand shape (collapsed or
// stretched landmarks), so a shadow/edge can't masquerade as a gesture input.
std::optional<HandInfo> classify_hand(const Hand& hand);

inline bool is_pointing(const HandInfo& h) {
    return h.index_up && !h.middle_up && !h.ring_up && !h.pinky_up;
}

// Lets callers std::visit over hand count instead of branching on size().
struct NoHands {};
struct OneHand {
    HandInfo hand;
};
struct TwoHands {
    HandInfo first;
    HandInfo second;
};
using HandsView = std::variant<NoHands, OneHand, TwoHands>;

HandsView classify_hands(const HandResult& hand_result);

}  // namespace miaucam
