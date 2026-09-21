#include "detection_input.h"

#include <gtest/gtest.h>

using namespace miaucam;

TEST(DetectionInput, EmptyInputMeansNothingDetected) {
    const auto detection = to_detection({});
    EXPECT_TRUE(detection.hand.hands.empty());
    EXPECT_FALSE(detection.face.has_face);
    EXPECT_FALSE(detection.face.has_transform);
}

TEST(DetectionInput, SplitsHandsAt21Landmarks) {
    RawDetection raw;
    raw.hands.assign(2 * 21 * 3, 0.5f);
    raw.hands.push_back(1.0f);  // trailing partial point is dropped
    const auto detection = to_detection(raw);
    ASSERT_EQ(detection.hand.hands.size(), 2u);
    EXPECT_EQ(detection.hand.hands[0].landmarks.size(), 21u);
    EXPECT_FLOAT_EQ(detection.hand.hands[1].landmarks[20].z, 0.5f);
}

TEST(DetectionInput, FaceKeypointsAndExpression) {
    RawDetection raw;
    raw.face = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f};
    raw.expression = {0.9f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
    raw.transform.assign(16, 0.0f);
    raw.transform[3] = 7.0f;
    const auto detection = to_detection(raw);
    EXPECT_TRUE(detection.face.has_face);
    EXPECT_FLOAT_EQ(detection.face.keypoints.lower_lip.y, 0.5f);
    EXPECT_FLOAT_EQ(detection.face.keypoints.left_cheek.z, 1.2f);
    EXPECT_FLOAT_EQ(detection.face.expression.jaw_open, 0.9f);
    EXPECT_FLOAT_EQ(detection.face.expression.eye_wide_right, 0.7f);
    EXPECT_TRUE(detection.face.has_transform);
    EXPECT_FLOAT_EQ(detection.face.transform[3], 7.0f);
}

TEST(DetectionInput, WrongSizedFaceOrExpressionIsIgnored) {
    RawDetection raw;
    raw.face = {0.1f, 0.2f, 0.3f};
    raw.expression = {0.9f};
    const auto detection = to_detection(raw);
    EXPECT_FALSE(detection.face.has_face);
    EXPECT_FLOAT_EQ(detection.face.expression.jaw_open, 0.0f);
}

TEST(DetectionInput, WrongSizedTransformIsIgnored) {
    RawDetection raw;
    raw.transform.assign(9, 1.0f);
    EXPECT_FALSE(to_detection(raw).face.has_transform);
}

TEST(DetectionInput, ColumnMajorTransformIsTransposed) {
    RawDetection raw;
    raw.transform.assign(16, 0.0f);
    raw.transform[12] = 7.0f;  // translation x, column-major
    raw.transform[15] = 1.0f;
    const auto detection = to_detection(raw);
    EXPECT_FLOAT_EQ(detection.face.transform[3], 7.0f);
    EXPECT_FLOAT_EQ(detection.face.transform[12], 0.0f);
}
