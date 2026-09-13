// All tunable thresholds in one place, grouped by domain. gesture_meme.py
// kept these together on purpose: you tune most of them by eye, watching
// the debug HUD while making the pose the threshold is supposed to catch.
// Splitting them next to their owning module would read cleaner, but you'd
// lose the "everything to retune is on one page" workflow, so they stay
// here instead.
#pragma once

namespace miaucam {
namespace tuning {

// gesture stability / timing
constexpr int stable_frames_required = 5;
constexpr double default_fallback_ms = 600.0;
constexpr double face_stale_ms = 1200.0;

// head pose (yaw = turned sideways, pitch = tilted down)
constexpr double side_eye_yaw_deg = 15.0;
// unvalidated against real degrees the way yaw was - watch the HUD's pitch
// line while looking down to confirm the sign and find your own threshold.
constexpr double side_eye_down_pitch_deg = 12.0;

// huh cat: mouth open AND eyes wide. Kept low on purpose - real test
// readings only cleared the bar by a thin margin, and the 5-consecutive-
// frame stability requirement needs a real buffer against per-frame jitter.
constexpr double eye_wide_threshold = 0.01;
constexpr double huh_jaw_threshold = 0.03;

// danceCat: one open hand near the top of the screen, the other near the
// bottom - absolute frame position (0 = top edge, 1 = bottom edge).
constexpr double dance_top_zone_y = 0.35;
constexpr double dance_bottom_zone_y = 0.65;

// spin detection: tuned from two real recorded sessions
// (flow_debug_log.csv, regenerated each run). See gesture_meme.py for the
// full rationale on why this is a "fraction of a trailing window" trigger
// rather than a single-burst one.
constexpr int spin_flow_width = 160;
constexpr int spin_flow_height = 90;
constexpr double spin_flow_noise_floor_px = 0.4;
constexpr double spin_flow_min_moving_fraction = 0.15;
constexpr double spin_mag_threshold = 0.65;
constexpr double spin_fraction_window_ms = 2200.0;
constexpr double spin_fraction_required = 0.55;
constexpr double spin_flow_peak_hold_ms = 2000.0;

// hand-covering-face: how close the hand needs to be to where the mouth
// last was. Wider when the face detector has fully lost the face (strong
// evidence of a real occlusion); tighter when still partially tracked.
constexpr double hand_cover_face_dist_face_lost = 1.3;
constexpr double hand_cover_face_dist_face_seen = 0.7;

// facial expressions (mouth-open, wink) read MediaPipe's face blendshapes.
constexpr double mouth_open_jaw_threshold = 0.5;
constexpr double wink_threshold = 0.5;

// Landmark positions jitter a few pixels frame to frame even when holding
// still, which is enough noise to flip a threshold-based gesture (e.g.
// handCoverFace) on and off before the stability counter can settle. All
// continuous face signals (yaw, pitch, jaw-open, mouth/hand distances, ...)
// get smoothed with an exponential moving average before decide() ever
// compares them to a threshold: smoothed = alpha*new + (1-alpha)*previous.
// Lower alpha = smoother but slower to react to a real gesture change;
// higher alpha = snappier but noisier. 0.35 was picked to kill single-frame
// jitter without feeling laggy - retune by watching the HUD's numbers
// steady out (or not) while holding a pose still.
constexpr double signal_ema_alpha = 0.35;

}  // namespace tuning
}  // namespace miaucam
