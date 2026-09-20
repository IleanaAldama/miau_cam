// Platform-agnostic frame pipeline: advance() turns (state, frame, detections)
// into the next state and the gesture to show. Detection itself is injected,
// so desktop (MediaPipe C++) and Android (MediaPipe Tasks) share this file.
#pragma once

#include <cstdint>
#include <utility>

#include <opencv2/core.hpp>

#include "gesture.h"
#include "gesture_state.h"
#include "miaucam_bridge.h"

namespace miaucam {

struct DetectionResult {
    HandResult hand;
    FaceResult face;
};

struct CoreState {
    GestureState gesture_state;
    cv::Mat prev_flow_gray;
    Gesture current_gesture = Gesture::Default;
    Gesture candidate_gesture = Gesture::Default;
    int candidate_streak = 0;
    double last_non_default_at = 0.0;
};

struct FrameInput {
    RgbFrameView frame;
    double timestamp_ms;
};

struct StepOutput {
    Gesture gesture;
    bool gesture_changed;
    HandResult hand;
};

// The side-eye meme mirrors with head yaw; the sign was verified live.
bool flip_meme(const CoreState& state);

std::pair<CoreState, StepOutput> advance(CoreState state, const FrameInput& input,
                                           DetectionResult detection);

}  // namespace miaucam
