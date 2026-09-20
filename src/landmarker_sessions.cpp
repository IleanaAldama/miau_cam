#include "landmarker_sessions.h"

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

DetectionResult detect(LandmarkerSessions& sessions, const RgbFrameView& frame,
                        int64_t timestamp_ms) {
    auto shared_frame = CreateSharedFrame(frame);

    DetectionResult result;
    result.hand = sessions.hand->DetectForVideo(*shared_frame, timestamp_ms);
    result.face = sessions.face->DetectForVideo(*shared_frame, timestamp_ms);
    return result;
}

}  // namespace miaucam
