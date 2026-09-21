#include "hud.h"

#include <cstdio>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "tuning.h"

namespace miaucam {

namespace {

const std::vector<std::pair<int, int>> kHandConnections = {
    {0, 1}, {1, 2}, {2, 3}, {3, 4},
    {0, 5}, {5, 6}, {6, 7}, {7, 8},
    {5, 9}, {9, 10}, {10, 11}, {11, 12},
    {9, 13}, {13, 14}, {14, 15}, {15, 16},
    {13, 17}, {17, 18}, {18, 19}, {19, 20},
    {0, 17},
};

}  // namespace

void draw_debug_hud(cv::Mat& frame, const GestureState& state, Gesture current_gesture) {
    char buf[256];
    std::vector<std::string> lines;

    lines.push_back("gesture: " + to_string(current_gesture));

    std::snprintf(buf, sizeof(buf), "yaw: %+.1f deg  (side-eye thr +/-%.1f)",
                  state.last_yaw_debug.value_or(0.0),
                  tuning::head_pose::side_eye_yaw_deg);
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf), "flow mag: %.2f  (thr %.2f)", state.spin.last_magnitude_debug,
                  tuning::spin::mag_threshold);
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf), "spin fraction (2.2s window): %.2f  (thr %.2f)",
                  state.spin.last_fraction_debug, tuning::spin::fraction_required);
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf),
                  "peak score (last 2s): %.2f  <- read this AFTER you stop spinning",
                  state.spin.last_peak_debug);
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf), "jawOpen: %.2f  eyeWide: %.2f  (huh needs both > %.2f/%.2f)",
                  state.last_jaw_open_debug.value_or(0.0),
                  state.last_eye_wide_debug.value_or(0.0), tuning::huh::jaw_threshold,
                  tuning::huh::eye_wide_threshold);
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf),
                  "smile: %.2f  browRaise: %.2f  wink: %.2f  <- not wired to a meme yet",
                  state.last_smile_debug.value_or(0.0),
                  state.last_brow_raise_debug.value_or(0.0), state.last_wink_debug.value_or(0.0));
    lines.push_back(buf);

    std::snprintf(buf, sizeof(buf),
                  "pitch: %+.1f deg  (side-eye-down thr %.1f, unvalidated - watch this while looking down)",
                  state.last_pitch_debug.value_or(0.0),
                  tuning::head_pose::side_eye_down_pitch_deg);
    lines.push_back(buf);

    for (size_t i = 0; i < lines.size(); ++i) {
        int y = 24 + static_cast<int>(i) * 22;
        cv::putText(frame, lines[i], {10, y}, cv::FONT_HERSHEY_SIMPLEX, 0.55, {0, 0, 0}, 3,
                    cv::LINE_AA);
        cv::putText(frame, lines[i], {10, y}, cv::FONT_HERSHEY_SIMPLEX, 0.55, {0, 255, 120}, 1,
                    cv::LINE_AA);
    }
}

void draw_landmarks(cv::Mat& frame, const HandResult& hand_result) {
    int w = frame.cols, h = frame.rows;
    for (const auto& hand : hand_result.hands) {
        std::vector<cv::Point> pts;
        pts.reserve(hand.landmarks.size());
        for (const auto& lm : hand.landmarks) {
            pts.emplace_back(static_cast<int>(lm.x * w), static_cast<int>(lm.y * h));
        }
        for (const auto& [a, b] : kHandConnections) {
            cv::line(frame, pts[a], pts[b], {80, 220, 120}, 2);
        }
        for (const auto& p : pts) cv::circle(frame, p, 4, {60, 140, 255}, -1);
    }
}

cv::Mat fit_to_height(const cv::Mat& img, int height) {
    double scale = static_cast<double>(height) / img.rows;
    cv::Mat out;
    cv::resize(img, out, cv::Size(static_cast<int>(img.cols * scale), height));
    return out;
}

}  // namespace miaucam
