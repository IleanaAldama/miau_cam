// Dense optical flow reduced to a single "is this a spin" signal.
#pragma once

#include <deque>
#include <utility>

#include <opencv2/core.hpp>

namespace miaucam {

struct FlowSignal {
    double magnitude = 0.0;
    double coherence = 0.0;
};

// Everything the horizontal-flow reduction needs, gathered in one pass so the
// math on top of it is a pure function with no OpenCV in sight.
struct FlowStats {
    double sum_abs_x = 0.0;
    double sum_moving_x = 0.0;
    long total = 0;
    long moving = 0;
    long positive = 0;
    long negative = 0;
};

// A pixel is "moving" when its horizontal flow exceeds noise_floor_px.
FlowStats collect_flow_stats(const cv::Mat& flow, double noise_floor_px);

// magnitude is the mean |flow_x|; coherence is the share of moving pixels
// that agree with the dominant direction.
FlowSignal flow_signal_from_stats(const FlowStats& stats);

struct FlowResult {
    FlowSignal signal;
    cv::Mat small_gray;  // pass back in as prev_small_gray for the next frame
};

// frame: RGB24. Downsizes to a small gray image and runs Farneback flow
// against prev_small_gray (empty on the very first frame).
FlowResult compute_frame_flow(const cv::Mat& frame, const cv::Mat& prev_small_gray);

// Trailing-window state behind the spin trigger. Plain data - updated via
// the free function below rather than member methods.
struct SpinTrackerState {
    std::deque<std::pair<double, double>> flow_history;       // (t_ms, magnitude)
    std::deque<std::pair<double, double>> flow_peak_history;  // (t_ms, score)

    // debug fields, surfaced on the HUD.
    double last_magnitude_debug = 0.0;
    double last_coherence_debug = 0.0;
    double last_score_debug = 0.0;
    double last_fraction_debug = 0.0;
    double last_peak_debug = 0.0;
};

// Functional update: takes the previous state by value, returns the next
// one. Caller does `spin = update_spin_tracker(std::move(spin), ...)`.
SpinTrackerState update_spin_tracker(SpinTrackerState state, double magnitude,
                                       double coherence, double now_ms);

// A real spin keeps the flow magnitude elevated across most of a trailing
// window; a one-off lean/reach only fills a fraction of it. See tuning.h
// for the specific thresholds and how they were derived. Reads
// last_fraction_debug, which update_spin_tracker keeps current every frame
// - no timestamp needed here.
bool is_spinning(const SpinTrackerState& state);

}  // namespace miaucam
