// Flat landmark arrays from a platform's own MediaPipe Tasks binding,
// turned into the DetectionResult that advance() consumes.
#pragma once

#include <vector>

#include "frame_pipeline.h"

namespace miaucam {

struct RawDetection {
    std::vector<float> hands;  // xyz per landmark, 21 landmarks per hand
    std::vector<float> face;        // xyz of upper lip, lower lip, right cheek, left cheek
    std::vector<float> expression;  // jaw, smile L/R, brow, blink L/R, eye wide L/R
    std::vector<float> transform;  // 16 floats, either layout, or empty
};

DetectionResult to_detection(const RawDetection& raw);

}  // namespace miaucam
