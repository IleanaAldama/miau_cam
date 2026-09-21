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
constexpr float kMinHandConfidence = 0.6f;

template <class Landmark>
Point3 to_point(const Landmark& lm) {
    return Point3{lm.x, lm.y, lm.z};
}

float* expression_slot(FaceExpression& e, const std::string& name) {
    if (name == "jawOpen") return &e.jaw_open;
    if (name == "mouthSmileLeft") return &e.smile_left;
    if (name == "mouthSmileRight") return &e.smile_right;
    if (name == "browInnerUp") return &e.brow_inner_up;
    if (name == "eyeBlinkLeft") return &e.blink_left;
    if (name == "eyeBlinkRight") return &e.blink_right;
    if (name == "eyeWideLeft") return &e.eye_wide_left;
    if (name == "eyeWideRight") return &e.eye_wide_right;
    return nullptr;
}
}  // namespace

struct SharedMediaPipeFrame {
    mediapipe::Image image;
};

std::shared_ptr<SharedMediaPipeFrame> CreateSharedFrame(const RgbFrameView& frame_view) {
    auto image_frame = std::make_shared<mediapipe::ImageFrame>();
    image_frame->CopyPixelData(mediapipe::ImageFormat::SRGB, frame_view.width, frame_view.height,
                                frame_view.stride, frame_view.data,
                                mediapipe::ImageFrame::kDefaultAlignmentBoundary);

    auto frame = std::make_shared<SharedMediaPipeFrame>();
    frame->image = mediapipe::Image(std::move(image_frame));
    return frame;
}

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
    options->min_hand_detection_confidence = kMinHandConfidence;
    options->min_hand_presence_confidence = kMinHandConfidence;

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

HandResult HandLandmarkerSession::DetectForVideo(const SharedMediaPipeFrame& frame,
                                                   int64_t timestamp_ms) {
    HandResult result;
    auto detection = impl_->landmarker->DetectForVideo(frame.image, timestamp_ms);
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

FaceResult FaceLandmarkerSession::DetectForVideo(const SharedMediaPipeFrame& frame,
                                                   int64_t timestamp_ms) {
    FaceResult result;
    auto detection = impl_->landmarker->DetectForVideo(frame.image, timestamp_ms);
    if (!detection.ok() || detection->face_landmarks.empty()) return result;

    const auto& face = detection->face_landmarks[0].landmarks;
    if (face.size() <= 454) return result;

    result.has_face = true;
    result.keypoints = {to_point(face[13]), to_point(face[14]), to_point(face[234]),
                        to_point(face[454])};

    if (detection->face_blendshapes.has_value() &&
        !detection->face_blendshapes->empty()) {
        for (const auto& category : (*detection->face_blendshapes)[0].categories) {
            if (float* slot = expression_slot(result.expression, category.category_name.value_or(""))) {
                *slot = category.score;
            }
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
