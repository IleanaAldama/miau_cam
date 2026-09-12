#include "meme_catalog.h"

#include <opencv2/imgcodecs.hpp>

namespace miaucam {

std::vector<std::string> meme_files_for(Gesture g) {
    switch (g) {
        case Gesture::Rockstar: return {"cat.jpg"};
        case Gesture::Default: return {"pokercat.jpg"};
        case Gesture::OneFingerUp: return {"profcat.jpg", "professorcat.jpg"};
        case Gesture::Fist: return {"punchcat.jpg"};
        case Gesture::Shhh: return {"shhcat.jpg"};
        case Gesture::TwoFingersTogether:
            return {"uwucat.jpg", "uwucatt.jpg", "fingers together muehehe .jpg"};
        case Gesture::HandCoverFace: return {"hand cover face .jpg"};
        case Gesture::CrashOutCat: return {"crashout cat .jpg"};
        case Gesture::TwoHandsOnHead: return {"two hands on head .jpg"};
        case Gesture::HandStretchedOut: return {"hand stretched out, palm facing up .jpg"};
        case Gesture::SideEyeCat: return {"side eye cat.jpg"};
        case Gesture::SideEyeDownCat: return {"side eye.png"};
        case Gesture::MouthOpenCat: return {"laugh and point .jpg"};
        case Gesture::HuhCat: return {"huh.png"};
        case Gesture::DanceCat:
        case Gesture::SpinCat:
            return {};  // video gestures - see video_file_for()
    }
    return {};
}

std::string video_file_for(Gesture g) {
    switch (g) {
        case Gesture::DanceCat: return "two palms up.mov";
        case Gesture::SpinCat: return "spin cat.mov";
        default: return "";
    }
}

Result<MemeCatalog> load_memes(const std::string& memes_dir) {
    MemeCatalog cache;
    for (Gesture g : kAllGestures) {
        if (is_video_gesture(g)) continue;  // streamed frame-by-frame instead

        std::vector<cv::Mat> imgs;
        for (const auto& name : meme_files_for(g)) {
            cv::Mat img = cv::imread(memes_dir + "/" + name);
            if (img.empty()) {
                return Result<MemeCatalog>::Err("missing meme file: " + memes_dir + "/" + name);
            }
            imgs.push_back(std::move(img));
        }
        cache[g] = std::move(imgs);
    }
    return Result<MemeCatalog>::Ok(std::move(cache));
}

}  // namespace miaucam
