#include "gesture_state.h"

#include <algorithm>
#include <cmath>

#include "hand_classifier.h"
#include "tuning.h"

namespace miaucam {

namespace {

// Poor man's pattern matching: an overload set built from lambdas, used
// with std::visit below so the hand-count dispatch reads as "for this
// shape of input, do this" instead of an if/else on hands.size().
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

bool face_is_fresh(const GestureState& s, double now_ms) {
    return s.last_face.has_value() && (now_ms - s.last_face->t_ms) < tuning::face_stale_ms;
}

// fist / rockstar / shhh / one-finger-up / hand-covering-face /
// hand-stretched-out / side-eye-as-fallback / default, in that order. This
// is what a single hand gets checked against - and it's also what the
// *first* hand of a two-hand frame falls back to once neither
// twoFingersTogether, twoHandsOnHead, crashOutCat nor danceCat matched.
Gesture decide_single_hand_shape(const GestureState& state, const HandInfo& h, bool fresh) {
    if (h.curled_count == 4) return Gesture::Fist;

    if (h.thumb_out && h.pinky_up && !h.index_up && !h.middle_up && !h.ring_up) {
        return Gesture::Rockstar;
    }

    // shhh / one-finger-up: a single extended index finger, checked before
    // the broader hand-covering-face test below so shhh (finger on the
    // mouth) doesn't get swallowed by "any hand near the face".
    if (h.index_up && !h.middle_up && !h.ring_up && !h.pinky_up) {
        if (fresh) {
            float d = dist(h.index_tip, state.last_face->mouth_center) / state.last_face->face_width;
            if (d < 0.55f) return Gesture::Shhh;
        }
        return Gesture::OneFingerUp;
    }

    // hand covering face: the one hand we see sits roughly where the face
    // last was. Wider tolerance if the face detector has fully lost the
    // face; tighter if it's still partially tracking through the fingers.
    if (fresh) {
        float d = dist(h.palm_center, state.last_face->mouth_center) / state.last_face->face_width;
        double threshold = state.face_seen_this_frame ? tuning::hand_cover_face_dist_face_seen
                                                        : tuning::hand_cover_face_dist_face_lost;
        if (d < threshold) return Gesture::HandCoverFace;
    }

    if (h.curled_count == 0) return Gesture::HandStretchedOut;

    // hands are up but not making a specific shape - still allow a strong
    // side-eye read to win over an ambiguous hand pose.
    if (fresh && std::abs(state.last_face->yaw_deg) > tuning::side_eye_yaw_deg) {
        return Gesture::SideEyeCat;
    }

    return Gesture::Default;
}

// twoFingersTogether / twoHandsOnHead / crashOutCat / danceCat - the
// gestures that only make sense as a *pair* of hands. nullopt means "none
// of these matched", so the caller falls through to the single-hand shape
// logic on the first hand, exactly like gesture_meme.py's `h = hands[0]`.
std::optional<Gesture> decide_two_hand_shape(const GestureState& state, const HandInfo& a,
                                               const HandInfo& b, bool fresh) {
    if (is_pointing(a) && is_pointing(b)) {
        float avg_scale = (a.hand_scale + b.hand_scale) / 2.0f;
        float tip_gap = dist(a.index_tip, b.index_tip) / avg_scale;
        if (tip_gap < 1.4f) return Gesture::TwoFingersTogether;
    }

    if (fresh) {
        const Vec3& mouth_center = state.last_face->mouth_center;
        float face_width = state.last_face->face_width;
        bool near_face = dist(a.palm_center, mouth_center) / face_width < 2.2f &&
                          dist(b.palm_center, mouth_center) / face_width < 2.2f;
        if (near_face) {
            float head_top_y = mouth_center.y - face_width * 1.1f;
            bool both_above_head = a.palm_center.y < head_top_y && b.palm_center.y < head_top_y;
            if (both_above_head) return Gesture::TwoHandsOnHead;

            // crashOutCat requires both hands to actually be clenched
            // fists - an open hand near the face falls through instead of
            // getting swallowed by this.
            if (a.curled_count == 4 && b.curled_count == 4) return Gesture::CrashOutCat;
        }
    }

    // danceCat: both hands showing open palms, one near the TOP of the
    // screen and the other near the BOTTOM - absolute frame position,
    // doesn't matter which hand is on top.
    if (a.curled_count == 0 && b.curled_count == 0) {
        float top = std::min(a.palm_center.y, b.palm_center.y);
        float bottom = std::max(a.palm_center.y, b.palm_center.y);
        if (top < tuning::dance_top_zone_y && bottom > tuning::dance_bottom_zone_y) {
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

    state.last_face = snap;
    state.last_yaw_debug = snap->yaw_deg;
    if (face_result.has_transform) {
        state.last_pitch_debug = pitch_from_transform(face_result.transform);
    }

    auto scores = blendshape_map(face_result);
    auto get = [&](const char* k) {
        auto it = scores.find(k);
        return it == scores.end() ? 0.0f : it->second;
    };
    state.last_jaw_open_debug = get("jawOpen");
    state.last_smile_debug = std::max(get("mouthSmileLeft"), get("mouthSmileRight"));
    state.last_brow_raise_debug = get("browInnerUp");
    state.last_wink_debug = wink_score(scores);
    state.last_eye_wide_debug = eye_wide_score(scores);

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
                // no hands: side-eye and huh are both face-only poses, and
                // BOTH require no hands visible - mouthOpenCat lives only
                // in the OneHand/TwoHands cases below, specifically so it
                // can never clash with huhCat: huhCat needs mouth-open
                // with no hand, mouthOpenCat needs mouth-open WITH a hand.
                if (fresh && state.last_jaw_open_debug > tuning::huh_jaw_threshold &&
                    state.last_eye_wide_debug > tuning::eye_wide_threshold) {
                    return Gesture::HuhCat;
                }
                if (fresh && std::abs(state.last_face->yaw_deg) > tuning::side_eye_yaw_deg) {
                    return Gesture::SideEyeCat;
                }
                if (fresh && state.last_pitch_debug > tuning::side_eye_down_pitch_deg) {
                    return Gesture::SideEyeDownCat;
                }
                return Gesture::Default;
            },
            [&](const OneHand& one) -> Gesture {
                // mouthOpenCat: mouth open AND a hand visible somewhere in
                // frame - any hand shape counts. Checked before hand-shape-
                // specific logic so an open mouth with a hand up (eating,
                // talking with your hands) reads as this.
                if (fresh && state.last_jaw_open_debug > tuning::mouth_open_jaw_threshold) {
                    return Gesture::MouthOpenCat;
                }
                return decide_single_hand_shape(state, one.hand, fresh);
            },
            [&](const TwoHands& two) -> Gesture {
                if (fresh && state.last_jaw_open_debug > tuning::mouth_open_jaw_threshold) {
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
