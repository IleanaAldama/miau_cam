// C++ bridge exposing just enough of MediaPipe's Tasks C++ API (hand &
// face landmarking) for the miaucam CMake project to use, without pulling
// MediaPipe's internal headers/build graph into the CMake side. Built as a
// shared library via Bazel; consumed as a plain imported library from
// CMake. Every type here is plain C++ (no protobuf, no mediapipe::Image) so
// the header has zero MediaPipe dependencies.
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

// One detected hand: 21 normalized landmarks (same topology/order as
// MediaPipe's hand model - wrist=0, thumb=1-4, index=5-8, middle=9-12,
// ring=13-16, pinky=17-20).
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
    // 468 normalized face landmarks (only populated if has_face).
    std::vector<Point3> landmarks;
    // One score per blendshape category (empty if blendshapes weren't
    // requested or no face was seen).
    std::vector<BlendshapeScore> blendshapes;
    // Row-major 4x4 facial transformation matrix (identity-ish transform
    // absent if the model didn't return one). transform[r * 4 + c].
    bool has_transform = false;
    float transform[16] = {};
};

// A running HandLandmarker (video mode). Create() returns null on failure
// and fills *error with MediaPipe's status message.
class HandLandmarkerSession {
public:
    ~HandLandmarkerSession();
    HandLandmarkerSession(const HandLandmarkerSession&) = delete;
    HandLandmarkerSession& operator=(const HandLandmarkerSession&) = delete;

    static std::unique_ptr<HandLandmarkerSession> Create(
        const std::string& model_path, int num_hands, std::string* error);

    // rgb_data must be a contiguous, tightly packed RGB24 buffer
    // (width * height * 3 bytes, row-major, no padding).
    HandResult DetectForVideo(const uint8_t* rgb_data, int width, int height,
                               int64_t timestamp_ms);

private:
    HandLandmarkerSession();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// A running FaceLandmarker (video mode), with blendshapes + facial
// transformation matrices enabled.
class FaceLandmarkerSession {
public:
    ~FaceLandmarkerSession();
    FaceLandmarkerSession(const FaceLandmarkerSession&) = delete;
    FaceLandmarkerSession& operator=(const FaceLandmarkerSession&) = delete;

    static std::unique_ptr<FaceLandmarkerSession> Create(
        const std::string& model_path, int num_faces, std::string* error);

    FaceResult DetectForVideo(const uint8_t* rgb_data, int width, int height,
                               int64_t timestamp_ms);

private:
    FaceLandmarkerSession();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace miaucam

#endif  // MIAUCAM_BRIDGE_H_
