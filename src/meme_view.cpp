#include "meme_view.h"

#include <opencv2/imgproc.hpp>

namespace miaucam {

cv::Mat render_meme(const cv::Mat& meme, bool flip, int height) {
    cv::Mat oriented;
    if (flip) {
        cv::flip(meme, oriented, 1);
    } else {
        oriented = meme;
    }

    const double scale = static_cast<double>(height) / oriented.rows;
    cv::Mat out;
    cv::resize(oriented, out, cv::Size(static_cast<int>(oriented.cols * scale), height));
    return out;
}

const cv::Mat& cached_meme_view(MemeViewCache& cache, const cv::Mat& meme, bool flip, int height) {
    if (cache.view.empty() || cache.source != meme.data || cache.flip != flip ||
        cache.height != height) {
        cache = {meme.data, flip, height, render_meme(meme, flip, height)};
    }
    return cache.view;
}

}  // namespace miaucam
