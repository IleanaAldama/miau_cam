#include "face_signals.h"

#include <gtest/gtest.h>

#include "tuning.h"

using namespace miaucam;

namespace {

FaceResult make_face() {
    FaceResult f;
    f.has_face = true;
    f.keypoints = {{0.5f, 0.40f, 0}, {0.5f, 0.50f, 0}, {0.3f, 0.45f, 0}, {0.7f, 0.45f, 0}};
    f.expression.jaw_open = 0.6f;
    f.expression.smile_left = 0.2f;
    f.expression.smile_right = 0.4f;
    f.expression.brow_inner_up = 0.3f;
    return f;
}

}  // namespace

TEST(FaceSignals, NoFaceMeansNoSignals) {
    EXPECT_FALSE(extract_face_signals(FaceResult{}, 0.0).has_value());
}

TEST(FaceSignals, GeometryComesFromTheFourKeypoints) {
    const auto s = extract_face_signals(make_face(), 42.0);
    ASSERT_TRUE(s.has_value());
    EXPECT_NEAR(s->mouth_center.y, 0.45f, 1e-6);
    EXPECT_NEAR(s->face_width, 0.4f, 1e-6);
    EXPECT_NEAR(s->mouth_open, 0.1f / 0.4f, 1e-5);
    EXPECT_DOUBLE_EQ(s->t_ms, 42.0);
}

TEST(FaceSignals, ExpressionScoresAreCombinedPerSignal) {
    FaceResult f = make_face();
    f.expression.eye_wide_left = 0.2f;
    f.expression.eye_wide_right = 0.5f;
    const auto s = extract_face_signals(f, 0.0);
    EXPECT_NEAR(s->jaw_open, 0.6, 1e-6);
    EXPECT_NEAR(s->smile, 0.4, 1e-6);  // the stronger side
    EXPECT_NEAR(s->brow_raise, 0.3, 1e-6);
    EXPECT_NEAR(s->eye_wide, 0.5, 1e-6);
}

TEST(FaceSignals, MissingTransformMeansZeroPose) {
    const auto s = extract_face_signals(make_face(), 0.0);
    EXPECT_DOUBLE_EQ(s->yaw_deg, 0.0);
    EXPECT_DOUBLE_EQ(s->pitch_deg, 0.0);
}

TEST(WinkScore, NeedsOneEyeClearlyClosing) {
    FaceExpression e;
    e.blink_left = 0.2f;
    e.blink_right = 0.1f;
    EXPECT_DOUBLE_EQ(wink_score(e), 0.0);

    e.blink_left = 0.9f;
    e.blink_right = 0.2f;
    EXPECT_NEAR(wink_score(e), 0.7, 1e-6);
}

TEST(Blend, MixesEveryFieldAndKeepsTheRawTimestamp) {
    FaceSignals before, raw;
    before.yaw_deg = 10.0;
    before.jaw_open = 0.2;
    before.face_width = 0.4f;
    before.mouth_center = {0.5f, 0.5f, 0.0f};
    before.t_ms = 0.0;
    raw.yaw_deg = 30.0;
    raw.jaw_open = 0.6;
    raw.face_width = 0.6f;
    raw.mouth_center = {0.7f, 0.7f, 0.0f};
    raw.t_ms = 33.0;

    const FaceSignals out = blend(before, raw, 0.5);
    EXPECT_DOUBLE_EQ(out.yaw_deg, 20.0);
    EXPECT_DOUBLE_EQ(out.jaw_open, 0.4);
    EXPECT_NEAR(out.face_width, 0.5f, 1e-6);
    EXPECT_NEAR(out.mouth_center.x, 0.6f, 1e-6);
    EXPECT_DOUBLE_EQ(out.t_ms, 33.0);
}

TEST(Blend, AlphaOneIsRawAndAlphaZeroIsPrevious) {
    FaceSignals before, raw;
    before.pitch_deg = 5.0;
    raw.pitch_deg = 25.0;
    EXPECT_DOUBLE_EQ(blend(before, raw, 1.0).pitch_deg, 25.0);
    EXPECT_DOUBLE_EQ(blend(before, raw, 0.0).pitch_deg, 5.0);
}
