#include "optical_flow.h"

#include <algorithm>

#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>

#include "tuning.h"

namespace miaucam {

FlowSignal compute_frame_flow(const cv::Mat &frame, cv::Mat &prev_small_gray) {
  cv::Mat gray, small;
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
  cv::resize(gray, small,
             cv::Size(tuning::spin_flow_width, tuning::spin_flow_height));

  FlowSignal sig;
  if (prev_small_gray.empty()) {
    prev_small_gray = small;
    return sig;
  }

  cv::Mat flow;
  cv::calcOpticalFlowFarneback(prev_small_gray, small, flow, 0.5, 2, 15, 2, 5,
                               1.2, 0);
  std::vector<cv::Mat> channels(2);
  cv::split(flow, channels);
  const cv::Mat &flow_x = channels[0];

  sig.magnitude = cv::mean(cv::abs(flow_x))[0];

  cv::Mat moving_mask = cv::abs(flow_x) > tuning::spin_flow_noise_floor_px;
  int moving_count = cv::countNonZero(moving_mask);
  int total = flow_x.rows * flow_x.cols;

  if (static_cast<double>(moving_count) / total <
      tuning::spin_flow_min_moving_fraction) {
    sig.coherence = 0.0;
    prev_small_gray = small;
    return sig;
  }

  double masked_mean = cv::mean(flow_x, moving_mask)[0];
  if (masked_mean == 0.0) {
    sig.coherence = 0.0;
  } else {
    cv::Mat same_sign_mask = masked_mean > 0 ? (flow_x > 0) : (flow_x < 0);
    cv::Mat agree_mask;
    cv::bitwise_and(moving_mask, same_sign_mask, agree_mask);
    int agree = cv::countNonZero(agree_mask);
    sig.coherence = static_cast<double>(agree) / moving_count;
  }

  prev_small_gray = small;
  return sig;
}

namespace {
void prune_older_than(std::vector<std::pair<double, double>> &v, double now_ms,
                      double window_ms) {
  v.erase(std::remove_if(
              v.begin(), v.end(),
              [&](const auto &p) { return now_ms - p.first >= window_ms; }),
          v.end());
}
} // namespace

SpinTrackerState update_spin_tracker(SpinTrackerState state, double magnitude,
                                     double coherence, double now_ms) {
  double score = magnitude * coherence; // debug/log only, not the trigger

  state.flow_history.emplace_back(now_ms, magnitude);
  prune_older_than(state.flow_history, now_ms, tuning::spin_fraction_window_ms);

  state.flow_peak_history.emplace_back(now_ms, score);
  prune_older_than(state.flow_peak_history, now_ms,
                   tuning::spin_flow_peak_hold_ms);

  state.last_magnitude_debug = magnitude;
  state.last_coherence_debug = coherence;
  state.last_score_debug = score;

  state.last_peak_debug = 0.0;
  for (const auto &[t, s] : state.flow_peak_history) {
    state.last_peak_debug = std::max(state.last_peak_debug, s);
  }

  int elevated = 0;
  for (const auto &[t, m] : state.flow_history) {
    if (m > tuning::spin_mag_threshold)
      elevated++;
  }
  state.last_fraction_debug =
      state.flow_history.empty()
          ? 0.0
          : static_cast<double>(elevated) / state.flow_history.size();

  return state;
}

bool is_spinning(const SpinTrackerState &state) {
  // flow_history is already pruned by update_spin_tracker every frame, so
  // re-pruning here isn't needed the way the Python version did it
  // defensively - last_fraction_debug is always current.
  return state.last_fraction_debug > tuning::spin_fraction_required;
}

} // namespace miaucam
