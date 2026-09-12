#include "hand_classifier.h"

#include <algorithm>

namespace miaucam {

HandInfo classify_hand(const Hand& hand) {
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
    h.thumb_out = thumb_pinky_spread > 1.05f;

    const bool extended[] = {h.index_up, h.middle_up, h.ring_up, h.pinky_up};
    h.curled_count =
        static_cast<int>(std::count(std::begin(extended), std::end(extended), false));

    h.index_tip = pts[8];
    h.wrist = pts[0];
    h.palm_center = pts[9];
    return h;
}

HandsView classify_hands(const HandResult& hand_result) {
    switch (hand_result.hands.size()) {
        case 0:
            return NoHands{};
        case 1:
            return OneHand{classify_hand(hand_result.hands[0])};
        default:
            return TwoHands{classify_hand(hand_result.hands[0]),
                             classify_hand(hand_result.hands[1])};
    }
}

}  // namespace miaucam
