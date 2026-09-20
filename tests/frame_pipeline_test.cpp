#include "frame_pipeline.h"

#include <gtest/gtest.h>

using namespace miaucam;

namespace {

CoreState side_eye_with_yaw(double yaw_deg) {
    CoreState state;
    state.current_gesture = Gesture::SideEyeCat;
    FaceSnapshot face;
    face.yaw_deg = yaw_deg;
    state.gesture_state.last_face = face;
    return state;
}

}  // namespace

TEST(FlipMeme, MirrorsOnlyWhenYawIsNonNegative) {
    EXPECT_TRUE(flip_meme(side_eye_with_yaw(12.0)));
    EXPECT_TRUE(flip_meme(side_eye_with_yaw(0.0)));
    EXPECT_FALSE(flip_meme(side_eye_with_yaw(-12.0)));
}

TEST(FlipMeme, OtherGesturesAndMissingFaceNeverFlip) {
    auto state = side_eye_with_yaw(30.0);
    state.current_gesture = Gesture::Fist;
    EXPECT_FALSE(flip_meme(state));

    CoreState no_face;
    no_face.current_gesture = Gesture::SideEyeCat;
    EXPECT_FALSE(flip_meme(no_face));
}
