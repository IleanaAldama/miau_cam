// Pure "meme -> pixels for the window" helpers. Nothing here modifies its
// input: cv::Mat copies share pixels, so writing into one corrupts the source.
#pragma once

#include <opencv2/core.hpp>

namespace miaucam {

// Optionally mirrored, then scaled to the given height. Always a new image.
cv::Mat render_meme(const cv::Mat& meme, bool flip, int height);

// Remembers the last render; it only changes when the source image, the flip
// or the height does, so the per-frame cost is a pointer compare.
struct MemeViewCache {
    const uchar* source = nullptr;
    bool flip = false;
    int height = 0;
    cv::Mat view;
};

const cv::Mat& cached_meme_view(MemeViewCache& cache, const cv::Mat& meme, bool flip, int height);

}  // namespace miaucam
