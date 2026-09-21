// Plain-C++ wrapper over MediaPipe's hand/face landmarking Tasks API - no
// protobuf or mediapipe:: types leak into this header.
#ifndef MIAUCAM_BRIDGE_H_
#define MIAUCAM_BRIDGE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace miaucam {

struct Point3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// 21 normalized landmarks: wrist=0, thumb=1-4, index=5-8, middle=9-12,
// ring=13-16, pinky=17-20.
struct Hand {
    std::vector<Point3> landmarks;
};

struct HandResult {
    std::vector<Hand> hands;
};

// The four face-mesh points the app reads: indices 13, 14, 234 and 454.
struct FaceKeypoints {
    Point3 upper_lip, lower_lip, right_cheek, left_cheek;
};

// The blendshape scores the app reads; 0 when the model reported none.
struct FaceExpression {
    float jaw_open = 0.0f;
    float smile_left = 0.0f;
    float smile_right = 0.0f;
    float brow_inner_up = 0.0f;
    float blink_left = 0.0f;
    float blink_right = 0.0f;
    float eye_wide_left = 0.0f;
    float eye_wide_right = 0.0f;
};

struct FaceResult {
    bool has_face = false;
    FaceKeypoints keypoints;
    FaceExpression expression;
    bool has_transform = false;
    float transform[16] = {};  // row-major 4x4, transform[r * 4 + c]
};

// Non-owning view over an RGB24 buffer someone else owns (a cv::Mat today,
// an Android camera buffer later) - never held past the call it's passed
// to. stride is the byte length of one row: width * 3 if tightly packed,
// wider if the source pads rows (as Android's camera buffers often do).
// Not a smart pointer on purpose: this doesn't own the memory, so wrapping
// it in unique_ptr/shared_ptr would misrepresent who's responsible for it.
struct RgbFrameView {
    const uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    int stride = 0;
};

// Opaque handle; defined only in miaucam_bridge.cc so mediapipe::Image
// never leaks out. shared_ptr, not unique_ptr, so it stays destructible
// from a TU that only has this forward declaration.
struct SharedMediaPipeFrame;

// Deep-copies frame once; share the result across both sessions below.
std::shared_ptr<SharedMediaPipeFrame> CreateSharedFrame(const RgbFrameView& frame);

// Create() returns null on failure and fills *error.
class HandLandmarkerSession {
public:
    ~HandLandmarkerSession();
    HandLandmarkerSession(const HandLandmarkerSession&) = delete;
    HandLandmarkerSession& operator=(const HandLandmarkerSession&) = delete;

    static std::unique_ptr<HandLandmarkerSession> Create(
        const std::string& model_path, int num_hands, std::string* error);

    HandResult DetectForVideo(const SharedMediaPipeFrame& frame, int64_t timestamp_ms);

private:
    HandLandmarkerSession();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Blendshapes + facial transformation matrices are always enabled.
class FaceLandmarkerSession {
public:
    ~FaceLandmarkerSession();
    FaceLandmarkerSession(const FaceLandmarkerSession&) = delete;
    FaceLandmarkerSession& operator=(const FaceLandmarkerSession&) = delete;

    static std::unique_ptr<FaceLandmarkerSession> Create(
        const std::string& model_path, int num_faces, std::string* error);

    FaceResult DetectForVideo(const SharedMediaPipeFrame& frame, int64_t timestamp_ms);

private:
    FaceLandmarkerSession();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace miaucam

#endif  // MIAUCAM_BRIDGE_H_
