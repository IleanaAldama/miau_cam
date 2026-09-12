#include "face_signals.h"

#include <algorithm>
#include <cmath>

#include "tuning.h"

namespace miaucam {

std::optional<FaceSnapshot> extract_face_snapshot(const FaceResult& face_result,
                                                    double now_ms) {
    if (!face_result.has_face) return std::nullopt;

    const auto& f = face_result.landmarks;
    Vec3 upper_lip = p3(f[13]), lower_lip = p3(f[14]);
    Vec3 right_cheek = p3(f[234]), left_cheek = p3(f[454]);

    FaceSnapshot snap;
    snap.mouth_center = (upper_lip + lower_lip) * 0.5f;
    snap.face_width = dist(right_cheek, left_cheek);
    snap.mouth_open = dist(upper_lip, lower_lip) / snap.face_width;
    snap.yaw_deg = face_result.has_transform ? yaw_from_transform(face_result.transform) : 0.0;
    snap.t_ms = now_ms;
    return snap;
}

std::unordered_map<std::string, float> blendshape_map(const FaceResult& f) {
    std::unordered_map<std::string, float> m;
    for (const auto& b : f.blendshapes) m[b.name] = b.score;
    return m;
}

namespace {
float score_or_zero(const std::unordered_map<std::string, float>& scores, const char* key) {
    auto it = scores.find(key);
    return it == scores.end() ? 0.0f : it->second;
}
}  // namespace

double wink_score(const std::unordered_map<std::string, float>& scores) {
    float left = score_or_zero(scores, "eyeBlinkLeft");
    float right = score_or_zero(scores, "eyeBlinkRight");
    if (std::max(left, right) < tuning::wink_threshold) return 0.0;
    return std::abs(left - right);
}

double eye_wide_score(const std::unordered_map<std::string, float>& scores) {
    return std::max(score_or_zero(scores, "eyeWideLeft"), score_or_zero(scores, "eyeWideRight"));
}

}  // namespace miaucam
