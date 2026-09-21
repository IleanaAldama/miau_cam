#include "detection_input.h"

#include <algorithm>
#include <cmath>

namespace miaucam {

namespace {

constexpr size_t floats_per_point = 3;
constexpr size_t landmarks_per_hand = 21;
constexpr size_t face_keypoint_floats = 4 * floats_per_point;
constexpr size_t expression_size = 8;
constexpr size_t transform_size = 16;

std::vector<Point3> to_points(std::vector<float>::const_iterator first,
                              std::vector<float>::const_iterator last) {
    std::vector<Point3> points;
    for (; last - first >= static_cast<long>(floats_per_point); first += floats_per_point) {
        points.push_back({first[0], first[1], first[2]});
    }
    return points;
}

FaceKeypoints to_keypoints(const std::vector<float>& f) {
    auto at = [&](size_t i) { return Point3{f[i * 3], f[i * 3 + 1], f[i * 3 + 2]}; };
    return {at(0), at(1), at(2), at(3)};
}

FaceExpression to_expression(const std::vector<float>& e) {
    return {e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]};
}

// A pose matrix has its translation in one edge and zeros on the other, which
// tells row-major (right column) from column-major (bottom row).
std::vector<float> to_row_major(const std::vector<float>& m) {
    const float right_column = std::abs(m[3]) + std::abs(m[7]) + std::abs(m[11]);
    const float bottom_row = std::abs(m[12]) + std::abs(m[13]) + std::abs(m[14]);
    if (right_column >= bottom_row) return m;

    std::vector<float> out(transform_size);
    for (size_t i = 0; i < transform_size; ++i) out[i] = m[(i % 4) * 4 + i / 4];
    return out;
}

}  // namespace

DetectionResult to_detection(const RawDetection& raw) {
    DetectionResult detection;

    const size_t floats_per_hand = landmarks_per_hand * floats_per_point;
    for (size_t i = 0; i + floats_per_hand <= raw.hands.size(); i += floats_per_hand) {
        detection.hand.hands.push_back(
            {to_points(raw.hands.begin() + i, raw.hands.begin() + i + floats_per_hand)});
    }

    if (raw.face.size() == face_keypoint_floats) {
        detection.face.has_face = true;
        detection.face.keypoints = to_keypoints(raw.face);
    }

    if (raw.expression.size() == expression_size) {
        detection.face.expression = to_expression(raw.expression);
    }

    if (raw.transform.size() == transform_size) {
        detection.face.has_transform = true;
        const auto row_major = to_row_major(raw.transform);
        std::copy(row_major.begin(), row_major.end(), detection.face.transform);
    }
    return detection;
}

}  // namespace miaucam
