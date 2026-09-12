// Owns the two MediaPipe sessions and hides the pixel-format plumbing
// (BGR->RGB, timestamp bookkeeping) behind a single detect() call. main.cpp
// never touches raw pixel buffers directly.
#pragma once

#include <memory>
#include <string>

#include <opencv2/core.hpp>

#include "miaucam_bridge.h"
#include "result.h"

namespace miaucam {

struct LandmarkerService {
    std::unique_ptr<HandLandmarkerSession> hand;
    std::unique_ptr<FaceLandmarkerSession> face;
};

Result<LandmarkerService> create_landmarker_service(const std::string& models_dir);

struct DetectionResult {
    HandResult hand;
    FaceResult face;
};

// bgr_frame must be a standard, tightly-packed cv::Mat (as produced by
// cv::VideoCapture::read) - not a sub-matrix/ROI.
DetectionResult detect(LandmarkerService& service, const cv::Mat& bgr_frame, int64_t timestamp_ms);

}  // namespace miaucam
