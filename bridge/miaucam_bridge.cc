#include "mediapipe/examples/desktop/miaucam/miaucam_bridge.h"

#include <utility>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/core/base_options.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/face_landmarker/face_landmarker.h"
#include "mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h"

namespace miaucam {

namespace {

mediapipe::Image MakeImage(const uint8_t* rgb_data, int width, int height) {
    auto frame = std::make_shared<mediapipe::ImageFrame>();
    frame->CopyPixelData(mediapipe::ImageFormat::SRGB, width, height,
                          width * 3, rgb_data,
                          mediapipe::ImageFrame::kDefaultAlignmentBoundary);
    return mediapipe::Image(std::move(frame));
}

}  // namespace

// ---------------------------------------------------------------------------
// HandLandmarkerSession
// ---------------------------------------------------------------------------

struct HandLandmarkerSession::Impl {
    std::unique_ptr<mediapipe::tasks::vision::hand_landmarker::HandLandmarker>
        landmarker;
};

HandLandmarkerSession::HandLandmarkerSession() : impl_(new Impl()) {}
HandLandmarkerSession::~HandLandmarkerSession() = default;

std::unique_ptr<HandLandmarkerSession> HandLandmarkerSession::Create(
    const std::string& model_path, int num_hands, std::string* error) {
    auto options = std::make_unique<
        mediapipe::tasks::vision::hand_landmarker::HandLandmarkerOptions>();
    options->base_options.model_asset_path = model_path;
    options->running_mode = mediapipe::tasks::vision::core::RunningMode::VIDEO;
    options->num_hands = num_hands;

    auto landmarker =
        mediapipe::tasks::vision::hand_landmarker::HandLandmarker::Create(
            std::move(options));
    if (!landmarker.ok()) {
        if (error) *error = landmarker.status().ToString();
        return nullptr;
    }

    std::unique_ptr<HandLandmarkerSession> session(new HandLandmarkerSession());
    session->impl_->landmarker = std::move(*landmarker);
    return session;
}

HandResult HandLandmarkerSession::DetectForVideo(const uint8_t* rgb_data,
                                                   int width, int height,
                                                   int64_t timestamp_ms) {
    HandResult result;
    auto detection = impl_->landmarker->DetectForVideo(
        MakeImage(rgb_data, width, height), timestamp_ms);
    if (!detection.ok()) return result;

    for (const auto& hand_landmarks : detection->hand_landmarks) {
        Hand hand;
        hand.landmarks.reserve(hand_landmarks.landmarks.size());
        for (const auto& lm : hand_landmarks.landmarks) {
            hand.landmarks.push_back(Point3{lm.x, lm.y, lm.z});
        }
        result.hands.push_back(std::move(hand));
    }
    return result;
}

// ---------------------------------------------------------------------------
// FaceLandmarkerSession
// ---------------------------------------------------------------------------

struct FaceLandmarkerSession::Impl {
    std::unique_ptr<mediapipe::tasks::vision::face_landmarker::FaceLandmarker>
        landmarker;
};

FaceLandmarkerSession::FaceLandmarkerSession() : impl_(new Impl()) {}
FaceLandmarkerSession::~FaceLandmarkerSession() = default;

std::unique_ptr<FaceLandmarkerSession> FaceLandmarkerSession::Create(
    const std::string& model_path, int num_faces, std::string* error) {
    auto options = std::make_unique<
        mediapipe::tasks::vision::face_landmarker::FaceLandmarkerOptions>();
    options->base_options.model_asset_path = model_path;
    options->running_mode = mediapipe::tasks::vision::core::RunningMode::VIDEO;
    options->num_faces = num_faces;
    options->output_face_blendshapes = true;
    options->output_facial_transformation_matrixes = true;

    auto landmarker =
        mediapipe::tasks::vision::face_landmarker::FaceLandmarker::Create(
            std::move(options));
    if (!landmarker.ok()) {
        if (error) *error = landmarker.status().ToString();
        return nullptr;
    }

    std::unique_ptr<FaceLandmarkerSession> session(new FaceLandmarkerSession());
    session->impl_->landmarker = std::move(*landmarker);
    return session;
}

FaceResult FaceLandmarkerSession::DetectForVideo(const uint8_t* rgb_data,
                                                   int width, int height,
                                                   int64_t timestamp_ms) {
    FaceResult result;
    auto detection = impl_->landmarker->DetectForVideo(
        MakeImage(rgb_data, width, height), timestamp_ms);
    if (!detection.ok() || detection->face_landmarks.empty()) return result;

    result.has_face = true;
    const auto& face = detection->face_landmarks[0];
    result.landmarks.reserve(face.landmarks.size());
    for (const auto& lm : face.landmarks) {
        result.landmarks.push_back(Point3{lm.x, lm.y, lm.z});
    }

    if (detection->face_blendshapes.has_value() &&
        !detection->face_blendshapes->empty()) {
        for (const auto& category : (*detection->face_blendshapes)[0].categories) {
            BlendshapeScore score;
            score.name = category.category_name.value_or("");
            score.score = category.score;
            result.blendshapes.push_back(std::move(score));
        }
    }

    if (detection->facial_transformation_matrixes.has_value() &&
        !detection->facial_transformation_matrixes->empty()) {
        const auto& matrix = (*detection->facial_transformation_matrixes)[0];
        result.has_transform = true;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                result.transform[r * 4 + c] = matrix(r, c);
            }
        }
    }

    return result;
}

}  // namespace miaucam
