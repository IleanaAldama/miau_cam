// enum class instead of gesture_meme.py's strings: switch-based dispatch
// for meme lookup/video-check instead of string comparisons.
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

constexpr Gesture kAllGestures[] = {
    Gesture::Default,       Gesture::Rockstar,        Gesture::OneFingerUp,
    Gesture::Fist,          Gesture::Shhh,            Gesture::TwoFingersTogether,
    Gesture::HandCoverFace, Gesture::CrashOutCat,     Gesture::TwoHandsOnHead,
    Gesture::HandStretchedOut, Gesture::SideEyeCat,   Gesture::SideEyeDownCat,
    Gesture::MouthOpenCat,  Gesture::HuhCat,          Gesture::DanceCat,
    Gesture::SpinCat,
};

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
