// Tunable thresholds, kept in one place for on-page HUD tuning.
#pragma once

namespace miaucam {
namespace tuning {

constexpr int stable_frames_required = 5;
constexpr double default_fallback_ms = 600.0;
constexpr double face_stale_ms = 1200.0;

constexpr double side_eye_yaw_deg = 15.0;
// unvalidated sign/threshold - confirm against the HUD's pitch line.
constexpr double side_eye_down_pitch_deg = 12.0;

constexpr double eye_wide_threshold = 0.01;
constexpr double huh_jaw_threshold = 0.03;

constexpr double dance_top_zone_y = 0.35;
constexpr double dance_bottom_zone_y = 0.65;

// see gesture_meme.py for why this is a trailing-window fraction trigger.
constexpr int spin_flow_width = 160;
constexpr int spin_flow_height = 90;
constexpr double spin_flow_noise_floor_px = 0.4;
constexpr double spin_flow_min_moving_fraction = 0.15;
constexpr double spin_mag_threshold = 0.65;
constexpr double spin_fraction_window_ms = 2200.0;
constexpr double spin_fraction_required = 0.55;
constexpr double spin_flow_peak_hold_ms = 2000.0;

constexpr double hand_cover_face_dist_face_lost = 1.3;
constexpr double hand_cover_face_dist_face_seen = 0.7;

constexpr double mouth_open_jaw_threshold = 0.5;
constexpr double wink_threshold = 0.5;

// EMA alpha for smoothing face signals before decide() thresholds them.
constexpr double signal_ema_alpha = 0.35;

}  // namespace tuning
}  // namespace miaucam
