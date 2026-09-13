// "gesture -> asset(s) on disk". No detection logic lives here at all.
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

// File names (relative to memes/) for a still-image gesture. Empty for
// video gestures - use video_file_for() instead.
std::vector<std::string> meme_files_for(Gesture g);

// File name (relative to memes/) for a video gesture; empty string for
// still-image gestures.
std::string video_file_for(Gesture g);

using MemeCatalog = std::unordered_map<Gesture, std::vector<cv::Mat>>;

// Loads every still-image gesture's meme(s) up front. Video gestures are
// streamed frame-by-frame elsewhere and are not part of this cache.
Result<MemeCatalog> load_memes(const std::string& memes_dir);

// Randomly picks one of a gesture's meme images (some gestures have several
// - variety on repeat triggers). g must be a still-image gesture present in
// catalog.
const cv::Mat& pick_meme(const MemeCatalog& catalog, Gesture g, std::mt19937& rng);

// One VideoCapture per video gesture, keyed by gesture name.
struct VideoGestureCaptures {
    std::unordered_map<Gesture, cv::VideoCapture> caps;
};

Result<VideoGestureCaptures> open_video_gesture_captures(const std::string& memes_dir);

// Reads the next frame of a video gesture's meme, looping back to the start
// once it ends.
cv::Mat next_video_frame(VideoGestureCaptures& video_caps, Gesture g);

}  // namespace miaucam
