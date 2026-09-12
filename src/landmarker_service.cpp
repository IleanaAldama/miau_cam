#include "landmarker_service.h"

#include <opencv2/imgproc.hpp>

namespace miaucam {

Result<LandmarkerService> create_landmarker_service(const std::string& models_dir) {
    std::string error;

    auto hand = HandLandmarkerSession::Create(models_dir + "/hand_landmarker.task",
                                               /*num_hands=*/2, &error);
    if (!hand) return Result<LandmarkerService>::Err("HandLandmarker: " + error);

    auto face = FaceLandmarkerSession::Create(models_dir + "/face_landmarker.task",
                                               /*num_faces=*/1, &error);
    if (!face) return Result<LandmarkerService>::Err("FaceLandmarker: " + error);

    return Result<LandmarkerService>::Ok(LandmarkerService{std::move(hand), std::move(face)});
}

DetectionResult detect(LandmarkerService& service, const cv::Mat& bgr_frame, int64_t timestamp_ms) {
    cv::Mat rgb;
    cv::cvtColor(bgr_frame, rgb, cv::COLOR_BGR2RGB);

    DetectionResult result;
    result.hand = service.hand->DetectForVideo(rgb.data, rgb.cols, rgb.rows, timestamp_ms);
    result.face = service.face->DetectForVideo(rgb.data, rgb.cols, rgb.rows, timestamp_ms);
    return result;
}

}  // namespace miaucam
