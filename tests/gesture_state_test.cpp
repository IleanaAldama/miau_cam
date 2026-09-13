#include "gesture_state.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

using namespace miaucam;

namespace {

FaceResult make_face(double yaw_deg, float jaw_open, float eye_wide) {
    FaceResult f;
    f.has_face = true;
    f.landmarks.resize(455);
    f.landmarks[13] = {0.5f, 0.45f, 0};   // upper lip
    f.landmarks[14] = {0.5f, 0.47f, 0};   // lower lip
    f.landmarks[234] = {0.35f, 0.46f, 0}; // right cheek
    f.landmarks[454] = {0.65f, 0.46f, 0}; // left cheek

    f.has_transform = true;
    double rad = yaw_deg * M_PI / 180.0;
    float c = static_cast<float>(std::cos(rad)), s = static_cast<float>(std::sin(rad));
    float m[16] = {
        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1,
    };
    std::copy(std::begin(m), std::end(m), f.transform);

    f.blendshapes.push_back({"jawOpen", jaw_open});
    f.blendshapes.push_back({"eyeWideLeft", eye_wide});
    f.blendshapes.push_back({"eyeWideRight", eye_wide});
    return f;
}

Hand make_fist_hand() {
    Hand h;
    h.landmarks.resize(21);
    h.landmarks[0] = {0.5f, 1.0f, 0};
    auto curl = [&](int mcp, int pip, int tip, float x) {
        h.landmarks[mcp] = {x, 0.7f, 0};
        h.landmarks[pip] = {x, 0.6f, 0};
        h.landmarks[tip] = {x, 0.68f, 0};
    };
    curl(5, 6, 8, 0.42f);
    curl(9, 10, 12, 0.5f);
    curl(13, 14, 16, 0.58f);
    curl(17, 18, 20, 0.66f);
    h.landmarks[4] = {0.6f, 0.72f, 0};  // thumb tucked
    return h;
}

}  // namespace

TEST(GestureState, DefaultWithNothing) {
    GestureState state;
    HandResult hands;
    EXPECT_EQ(decide(state, hands, 0.0), Gesture::Default);
}

TEST(GestureState, SideEyeFromYawNoHands) {
    GestureState state = update_face(GestureState{}, make_face(25.0, 0.0f, 0.0f), 0.0);
    HandResult hands;
    EXPECT_EQ(decide(state, hands, 0.0), Gesture::SideEyeCat);
}

TEST(GestureState, FistWithOneHand) {
    GestureState state;
    HandResult hands;
    hands.hands.push_back(make_fist_hand());
    EXPECT_EQ(decide(state, hands, 0.0), Gesture::Fist);
}

TEST(GestureState, MouthOpenBeatsHandShape) {
    GestureState state = update_face(GestureState{}, make_face(0.0, 0.9f, 0.0f), 0.0);
    HandResult hands;
    hands.hands.push_back(make_fist_hand());  // would be Fist on its own
    EXPECT_EQ(decide(state, hands, 0.0), Gesture::MouthOpenCat);
}

TEST(GestureState, SpinBeatsEverything) {
    GestureState state;
    double t = 0.0;
    for (int i = 0; i < 40; ++i) {
        state = update_flow(std::move(state), /*magnitude=*/1.0, /*coherence=*/1.0, t);
        t += 50.0;
    }
    HandResult hands;
    hands.hands.push_back(make_fist_hand());  // would be Fist if spin didn't win
    EXPECT_EQ(decide(state, hands, t), Gesture::SpinCat);
}
