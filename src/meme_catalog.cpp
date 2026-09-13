#include "meme_catalog.h"

#include <opencv2/imgcodecs.hpp>

namespace miaucam {

Result<MemeCatalog> load_memes(const std::string& memes_dir) {
    MemeCatalog cache;
    for (const auto& info : all_gestures()) {
        if (info.is_video) continue;  // streamed frame-by-frame instead

        std::vector<cv::Mat> imgs;
        for (const auto& name : info.files) {
            cv::Mat img = cv::imread(memes_dir + "/" + name);
            if (img.empty()) {
                return Result<MemeCatalog>::Err("missing meme file: " + memes_dir + "/" + name);
            }
            imgs.push_back(std::move(img));
        }
        cache[info.id] = std::move(imgs);
    }
    return Result<MemeCatalog>::Ok(std::move(cache));
}

const cv::Mat& pick_meme(const MemeCatalog& catalog, Gesture g, std::mt19937& rng) {
    const auto& imgs = catalog.at(g);
    std::uniform_int_distribution<size_t> dist(0, imgs.size() - 1);
    return imgs[dist(rng)];
}

Result<VideoGestureCaptures> open_video_gesture_captures(const std::string& memes_dir) {
    VideoGestureCaptures out;
    for (const auto& info : all_gestures()) {
        if (!info.is_video) continue;
        std::string path = memes_dir + "/" + info.files.front();
        cv::VideoCapture cap(path);
        if (!cap.isOpened()) {
            return Result<VideoGestureCaptures>::Err("missing meme file: " + path);
        }
        out.caps[info.id] = std::move(cap);
    }
    return Result<VideoGestureCaptures>::Ok(std::move(out));
}

cv::Mat next_video_frame(VideoGestureCaptures& video_caps, Gesture g) {
    cv::Mat frame;
    cv::VideoCapture& cap = video_caps.caps.at(g);
    if (!cap.read(frame)) {
        cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        cap.read(frame);
    }
    return frame;
}

}  // namespace miaucam
