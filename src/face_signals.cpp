#include "face_signals.h"

#include <algorithm>
#include <cmath>

#include "tuning.h"

namespace miaucam {

std::optional<FaceSignals> extract_face_signals(const FaceResult& face_result, double now_ms) {
    if (!face_result.has_face) return std::nullopt;

    const FaceKeypoints& k = face_result.keypoints;
    const FaceExpression& e = face_result.expression;
    const Vec3 upper_lip = p3(k.upper_lip), lower_lip = p3(k.lower_lip);

    FaceSignals s;
    s.mouth_center = (upper_lip + lower_lip) * 0.5f;
    s.face_width = dist(p3(k.right_cheek), p3(k.left_cheek));
    s.mouth_open = dist(upper_lip, lower_lip) / s.face_width;
    if (face_result.has_transform) {
        s.yaw_deg = yaw_from_transform(face_result.transform);
        s.pitch_deg = pitch_from_transform(face_result.transform);
    }
    s.jaw_open = e.jaw_open;
    s.smile = std::max(e.smile_left, e.smile_right);
    s.brow_raise = e.brow_inner_up;
    s.wink = wink_score(e);
    s.eye_wide = eye_wide_score(e);
    s.t_ms = now_ms;
    return s;
}

FaceSignals blend(const FaceSignals& previous, const FaceSignals& raw, double alpha) {
    auto mix = [&](double before, double now) { return alpha * now + (1.0 - alpha) * before; };

    FaceSignals out;
    out.mouth_center = previous.mouth_center * static_cast<float>(1.0 - alpha) +
                       raw.mouth_center * static_cast<float>(alpha);
    out.face_width = static_cast<float>(mix(previous.face_width, raw.face_width));
    out.mouth_open = static_cast<float>(mix(previous.mouth_open, raw.mouth_open));
    out.yaw_deg = mix(previous.yaw_deg, raw.yaw_deg);
    out.pitch_deg = mix(previous.pitch_deg, raw.pitch_deg);
    out.jaw_open = mix(previous.jaw_open, raw.jaw_open);
    out.smile = mix(previous.smile, raw.smile);
    out.brow_raise = mix(previous.brow_raise, raw.brow_raise);
    out.wink = mix(previous.wink, raw.wink);
    out.eye_wide = mix(previous.eye_wide, raw.eye_wide);
    out.t_ms = raw.t_ms;
    return out;
}

double wink_score(const FaceExpression& e) {
    if (std::max(e.blink_left, e.blink_right) < tuning::expression::wink_threshold) return 0.0;
    return std::abs(e.blink_left - e.blink_right);
}

double eye_wide_score(const FaceExpression& e) {
    return std::max(e.eye_wide_left, e.eye_wide_right);
}

}  // namespace miaucam
