#include "landmarker_sessions.h"

#include <opencv2/imgproc.hpp>

namespace miaucam {

Result<LandmarkerSessions> create_landmarker_sessions(const std::string& models_dir) {
    std::string error;

    auto hand = HandLandmarkerSession::Create(models_dir + "/hand_landmarker.task",
                                               /*num_hands=*/2, &error);
    if (!hand) return Result<LandmarkerSessions>::Err("HandLandmarker: " + error);

    auto face = FaceLandmarkerSession::Create(models_dir + "/face_landmarker.task",
                                               /*num_faces=*/1, &error);
    if (!face) return Result<LandmarkerSessions>::Err("FaceLandmarker: " + error);

    return Result<LandmarkerSessions>::Ok(LandmarkerSessions{std::move(hand), std::move(face)});
}

DetectionResult detect(LandmarkerSessions& sessions, const cv::Mat& bgr_frame, int64_t timestamp_ms) {
    cv::Mat rgb;
    cv::cvtColor(bgr_frame, rgb, cv::COLOR_BGR2RGB);

    DetectionResult result;
    result.hand = sessions.hand->DetectForVideo(rgb.data, rgb.cols, rgb.rows, timestamp_ms);
    result.face = sessions.face->DetectForVideo(rgb.data, rgb.cols, rgb.rows, timestamp_ms);
    return result;
}

}  // namespace miaucam
