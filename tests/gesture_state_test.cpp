#include "gesture_state.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include "tuning.h"

using namespace miaucam;

namespace {

FaceResult make_face(double yaw_deg, float jaw_open, float eye_wide) {
    FaceResult f;
    f.has_face = true;
    f.keypoints = {{0.5f, 0.45f, 0},    // upper lip
                   {0.5f, 0.47f, 0},    // lower lip
                   {0.35f, 0.46f, 0},   // right cheek
                   {0.65f, 0.46f, 0}};  // left cheek

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

    f.expression.jaw_open = jaw_open;
    f.expression.eye_wide_left = eye_wide;
    f.expression.eye_wide_right = eye_wide;
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

TEST(GestureState, HuhNeedsOpenMouthAndWideEyesWithNoHands) {
    GestureState state = update_face(GestureState{}, make_face(0.0, 0.2f, 0.2f), 0.0);
    EXPECT_EQ(decide(state, HandResult{}, 0.0), Gesture::HuhCat);

    HandResult hands;
    hands.hands.push_back(make_fist_hand());
    EXPECT_EQ(decide(state, hands, 0.0), Gesture::Fist);
}

TEST(GestureState, StaleFaceIsIgnored) {
    GestureState state = update_face(GestureState{}, make_face(25.0, 0.0f, 0.0f), 0.0);
    EXPECT_TRUE(fresh_face(state, 100.0).has_value());
    EXPECT_FALSE(fresh_face(state, tuning::stability::face_stale_ms + 1.0).has_value());
    EXPECT_EQ(decide(state, HandResult{}, tuning::stability::face_stale_ms + 1.0), Gesture::Default);
}

TEST(GestureState, FirstFaceSightingPassesThroughAndLaterOnesBlend) {
    GestureState state = update_face(GestureState{}, make_face(20.0, 0.0f, 0.0f), 0.0);
    ASSERT_TRUE(state.last_face.has_value());
    EXPECT_NEAR(state.last_face->yaw_deg, 20.0, 1e-3);

    state = update_face(std::move(state), make_face(0.0, 0.0f, 0.0f), 33.0);
    const double alpha = tuning::smoothing::ema_alpha;
    EXPECT_NEAR(state.last_face->yaw_deg, 20.0 * (1.0 - alpha), 1e-3);
}

TEST(GestureState, LosingTheFaceKeepsTheLastSignals) {
    GestureState state = update_face(GestureState{}, make_face(20.0, 0.0f, 0.0f), 0.0);
    state = update_face(std::move(state), FaceResult{}, 33.0);
    EXPECT_FALSE(state.face_seen_this_frame);
    ASSERT_TRUE(state.last_face.has_value());
    EXPECT_NEAR(state.last_face->yaw_deg, 20.0, 1e-3);
}
