// The set of gestures this app recognizes. gesture_meme.py used plain
// strings for this everywhere; an enum class gets the same job done with
// compiler-checked exhaustiveness (missing a case in a switch is a warning,
// not a silent typo) and lets meme lookup / video-vs-still dispatch read as
// pattern matching instead of string comparisons.
#pragma once

#include <string>

namespace miaucam {

enum class Gesture {
    Default,
    Rockstar,
    OneFingerUp,
    Fist,
    Shhh,
    TwoFingersTogether,
    HandCoverFace,
    CrashOutCat,
    TwoHandsOnHead,
    HandStretchedOut,
    SideEyeCat,
    SideEyeDownCat,
    MouthOpenCat,
    HuhCat,
    DanceCat,
    SpinCat,
};

// Every gesture, for code that needs to iterate all of them (e.g. loading
// every still-image meme up front).
constexpr Gesture kAllGestures[] = {
    Gesture::Default,       Gesture::Rockstar,        Gesture::OneFingerUp,
    Gesture::Fist,          Gesture::Shhh,            Gesture::TwoFingersTogether,
    Gesture::HandCoverFace, Gesture::CrashOutCat,     Gesture::TwoHandsOnHead,
    Gesture::HandStretchedOut, Gesture::SideEyeCat,   Gesture::SideEyeDownCat,
    Gesture::MouthOpenCat,  Gesture::HuhCat,          Gesture::DanceCat,
    Gesture::SpinCat,
};

// Human-readable name, for the debug HUD and the flow log CSV.
inline std::string to_string(Gesture g) {
    switch (g) {
        case Gesture::Default: return "default";
        case Gesture::Rockstar: return "rockstar";
        case Gesture::OneFingerUp: return "oneFingerUp";
        case Gesture::Fist: return "fist";
        case Gesture::Shhh: return "shhh";
        case Gesture::TwoFingersTogether: return "twoFingersTogether";
        case Gesture::HandCoverFace: return "handCoverFace";
        case Gesture::CrashOutCat: return "crashOutCat";
        case Gesture::TwoHandsOnHead: return "twoHandsOnHead";
        case Gesture::HandStretchedOut: return "handStretchedOut";
        case Gesture::SideEyeCat: return "sideEyeCat";
        case Gesture::SideEyeDownCat: return "sideEyeDownCat";
        case Gesture::MouthOpenCat: return "mouthOpenCat";
        case Gesture::HuhCat: return "huhCat";
        case Gesture::DanceCat: return "danceCat";
        case Gesture::SpinCat: return "spinCat";
    }
    return "unknown";
}

// Gestures whose meme is a video, not a still image.
inline bool is_video_gesture(Gesture g) {
    switch (g) {
        case Gesture::DanceCat:
        case Gesture::SpinCat:
            return true;
        default:
            return false;
    }
}

}  // namespace miaucam
