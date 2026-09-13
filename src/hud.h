// "state -> pixels" - no detection logic or thresholds owned here.
#pragma once

#include <opencv2/core.hpp>

#include "gesture.h"
#include "gesture_state.h"
#include "miaucam_bridge.h"

namespace miaucam {

void draw_debug_hud(cv::Mat& frame, const GestureState& state, Gesture current_gesture);
void draw_landmarks(cv::Mat& frame, const HandResult& hand_result);

cv::Mat fit_to_height(const cv::Mat& img, int height);

// side eye cat.jpg faces left by default; mirrors it when yaw_deg says the
// subject turned the other way.
cv::Mat orient_for_yaw(const cv::Mat& meme, double yaw_deg);

}  // namespace miaucam
