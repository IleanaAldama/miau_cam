#include "gesture.h"

#include <cassert>
#include <cstddef>

namespace miaucam {

const std::vector<GestureInfo>& all_gestures() {
    static const std::vector<GestureInfo> table = {
        {Gesture::Default, "default", false, {"pokercat.jpg"}},
        {Gesture::Rockstar, "rockstar", false, {"cat.jpg"}},
        {Gesture::OneFingerUp, "oneFingerUp", false, {"profcat.jpg", "professorcat.jpg"}},
        {Gesture::Fist, "fist", false, {"punchcat.jpg"}},
        {Gesture::Shhh, "shhh", false, {"shhcat.jpg"}},
        {Gesture::TwoFingersTogether, "twoFingersTogether", false,
         {"uwucat.jpg", "uwucatt.jpg", "fingers together muehehe .jpg"}},
        {Gesture::HandCoverFace, "handCoverFace", false, {"hand cover face .jpg"}},
        {Gesture::CrashOutCat, "crashOutCat", false, {"crashout cat .jpg"}},
        {Gesture::TwoHandsOnHead, "twoHandsOnHead", false, {"two hands on head .jpg"}},
        {Gesture::HandStretchedOut, "handStretchedOut", false,
         {"hand stretched out, palm facing up .jpg"}},
        {Gesture::SideEyeCat, "sideEyeCat", false, {"side eye cat.jpg"}},
        {Gesture::SideEyeDownCat, "sideEyeDownCat", false, {"side eye.png"}},
        {Gesture::MouthOpenCat, "mouthOpenCat", false, {"laugh and point .jpg"}},
        {Gesture::HuhCat, "huhCat", false, {"huh.png"}},
        {Gesture::DanceCat, "danceCat", true, {"two palms up.mov"}},
        {Gesture::SpinCat, "spinCat", true, {"spin cat.mov"}},
    };
    return table;
}

const GestureInfo& info_for(Gesture g) {
    const auto& table = all_gestures();
    size_t idx = static_cast<size_t>(g);
    assert(idx < table.size() && table[idx].id == g);
    return table[idx];
}

}  // namespace miaucam
