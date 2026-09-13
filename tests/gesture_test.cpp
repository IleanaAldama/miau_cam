#include "gesture.h"

#include <gtest/gtest.h>

using namespace miaucam;

TEST(Gesture, EveryGestureHasMetadata) {
    EXPECT_EQ(all_gestures().size(), 16u);
    for (const auto& info : all_gestures()) {
        EXPECT_FALSE(info.name.empty());
        EXPECT_FALSE(info.files.empty());
        EXPECT_EQ(info_for(info.id).id, info.id);
    }
}

TEST(Gesture, VideoGesturesHaveExactlyOneFile) {
    for (const auto& info : all_gestures()) {
        if (info.is_video) EXPECT_EQ(info.files.size(), 1u);
    }
}

TEST(Gesture, ToStringAndIsVideoGesture) {
    EXPECT_EQ(to_string(Gesture::SpinCat), "spinCat");
    EXPECT_EQ(to_string(Gesture::Default), "default");
    EXPECT_TRUE(is_video_gesture(Gesture::SpinCat));
    EXPECT_TRUE(is_video_gesture(Gesture::DanceCat));
    EXPECT_FALSE(is_video_gesture(Gesture::Default));
}
