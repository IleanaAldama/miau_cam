#include "gesture_state.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <variant>

#include "hand_classifier.h"
#include "tuning.h"

namespace miaucam {

namespace {

// Everything a rule may look at. face is set only while the face is fresh.
struct Situation {
    bool spinning;
    std::optional<FaceSignals> face;
    bool face_seen_now;
    HandsView hands;
};

using Rule = std::optional<Gesture> (*)(const Situation&);

bool no_hands(const Situation& s) { return std::holds_alternative<NoHands>(s.hands); }

// Also the fallback for a two-hand frame's first hand once no two-hand
// shape matched.
Gesture decide_single_hand_shape(const Situation& s, const HandInfo& h) {
    if (h.curled_count == 4) return Gesture::Fist;

    if (h.thumb_out && h.pinky_up && !h.index_up && !h.middle_up && !h.ring_up) {
        return Gesture::Rockstar;
    }

    // checked before hand-cover-face so shhh isn't swallowed by it.
    if (h.index_up && !h.middle_up && !h.ring_up && !h.pinky_up) {
        if (s.face) {
            float d = dist(h.index_tip, s.face->mouth_center) / s.face->face_width;
            if (d < tuning::shhh::mouth_dist) return Gesture::Shhh;
        }
        return Gesture::OneFingerUp;
    }

    // wider tolerance once the face detector fully loses the face.
    if (s.face) {
        float d = dist(h.palm_center, s.face->mouth_center) / s.face->face_width;
        double threshold = s.face_seen_now ? tuning::hand_cover_face::dist_face_seen
                                           : tuning::hand_cover_face::dist_face_lost;
        if (d < threshold) return Gesture::HandCoverFace;
    }

    if (h.curled_count == 0) return Gesture::HandStretchedOut;

    // let a strong side-eye read win over an ambiguous hand pose.
    if (s.face && std::abs(s.face->yaw_deg) > tuning::head_pose::side_eye_yaw_deg) {
        return Gesture::SideEyeCat;
    }

    return Gesture::Default;
}

// nullopt means the caller falls through to decide_single_hand_shape.
std::optional<Gesture> decide_two_hand_shape(const Situation& s, const HandInfo& a,
                                             const HandInfo& b) {
    if (is_pointing(a) && is_pointing(b)) {
        float avg_scale = (a.hand_scale + b.hand_scale) / 2.0f;
        float tip_gap = dist(a.index_tip, b.index_tip) / avg_scale;
        if (tip_gap < tuning::two_fingers::tip_gap_factor) return Gesture::TwoFingersTogether;
    }

    if (s.face) {
        const Vec3& mouth_center = s.face->mouth_center;
        float face_width = s.face->face_width;
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

// The rules, highest priority first.

// spinning in the chair beats everything else, hands included.
std::optional<Gesture> spin_rule(const Situation& s) {
    if (s.spinning) return Gesture::SpinCat;
    return std::nullopt;
}

// any hand shape counts; mouthOpenCat never clashes with huhCat (no hands).
std::optional<Gesture> mouth_open_rule(const Situation& s) {
    if (!no_hands(s) && s.face && s.face->jaw_open > tuning::expression::mouth_open_jaw_threshold) {
        return Gesture::MouthOpenCat;
    }
    return std::nullopt;
}

std::optional<Gesture> huh_rule(const Situation& s) {
    if (no_hands(s) && s.face && s.face->jaw_open > tuning::huh::jaw_threshold &&
        s.face->eye_wide > tuning::huh::eye_wide_threshold) {
        return Gesture::HuhCat;
    }
    return std::nullopt;
}

std::optional<Gesture> two_hand_rule(const Situation& s) {
    if (const auto* two = std::get_if<TwoHands>(&s.hands)) {
        return decide_two_hand_shape(s, two->first, two->second);
    }
    return std::nullopt;
}

std::optional<Gesture> single_hand_rule(const Situation& s) {
    if (const auto* one = std::get_if<OneHand>(&s.hands)) {
        return decide_single_hand_shape(s, one->hand);
    }
    if (const auto* two = std::get_if<TwoHands>(&s.hands)) {
        return decide_single_hand_shape(s, two->first);
    }
    return std::nullopt;
}

std::optional<Gesture> side_eye_rule(const Situation& s) {
    if (no_hands(s) && s.face && std::abs(s.face->yaw_deg) > tuning::head_pose::side_eye_yaw_deg) {
        return Gesture::SideEyeCat;
    }
    return std::nullopt;
}

std::optional<Gesture> side_eye_down_rule(const Situation& s) {
    if (no_hands(s) && s.face && s.face->pitch_deg > tuning::head_pose::side_eye_down_pitch_deg) {
        return Gesture::SideEyeDownCat;
    }
    return std::nullopt;
}

constexpr std::array<Rule, 7> rules = {
    spin_rule,       mouth_open_rule, huh_rule,           two_hand_rule,
    single_hand_rule, side_eye_rule,  side_eye_down_rule,
};

}  // namespace

std::optional<FaceSignals> fresh_face(const GestureState& state, double now_ms) {
    if (state.last_face && now_ms - state.last_face->t_ms < tuning::stability::face_stale_ms) {
        return state.last_face;
    }
    return std::nullopt;
}

GestureState update_flow(GestureState state, double magnitude, double coherence, double now_ms) {
    state.spin = update_spin_tracker(std::move(state.spin), magnitude, coherence, now_ms);
    return state;
}

GestureState update_face(GestureState state, const FaceResult& face_result, double now_ms) {
    state.face_seen_this_frame = face_result.has_face;

    auto raw = extract_face_signals(face_result, now_ms);
    if (!raw) return state;

    // smooth before any threshold comparison in decide().
    state.last_face = state.last_face
                          ? blend(*state.last_face, *raw, tuning::smoothing::ema_alpha)
                          : *raw;
    return state;
}

Gesture decide(const GestureState& state, const HandResult& hand_result, double now_ms) {
    const Situation situation{is_spinning(state.spin), fresh_face(state, now_ms),
                              state.face_seen_this_frame, classify_hands(hand_result)};

    for (Rule rule : rules) {
        if (auto gesture = rule(situation)) return *gesture;
    }
    return Gesture::Default;
}

}  // namespace miaucam
