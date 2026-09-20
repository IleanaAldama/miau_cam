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

TEST(DetectionInput, FaceBlendshapesAndTransform) {
    RawDetection raw;
    raw.face = {0.1f, 0.2f, 0.3f};
    raw.blendshape_names = {"jawOpen", "eyeBlinkLeft"};
    raw.blendshape_scores = {0.9f};
    raw.transform.assign(16, 0.0f);
    raw.transform[3] = 7.0f;
    const auto detection = to_detection(raw);
    EXPECT_TRUE(detection.face.has_face);
    ASSERT_EQ(detection.face.blendshapes.size(), 1u);
    EXPECT_EQ(detection.face.blendshapes[0].name, "jawOpen");
    EXPECT_TRUE(detection.face.has_transform);
    EXPECT_FLOAT_EQ(detection.face.transform[3], 7.0f);
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
