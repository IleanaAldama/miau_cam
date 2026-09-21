// Tunable thresholds, grouped by the gesture/system each one belongs to.
#pragma once

namespace miaucam {
namespace tuning {

namespace stability {
constexpr int frames_required = 5;
constexpr double default_fallback_ms = 600.0;
constexpr double face_stale_ms = 1200.0;
}  // namespace stability

namespace head_pose {
constexpr double side_eye_yaw_deg = 15.0;
// unvalidated sign/threshold - confirm against the HUD's pitch line.
constexpr double side_eye_down_pitch_deg = 12.0;
}  // namespace head_pose

namespace huh {
constexpr double eye_wide_threshold = 0.01;
constexpr double jaw_threshold = 0.03;
}  // namespace huh

namespace dance {
constexpr double top_zone_y = 0.35;
constexpr double bottom_zone_y = 0.65;
}  // namespace dance

// see gesture_meme.py for why this is a trailing-window fraction trigger.
namespace spin {
constexpr int flow_width = 160;
constexpr int flow_height = 90;
constexpr double flow_noise_floor_px = 0.4;
constexpr double flow_min_moving_fraction = 0.15;
constexpr double mag_threshold = 0.65;
constexpr double fraction_window_ms = 2200.0;
constexpr double fraction_required = 0.55;
constexpr double flow_peak_hold_ms = 2000.0;
}  // namespace spin

namespace hand_cover_face {
constexpr double dist_face_lost = 1.3;
constexpr double dist_face_seen = 0.7;
}  // namespace hand_cover_face

namespace expression {
constexpr double mouth_open_jaw_threshold = 0.5;
constexpr double wink_threshold = 0.5;
}  // namespace expression

namespace smoothing {
constexpr double ema_alpha = 0.35;
}  // namespace smoothing

namespace shhh {
constexpr double mouth_dist = 0.55;
}  // namespace shhh

namespace two_fingers {
constexpr double tip_gap_factor = 1.4;
}  // namespace two_fingers

namespace two_hands {
constexpr double near_face_factor = 2.2;
constexpr double head_top_face_widths = 1.1;
}  // namespace two_hands

namespace hand_shape {
constexpr double thumb_out_spread = 1.05;
}  // namespace hand_shape

namespace hand {
// hand_scale is wrist-to-palm in normalized units; smaller is not hand-sized.
// Index-tip reach beyond this many palm lengths means non-hand landmarks.
constexpr double min_hand_scale = 0.03;
constexpr double max_reach_ratio = 4.0;
}  // namespace hand

}  // namespace tuning
}  // namespace miaucam
