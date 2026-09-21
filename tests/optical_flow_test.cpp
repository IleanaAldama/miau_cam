#include "optical_flow.h"

#include <gtest/gtest.h>

#include "tuning.h"

using namespace miaucam;

namespace {

// 4x4 flow field: 8 pixels at +2, 2 at -2, the rest still.
cv::Mat make_flow() {
    cv::Mat flow(4, 4, CV_32FC2, cv::Scalar(0, 0));
    for (int i = 0; i < 8; ++i) flow.at<cv::Vec2f>(i / 4, i % 4)[0] = 2.0f;
    flow.at<cv::Vec2f>(2, 0)[0] = -2.0f;
    flow.at<cv::Vec2f>(2, 1)[0] = -2.0f;
    return flow;
}

}  // namespace

TEST(FlowStats, CountsMovingPixelsAndDirections) {
    const FlowStats stats = collect_flow_stats(make_flow(), 0.5);
    EXPECT_EQ(stats.total, 16);
    EXPECT_EQ(stats.moving, 10);
    EXPECT_EQ(stats.positive, 8);
    EXPECT_EQ(stats.negative, 2);
    EXPECT_DOUBLE_EQ(stats.sum_abs_x, 20.0);
    EXPECT_DOUBLE_EQ(stats.sum_moving_x, 12.0);
}

TEST(FlowStats, NoiseBelowTheFloorIsNotMoving) {
    cv::Mat flow(4, 4, CV_32FC2, cv::Scalar(0.3f, 0));
    const FlowStats stats = collect_flow_stats(flow, 0.5);
    EXPECT_EQ(stats.moving, 0);
    EXPECT_NEAR(stats.sum_abs_x, 16 * 0.3, 1e-5);
}

TEST(FlowSignal, MagnitudeIsTheMeanAbsoluteFlow) {
    const FlowSignal signal = flow_signal_from_stats(collect_flow_stats(make_flow(), 0.5));
    EXPECT_DOUBLE_EQ(signal.magnitude, 20.0 / 16.0);
}

TEST(FlowSignal, CoherenceIsTheShareAgreeingWithTheDominantDirection) {
    const FlowSignal signal = flow_signal_from_stats(collect_flow_stats(make_flow(), 0.5));
    EXPECT_DOUBLE_EQ(signal.coherence, 8.0 / 10.0);
}

TEST(FlowSignal, LeftwardDominanceCountsTheNegativePixels) {
    FlowStats stats;
    stats.total = 10;
    stats.moving = 5;
    stats.positive = 1;
    stats.negative = 4;
    stats.sum_moving_x = -6.0;
    EXPECT_DOUBLE_EQ(flow_signal_from_stats(stats).coherence, 4.0 / 5.0);
}

TEST(FlowSignal, TooLittleMotionMeansNoCoherence) {
    FlowStats stats;
    stats.total = 1000;
    stats.moving = 1;  // far below the required moving fraction
    stats.positive = 1;
    stats.sum_moving_x = 2.0;
    stats.sum_abs_x = 2.0;
    const FlowSignal signal = flow_signal_from_stats(stats);
    EXPECT_DOUBLE_EQ(signal.coherence, 0.0);
    EXPECT_DOUBLE_EQ(signal.magnitude, 2.0 / 1000);
}

TEST(FlowSignal, EmptyAndBalancedInputsAreQuiet) {
    EXPECT_DOUBLE_EQ(flow_signal_from_stats({}).magnitude, 0.0);

    FlowStats balanced;
    balanced.total = 10;
    balanced.moving = 4;
    balanced.positive = 2;
    balanced.negative = 2;  // cancels out: no dominant direction
    EXPECT_DOUBLE_EQ(flow_signal_from_stats(balanced).coherence, 0.0);
}

TEST(ComputeFrameFlow, FirstFrameHasNoSignalAndReturnsTheSmallGrayImage) {
    cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(10, 20, 30));
    const FlowResult result = compute_frame_flow(frame, cv::Mat());
    EXPECT_DOUBLE_EQ(result.signal.magnitude, 0.0);
    EXPECT_EQ(result.small_gray.cols, tuning::spin::flow_width);
    EXPECT_EQ(result.small_gray.rows, tuning::spin::flow_height);
    EXPECT_EQ(result.small_gray.channels(), 1);
}

TEST(ComputeFrameFlow, IdenticalFramesHaveNoMotion) {
    cv::Mat frame(480, 640, CV_8UC3);
    cv::randu(frame, 0, 255);
    const FlowResult first = compute_frame_flow(frame, cv::Mat());
    const FlowResult second = compute_frame_flow(frame, first.small_gray);
    EXPECT_NEAR(second.signal.magnitude, 0.0, 1e-3);
}
