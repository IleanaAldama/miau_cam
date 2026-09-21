#include "optical_flow.h"

#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>

#include "tuning.h"

namespace miaucam {

FlowStats collect_flow_stats(const cv::Mat &flow, double noise_floor_px) {
  FlowStats stats;
  stats.total = static_cast<long>(flow.rows) * flow.cols;

  for (int y = 0; y < flow.rows; ++y) {
    const cv::Vec2f *row = flow.ptr<cv::Vec2f>(y);
    for (int x = 0; x < flow.cols; ++x) {
      const float fx = row[x][0];
      const float magnitude = std::abs(fx);
      stats.sum_abs_x += magnitude;
      if (magnitude <= noise_floor_px)
        continue;

      stats.moving++;
      stats.sum_moving_x += fx;
      (fx > 0 ? stats.positive : stats.negative)++;
    }
  }
  return stats;
}

FlowSignal flow_signal_from_stats(const FlowStats &stats) {
  FlowSignal signal;
  if (stats.total == 0)
    return signal;

  signal.magnitude = stats.sum_abs_x / stats.total;

  const double moving_fraction = static_cast<double>(stats.moving) / stats.total;
  if (stats.moving == 0 || moving_fraction < tuning::spin::flow_min_moving_fraction)
    return signal;

  const double dominant = stats.sum_moving_x;
  if (dominant == 0.0)
    return signal;

  const long agree = dominant > 0 ? stats.positive : stats.negative;
  signal.coherence = static_cast<double>(agree) / stats.moving;
  return signal;
}

FlowResult compute_frame_flow(const cv::Mat &frame, const cv::Mat &prev_small_gray) {
  cv::Mat small_rgb, small;
  cv::resize(frame, small_rgb,
             cv::Size(tuning::spin::flow_width, tuning::spin::flow_height));
  cv::cvtColor(small_rgb, small, cv::COLOR_RGB2GRAY);

  if (prev_small_gray.empty())
    return {FlowSignal{}, small};

  cv::Mat flow;
  cv::calcOpticalFlowFarneback(prev_small_gray, small, flow, 0.5, 2, 15, 2, 5, 1.2, 0);
  const FlowStats stats = collect_flow_stats(flow, tuning::spin::flow_noise_floor_px);
  return {flow_signal_from_stats(stats), small};
}

namespace {
// Entries are appended in increasing timestamp order, so stale ones are
// always a front prefix - pop_front is O(1) instead of a vector shift.
void prune_older_than(std::deque<std::pair<double, double>> &q, double now_ms,
                      double window_ms) {
  while (!q.empty() && now_ms - q.front().first >= window_ms) {
    q.pop_front();
  }
}
} // namespace

SpinTrackerState update_spin_tracker(SpinTrackerState state, double magnitude,
                                     double coherence, double now_ms) {
  double score = magnitude * coherence; // debug/log only, not the trigger

  state.flow_history.emplace_back(now_ms, magnitude);
  prune_older_than(state.flow_history, now_ms, tuning::spin::fraction_window_ms);

  state.flow_peak_history.emplace_back(now_ms, score);
  prune_older_than(state.flow_peak_history, now_ms,
                   tuning::spin::flow_peak_hold_ms);

  state.last_magnitude_debug = magnitude;
  state.last_coherence_debug = coherence;
  state.last_score_debug = score;

  state.last_peak_debug = 0.0;
  for (const auto &[t, s] : state.flow_peak_history) {
    state.last_peak_debug = std::max(state.last_peak_debug, s);
  }

  int elevated = 0;
  for (const auto &[t, m] : state.flow_history) {
    if (m > tuning::spin::mag_threshold)
      elevated++;
  }
  state.last_fraction_debug =
      state.flow_history.empty()
          ? 0.0
          : static_cast<double>(elevated) / state.flow_history.size();

  return state;
}

bool is_spinning(const SpinTrackerState &state) {
  // last_fraction_debug is kept current by update_spin_tracker every frame.
  return state.last_fraction_debug > tuning::spin::fraction_required;
}

} // namespace miaucam
