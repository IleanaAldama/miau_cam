// "What is the face doing": mouth and cheek geometry, head pose and the
// expression scores, smoothed into one value that decisions and the HUD read.
#pragma once

#include <optional>

#include "geometry.h"
#include "miaucam_bridge.h"

namespace miaucam {

struct FaceSignals {
    Vec3 mouth_center;
    float face_width = 0.0f;
    float mouth_open = 0.0f;
    double yaw_deg = 0.0;
    double pitch_deg = 0.0;
    double jaw_open = 0.0;
    double smile = 0.0;
    double brow_raise = 0.0;
    double wink = 0.0;
    double eye_wide = 0.0;
    double t_ms = 0.0;
};

// nullopt if the result carries no face.
std::optional<FaceSignals> extract_face_signals(const FaceResult& face_result, double now_ms);

// Whole-struct EMA: alpha is the weight of raw. The timestamp is always raw's.
FaceSignals blend(const FaceSignals& previous, const FaceSignals& raw, double alpha);

// gap between the two blink scores, once one eye is clearly closing.
double wink_score(const FaceExpression& expression);

// max(eyeWideLeft, eyeWideRight).
double eye_wide_score(const FaceExpression& expression);

}  // namespace miaucam
