#include "hand_classifier.h"

#include <gtest/gtest.h>

using namespace miaucam;

namespace {

Hand make_hand(bool index_up, bool middle_up, bool ring_up, bool pinky_up, bool thumb_out) {
    Hand hand;
    hand.landmarks.resize(21);

    auto finger = [&](int mcp, int pip, int tip, float x, bool extended) {
        hand.landmarks[mcp] = {x, 0.7f, 0};
        hand.landmarks[pip] = {x, 0.55f, 0};
        hand.landmarks[tip] = {x, extended ? 0.4f : 0.68f, 0};  // curled: tip snaps back
    };

    hand.landmarks[0] = {0.5f, 1.0f, 0};  // wrist
    finger(5, 6, 8, 0.42f, index_up);
    finger(9, 10, 12, 0.5f, middle_up);   // also sets palm_center (9)
    finger(13, 14, 16, 0.58f, ring_up);
    finger(17, 18, 20, 0.66f, pinky_up);

    hand.landmarks[4] = {thumb_out ? 0.2f : 0.6f, 0.72f, 0};
    return hand;
}

}  // namespace

TEST(HandClassifier, OpenPalmHasNoCurledFingers) {
    HandInfo h = classify_hand(make_hand(true, true, true, true, false));
    EXPECT_TRUE(h.index_up && h.middle_up && h.ring_up && h.pinky_up);
    EXPECT_EQ(h.curled_count, 0);
}

TEST(HandClassifier, FistCurlsAllFour) {
    HandInfo h = classify_hand(make_hand(false, false, false, false, false));
    EXPECT_EQ(h.curled_count, 4);
}

TEST(HandClassifier, PointingIsIndexOnly) {
    EXPECT_TRUE(is_pointing(classify_hand(make_hand(true, false, false, false, false))));
    EXPECT_FALSE(is_pointing(classify_hand(make_hand(true, true, false, false, false))));
}

TEST(HandClassifier, RockstarShape) {
    HandInfo h = classify_hand(make_hand(false, false, false, true, true));
    EXPECT_TRUE(h.thumb_out && h.pinky_up && !h.index_up && !h.middle_up && !h.ring_up);
}

TEST(HandClassifier, ClassifyHandsDispatchesOnCount) {
    HandResult none;
    EXPECT_TRUE(std::holds_alternative<NoHands>(classify_hands(none)));

    HandResult one;
    one.hands.push_back(make_hand(true, false, false, false, false));
    EXPECT_TRUE(std::holds_alternative<OneHand>(classify_hands(one)));

    HandResult two;
    two.hands.push_back(make_hand(true, false, false, false, false));
    two.hands.push_back(make_hand(false, false, false, false, false));
    EXPECT_TRUE(std::holds_alternative<TwoHands>(classify_hands(two)));
}
