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

// frame: RGB24. Downsizes both frames + runs Farneback optical flow between
// them, reduced to (magnitude, coherence). prev_small_gray is updated in
// place for the next call; pass an empty Mat for the very first frame.
FlowSignal compute_frame_flow(const cv::Mat& frame, cv::Mat& prev_small_gray);

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
