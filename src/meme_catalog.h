// "gesture -> asset(s) on disk". No detection logic lives here at all.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

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

}  // namespace miaucam
