// "state -> pixels". No detection logic, no thresholds owned here - just
// drawing.
#pragma once

#include <opencv2/core.hpp>

#include "gesture.h"
#include "gesture_state.h"
#include "miaucam_bridge.h"

namespace miaucam {

void draw_debug_hud(cv::Mat& frame, const GestureState& state, Gesture current_gesture);
void draw_landmarks(cv::Mat& frame, const HandResult& hand_result);

// Resizes img to the given height, keeping aspect ratio.
cv::Mat fit_to_height(const cv::Mat& img, int height);

}  // namespace miaucam
