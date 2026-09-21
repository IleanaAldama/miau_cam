// Platform-agnostic frame pipeline: advance() turns (state, frame, detections)
// into the next state and the gesture to show. Detection itself is injected,
// so desktop (MediaPipe C++) and web (MediaPipe Tasks) share this file.
#pragma once

#include <cstdint>
#include <utility>

#include <opencv2/core.hpp>

#include "debounce.h"
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
    DebounceState debounce;
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

// flow, then face, then decide, then debounce.
std::pair<CoreState, StepOutput> advance(CoreState state, const FrameInput& input,
                                           DetectionResult detection);

}  // namespace miaucam
