#include "geometry.h"

#include <gtest/gtest.h>

using namespace miaucam;

TEST(Geometry, Dist) {
    EXPECT_NEAR(dist(Vec3(0, 0, 0), Vec3(3, 4, 0)), 5.0f, 1e-5);
}

TEST(Geometry, AngleDegParallelIsZero) {
    EXPECT_NEAR(angle_deg(Vec3(1, 0, 0), Vec3(2, 0, 0)), 0.0, 1e-3);
}

TEST(Geometry, AngleDegOppositeIs180) {
    EXPECT_NEAR(angle_deg(Vec3(1, 0, 0), Vec3(-1, 0, 0)), 180.0, 1e-3);
}

TEST(Geometry, AngleDegPerpendicularIs90) {
    EXPECT_NEAR(angle_deg(Vec3(1, 0, 0), Vec3(0, 1, 0)), 90.0, 1e-3);
}

TEST(Geometry, FingerExtendedStraightLine) {
    std::vector<Vec3> pts(9);
    pts[5] = Vec3(0.5f, 0.7f, 0);
    pts[6] = Vec3(0.5f, 0.55f, 0);
    pts[8] = Vec3(0.5f, 0.4f, 0);
    EXPECT_TRUE(finger_extended(pts, 5, 6, 8));
}

TEST(Geometry, FingerCurledBackIsNotExtended) {
    std::vector<Vec3> pts(9);
    pts[5] = Vec3(0.5f, 0.7f, 0);
    pts[6] = Vec3(0.5f, 0.6f, 0);
    pts[8] = Vec3(0.5f, 0.68f, 0);  // tip curls back past pip
    EXPECT_FALSE(finger_extended(pts, 5, 6, 8));
}

TEST(Geometry, YawAndPitchFromIdentityTransformAreZero) {
    float m[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    EXPECT_NEAR(yaw_from_transform(m), 0.0, 1e-6);
    EXPECT_NEAR(pitch_from_transform(m), 0.0, 1e-6);
}
