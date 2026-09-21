// Pure vector math over landmark points, no app-specific meaning.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

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

// Bent less than 45 degrees, compared by cosine to skip the acos.
template <class Points>
bool finger_extended(const Points& pts, int mcp, int pip, int tip) {
    constexpr double cos_45 = 0.70710678118654752;
    const Vec3 v1 = pts[pip] - pts[mcp];
    const Vec3 v2 = pts[tip] - pts[pip];
    const double m1 = cv::norm(v1), m2 = cv::norm(v2);
    if (m1 < 1e-9 || m2 < 1e-9) return false;
    return v1.dot(v2) / (m1 * m2) > cos_45;
}

// Yaw (left/right turn) from a row-major 4x4 facial transformation matrix.
inline double yaw_from_transform(const float m[16]) {
    auto r = [&](int i, int j) { return m[i * 4 + j]; };
    double sy = std::sqrt(r(0, 0) * r(0, 0) + r(1, 0) * r(1, 0));
    if (sy < 1e-6) return 0.0;
    return std::atan2(-r(2, 0), sy) * 180.0 / M_PI;
}

// Pitch (up/down tilt); sign unvalidated, see tuning.h.
inline double pitch_from_transform(const float m[16]) {
    auto r = [&](int i, int j) { return m[i * 4 + j]; };
    return std::atan2(r(2, 1), r(2, 2)) * 180.0 / M_PI;
}

}  // namespace miaucam
