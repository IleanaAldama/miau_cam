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

struct BlendshapeScore {
    std::string name;
    float score = 0.0f;
};

struct FaceResult {
    bool has_face = false;
    std::vector<Point3> landmarks;  // 468 points, only if has_face
    std::vector<BlendshapeScore> blendshapes;
    bool has_transform = false;
    float transform[16] = {};  // row-major 4x4, transform[r * 4 + c]
};

// Opaque handle; defined only in miaucam_bridge.cc so mediapipe::Image
// never leaks out. shared_ptr, not unique_ptr, so it stays destructible
// from a TU that only has this forward declaration.
struct SharedMediaPipeFrame;

// Deep-copies rgb_data once; share the result across both sessions below.
std::shared_ptr<SharedMediaPipeFrame> CreateSharedFrame(const uint8_t* rgb_data, int width,
                                                          int height);

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
