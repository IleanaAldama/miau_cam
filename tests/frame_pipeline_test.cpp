#include "frame_pipeline.h"

#include <gtest/gtest.h>

using namespace miaucam;

namespace {

CoreState side_eye_with_yaw(double yaw_deg) {
    CoreState state;
    state.debounce.current = Gesture::SideEyeCat;
    FaceSignals face;
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
    state.debounce.current = Gesture::Fist;
    EXPECT_FALSE(flip_meme(state));

    CoreState no_face;
    no_face.debounce.current = Gesture::SideEyeCat;
    EXPECT_FALSE(flip_meme(no_face));
}

TEST(Advance, QuietSyntheticFramesStayDefault) {
    cv::Mat frame(90, 160, CV_8UC3, cv::Scalar(90, 90, 90));
    const FrameInput input{{frame.data, frame.cols, frame.rows, static_cast<int>(frame.step)}, 0.0};

    CoreState state;
    for (int i = 0; i < 10; ++i) {
        auto [next, output] = advance(std::move(state), input, DetectionResult{});
        state = std::move(next);
        EXPECT_EQ(output.gesture, Gesture::Default);
        EXPECT_FALSE(output.gesture_changed);
    }
}
