// Owns the two MediaPipe sessions behind a single detect() call. Takes RGB
// bytes directly - no OpenCV/platform assumptions - so it works the same
// from any frame source (cv::VideoCapture+cvtColor today, Android's camera
// stack later).
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "frame_pipeline.h"
#include "miaucam_bridge.h"
#include "result.h"

namespace miaucam {

struct LandmarkerSessions {
    std::unique_ptr<HandLandmarkerSession> hand;
    std::unique_ptr<FaceLandmarkerSession> face;
};

Result<LandmarkerSessions> create_landmarker_sessions(const std::string& models_dir);

DetectionResult detect(LandmarkerSessions& sessions, const RgbFrameView& frame,
                        int64_t timestamp_ms);

}  // namespace miaucam
