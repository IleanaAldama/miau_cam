#include "hand_classifier.h"

#include <algorithm>
#include <vector>

#include "tuning.h"

namespace miaucam {

namespace {
// Collapsed clumps and stretched fragments (a detector locking onto a shadow
// edge) aren't hands: a real fingertip stays within a few palm lengths.
bool plausible_hand(const HandInfo& h) {
    if (h.hand_scale < tuning::hand::min_hand_scale) return false;
    const float tip_reach = dist(h.index_tip, h.wrist);
    return tip_reach / h.hand_scale <= tuning::hand::max_reach_ratio;
}
}  // namespace

std::optional<HandInfo> classify_hand(const Hand& hand) {
    if (hand.landmarks.size() < 21) return std::nullopt;

    std::vector<Vec3> pts;
    pts.reserve(hand.landmarks.size());
    for (const auto& lm : hand.landmarks) pts.push_back(p3(lm));

    HandInfo h;
    h.hand_scale = std::max(dist(pts[0], pts[9]), 1e-6f);

    h.index_up = finger_extended(pts, 5, 6, 8);
    h.middle_up = finger_extended(pts, 9, 10, 12);
    h.ring_up = finger_extended(pts, 13, 14, 16);
    h.pinky_up = finger_extended(pts, 17, 18, 20);

    float thumb_pinky_spread = dist(pts[4], pts[17]) / h.hand_scale;
    h.thumb_out = thumb_pinky_spread > tuning::hand_shape::thumb_out_spread;

    const bool extended[] = {h.index_up, h.middle_up, h.ring_up, h.pinky_up};
    h.curled_count =
        static_cast<int>(std::count(std::begin(extended), std::end(extended), false));

    h.index_tip = pts[8];
    h.wrist = pts[0];
    h.palm_center = pts[9];

    if (!plausible_hand(h)) return std::nullopt;
    return h;
}

HandsView classify_hands(const HandResult& hand_result) {
    std::vector<HandInfo> plausible;
    plausible.reserve(std::min<size_t>(hand_result.hands.size(), 2));
    for (const Hand& hand : hand_result.hands) {
        if (auto info = classify_hand(hand)) plausible.push_back(*std::move(info));
    }
    switch (plausible.size()) {
        case 0:
            return NoHands{};
        case 1:
            return OneHand{std::move(plausible[0])};
        default:
            return TwoHands{std::move(plausible[0]), std::move(plausible[1])};
    }
}

}  // namespace miaucam
