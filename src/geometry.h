// Pure geometry helpers - no app-specific meaning, just vector math over
// landmark points. Ported from the JS/Python versions' geometry helpers.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <opencv2/core.hpp>

#include "miaucam_bridge.h"

namespace miaucam {

using Vec3 = cv::Point3f;

inline Vec3 p3(const Point3& p) { return Vec3(p.x, p.y, p.z); }

inline float dist(const Vec3& a, const Vec3& b) {
    return static_cast<float>(cv::norm(a - b));
}

inline double angle_deg(const Vec3& v1, const Vec3& v2) {
    double m1 = cv::norm(v1), m2 = cv::norm(v2);
    if (m1 < 1e-9 || m2 < 1e-9) return 180.0;
    double cos_a = std::clamp(v1.dot(v2) / (m1 * m2), -1.0, 1.0);
    return std::acos(cos_a) * 180.0 / M_PI;
}

inline bool finger_extended(const std::vector<Vec3>& pts, int mcp, int pip, int tip) {
    Vec3 v1 = pts[pip] - pts[mcp];
    Vec3 v2 = pts[tip] - pts[pip];
    return angle_deg(v1, v2) < 45.0;
}

// Extract yaw (left/right turn) from a row-major 4x4 facial transformation
// matrix - MediaPipe's own head pose estimate, not a hand-rolled heuristic.
inline double yaw_from_transform(const float m[16]) {
    auto r = [&](int i, int j) { return m[i * 4 + j]; };
    double sy = std::sqrt(r(0, 0) * r(0, 0) + r(1, 0) * r(1, 0));
    if (sy < 1e-6) return 0.0;
    return std::atan2(-r(2, 0), sy) * 180.0 / M_PI;
}

// Same idea, for pitch (up/down tilt). Unvalidated against real degrees the
// way yaw was - see tuning.h's side_eye_down_pitch_deg comment.
inline double pitch_from_transform(const float m[16]) {
    auto r = [&](int i, int j) { return m[i * 4 + j]; };
    return std::atan2(r(2, 1), r(2, 2)) * 180.0 / M_PI;
}

}  // namespace miaucam
