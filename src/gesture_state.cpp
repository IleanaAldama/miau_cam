#include "gesture_state.h"

#include <algorithm>
#include <cmath>

#include "hand_classifier.h"
#include "tuning.h"

namespace miaucam {

namespace {

// std::visit overload set - pattern-matches on hand count.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

bool face_is_fresh(const GestureState& s, double now_ms) {
    return s.last_face.has_value() && (now_ms - s.last_face->t_ms) < tuning::stability::face_stale_ms;
}

// EMA; a nullopt previous reading is the first sighting (nothing to blend yet).
double smooth(std::optional<double> previous, double raw, double alpha) {
    return previous ? alpha * raw + (1.0 - alpha) * *previous : raw;
}

Vec3 smooth(std::optional<Vec3> previous, const Vec3& raw, double alpha) {
    if (!previous) return raw;
    return *previous * static_cast<float>(1.0 - alpha) + raw * static_cast<float>(alpha);
}

// Maps one FaceSnapshot field onto an optional predecessor; nullopt when no
// face has ever been seen, which the smooth* overloads treat as first data.
template <typename T>
std::optional<T> prev_field(const std::optional<FaceSnapshot>& prev, T FaceSnapshot::*member) {
    return prev ? std::optional<T>((*prev).*member) : std::nullopt;
}

// Also the fallback for a two-hand frame's first hand once no two-hand
// shape matched.
Gesture decide_single_hand_shape(const GestureState& state, const HandInfo& h, bool fresh) {
    if (h.curled_count == 4) return Gesture::Fist;

    if (h.thumb_out && h.pinky_up && !h.index_up && !h.middle_up && !h.ring_up) {
        return Gesture::Rockstar;
    }

    // checked before hand-cover-face so shhh isn't swallowed by it.
    if (h.index_up && !h.middle_up && !h.ring_up && !h.pinky_up) {
        if (fresh) {
            float d = dist(h.index_tip, state.last_face->mouth_center) / state.last_face->face_width;
            if (d < tuning::shhh::mouth_dist) return Gesture::Shhh;
        }
        return Gesture::OneFingerUp;
    }

    // wider tolerance once the face detector fully loses the face.
    if (fresh) {
        float d = dist(h.palm_center, state.last_face->mouth_center) / state.last_face->face_width;
        double threshold = state.face_seen_this_frame ? tuning::hand_cover_face::dist_face_seen
                                                        : tuning::hand_cover_face::dist_face_lost;
        if (d < threshold) return Gesture::HandCoverFace;
    }

    if (h.curled_count == 0) return Gesture::HandStretchedOut;

    // let a strong side-eye read win over an ambiguous hand pose.
    if (fresh && std::abs(state.last_face->yaw_deg) > tuning::head_pose::side_eye_yaw_deg) {
        return Gesture::SideEyeCat;
    }

    return Gesture::Default;
}

// nullopt means the caller falls through to decide_single_hand_shape.
std::optional<Gesture> decide_two_hand_shape(const GestureState& state, const HandInfo& a,
                                               const HandInfo& b, bool fresh) {
    if (is_pointing(a) && is_pointing(b)) {
        float avg_scale = (a.hand_scale + b.hand_scale) / 2.0f;
        float tip_gap = dist(a.index_tip, b.index_tip) / avg_scale;
        if (tip_gap < tuning::two_fingers::tip_gap_factor) return Gesture::TwoFingersTogether;
    }

    if (fresh) {
        const Vec3& mouth_center = state.last_face->mouth_center;
        float face_width = state.last_face->face_width;
        bool near_face = dist(a.palm_center, mouth_center) / face_width <
                             tuning::two_hands::near_face_factor &&
                          dist(b.palm_center, mouth_center) / face_width <
                             tuning::two_hands::near_face_factor;
        if (near_face) {
            float head_top_y = mouth_center.y - face_width * tuning::two_hands::head_top_face_widths;
            bool both_above_head = a.palm_center.y < head_top_y && b.palm_center.y < head_top_y;
            if (both_above_head) return Gesture::TwoHandsOnHead;

            // requires both fists; an open hand near the face falls through.
            if (a.curled_count == 4 && b.curled_count == 4) return Gesture::CrashOutCat;
        }
    }

    // one hand near the top of the frame, one near the bottom, either order.
    if (a.curled_count == 0 && b.curled_count == 0) {
        float top = std::min(a.palm_center.y, b.palm_center.y);
        float bottom = std::max(a.palm_center.y, b.palm_center.y);
        if (top < tuning::dance::top_zone_y && bottom > tuning::dance::bottom_zone_y) {
            return Gesture::DanceCat;
        }
    }

    return std::nullopt;
}

}  // namespace

GestureState update_flow(GestureState state, double magnitude, double coherence, double now_ms) {
    state.spin = update_spin_tracker(std::move(state.spin), magnitude, coherence, now_ms);
    return state;
}

GestureState update_face(GestureState state, const FaceResult& face_result, double now_ms) {
    state.face_seen_this_frame = face_result.has_face;

    auto snap = extract_face_snapshot(face_result, now_ms);
    if (!snap) return state;

    // Blend against the previous frame's snapshot; a missing predecessor just
    // passes the raw reading straight through as first data.
    double alpha = tuning::smoothing::ema_alpha;
    snap->mouth_center =
        smooth(prev_field(state.last_face, &FaceSnapshot::mouth_center), snap->mouth_center, alpha);
    snap->face_width =
        smooth(prev_field(state.last_face, &FaceSnapshot::face_width), snap->face_width, alpha);
    snap->mouth_open =
        smooth(prev_field(state.last_face, &FaceSnapshot::mouth_open), snap->mouth_open, alpha);
    snap->yaw_deg = smooth(prev_field(state.last_face, &FaceSnapshot::yaw_deg), snap->yaw_deg, alpha);
    state.last_face = snap;
    state.last_yaw_debug = snap->yaw_deg;

    if (face_result.has_transform) {
        double raw_pitch = pitch_from_transform(face_result.transform);
        state.last_pitch_debug = smooth(state.last_pitch_debug, raw_pitch, alpha);
    }

    auto scores = blendshape_map(face_result);
    auto get = [&](const char* k) {
        auto it = scores.find(k);
        return it == scores.end() ? 0.0f : it->second;
    };
    state.last_jaw_open_debug = smooth(state.last_jaw_open_debug, get("jawOpen"), alpha);
    state.last_smile_debug = smooth(state.last_smile_debug,
                                    std::max(get("mouthSmileLeft"), get("mouthSmileRight")), alpha);
    state.last_brow_raise_debug = smooth(state.last_brow_raise_debug, get("browInnerUp"), alpha);
    state.last_wink_debug = smooth(state.last_wink_debug, wink_score(scores), alpha);
    state.last_eye_wide_debug = smooth(state.last_eye_wide_debug, eye_wide_score(scores), alpha);

    return state;
}

Gesture decide(const GestureState& state, const HandResult& hand_result, double now_ms) {
    // spinning in the chair beats everything else, hands included.
    if (is_spinning(state.spin)) return Gesture::SpinCat;

    bool fresh = face_is_fresh(state, now_ms);
    HandsView view = classify_hands(hand_result);

    return std::visit(
        overloaded{
            [&](const NoHands&) -> Gesture {
                // mouthOpenCat lives in OneHand/TwoHands only, so it never
                // clashes with huhCat (mouth+no-hand vs mouth+hand).
                if (fresh && state.last_jaw_open_debug.value_or(0.0) > tuning::huh::jaw_threshold &&
                    state.last_eye_wide_debug.value_or(0.0) > tuning::huh::eye_wide_threshold) {
                    return Gesture::HuhCat;
                }
                if (fresh && std::abs(state.last_face->yaw_deg) > tuning::head_pose::side_eye_yaw_deg) {
                    return Gesture::SideEyeCat;
                }
                if (fresh && state.last_pitch_debug.value_or(0.0) >
                                tuning::head_pose::side_eye_down_pitch_deg) {
                    return Gesture::SideEyeDownCat;
                }
                return Gesture::Default;
            },
            [&](const OneHand& one) -> Gesture {
                // any hand shape counts; checked before hand-shape logic.
                if (fresh && state.last_jaw_open_debug.value_or(0.0) >
                                tuning::expression::mouth_open_jaw_threshold) {
                    return Gesture::MouthOpenCat;
                }
                return decide_single_hand_shape(state, one.hand, fresh);
            },
            [&](const TwoHands& two) -> Gesture {
                if (fresh && state.last_jaw_open_debug.value_or(0.0) >
                                tuning::expression::mouth_open_jaw_threshold) {
                    return Gesture::MouthOpenCat;
                }
                if (auto g = decide_two_hand_shape(state, two.first, two.second, fresh)) {
                    return *g;
                }
                return decide_single_hand_shape(state, two.first, fresh);
            },
        },
        view);
}

}  // namespace miaucam
