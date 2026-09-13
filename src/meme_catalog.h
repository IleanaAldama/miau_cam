// "gesture -> asset(s) on disk". No detection logic lives here.
#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "gesture.h"
#include "result.h"

namespace miaucam {

using MemeCatalog = std::unordered_map<Gesture, std::vector<cv::Mat>>;

// Video gestures are streamed frame-by-frame elsewhere, not cached here.
Result<MemeCatalog> load_memes(const std::string& memes_dir);

// g must be a still-image gesture present in catalog.
const cv::Mat& pick_meme(const MemeCatalog& catalog, Gesture g, std::mt19937& rng);

struct VideoGestureCaptures {
    std::unordered_map<Gesture, cv::VideoCapture> caps;
};

Result<VideoGestureCaptures> open_video_gesture_captures(const std::string& memes_dir);

// Loops back to the start once the video ends.
cv::Mat next_video_frame(VideoGestureCaptures& video_caps, Gesture g);

}  // namespace miaucam
