// Owns the two MediaPipe sessions; hides BGR->RGB + timestamp plumbing
// behind detect().
#pragma once

#include <memory>
#include <string>

#include <opencv2/core.hpp>

#include "miaucam_bridge.h"
#include "result.h"

namespace miaucam {

struct LandmarkerSessions {
    std::unique_ptr<HandLandmarkerSession> hand;
    std::unique_ptr<FaceLandmarkerSession> face;
};

Result<LandmarkerSessions> create_landmarker_sessions(const std::string& models_dir);

struct DetectionResult {
    HandResult hand;
    FaceResult face;
};

// bgr_frame must be a tightly-packed cv::Mat, not a sub-matrix/ROI.
DetectionResult detect(LandmarkerSessions& sessions, const cv::Mat& bgr_frame, int64_t timestamp_ms);

}  // namespace miaucam
