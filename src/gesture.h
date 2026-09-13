// enum class instead of gesture_meme.py's strings, plus one metadata table
// (name, still/video, file list) instead of scattering that across switches.
#pragma once

#include <string>
#include <vector>

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

struct GestureInfo {
    Gesture id;
    std::string name;
    bool is_video;
    std::vector<std::string> files;  // still: one or more images; video: one clip
};

// One entry per Gesture, indexed by its enum value.
const std::vector<GestureInfo>& all_gestures();
const GestureInfo& info_for(Gesture g);

inline const std::string& to_string(Gesture g) { return info_for(g).name; }
inline bool is_video_gesture(Gesture g) { return info_for(g).is_video; }
inline const std::vector<std::string>& files_for(Gesture g) { return info_for(g).files; }

}  // namespace miaucam
