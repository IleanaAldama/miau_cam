// "What is the face doing" - mouth/cheek geometry and blendshape scores,
// independent of what gesture they map to.
#pragma once

#include <optional>
#include <unordered_map>

#include "geometry.h"
#include "miaucam_bridge.h"

namespace miaucam {

struct FaceSnapshot {
    Vec3 mouth_center;
    float face_width = 0.0f;
    float mouth_open = 0.0f;
    double yaw_deg = 0.0;
    double t_ms = 0.0;
};

// nullopt if the result carries no face.
std::optional<FaceSnapshot> extract_face_snapshot(const FaceResult& face_result, double now_ms);

std::unordered_map<std::string, float> blendshape_map(const FaceResult& f);

// One eye closed, the other open - the absolute gap between the two blink
// scores, only counted once at least one eye is clearly closing (otherwise
// two half-lowered eyes would also read as a "gap").
double wink_score(const std::unordered_map<std::string, float>& scores);

// max(eyeWideLeft, eyeWideRight) - for huhCat (mouth open AND eyes wide,
// distinct from mouthOpenCat which is mouth-open alone).
double eye_wide_score(const std::unordered_map<std::string, float>& scores);

}  // namespace miaucam
