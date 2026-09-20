// Flat landmark arrays from a platform's own MediaPipe Tasks binding,
// turned into the DetectionResult that advance() consumes.
#pragma once

#include <string>
#include <vector>

#include "frame_pipeline.h"

namespace miaucam {

struct RawDetection {
    std::vector<float> hands;  // xyz per landmark, 21 landmarks per hand
    std::vector<float> face;   // xyz per landmark
    std::vector<std::string> blendshape_names;
    std::vector<float> blendshape_scores;
    std::vector<float> transform;  // 16 floats row-major, or empty
};

DetectionResult to_detection(const RawDetection& raw);

}  // namespace miaucam
