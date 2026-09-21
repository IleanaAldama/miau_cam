#include "meme_view.h"

#include <gtest/gtest.h>

#include <opencv2/imgproc.hpp>

using namespace miaucam;

namespace {

// Left half dark, right half bright: a flip is easy to see.
cv::Mat make_meme() {
    cv::Mat m(4, 6, CV_8UC3, cv::Scalar(0, 0, 0));
    m(cv::Rect(3, 0, 3, 4)).setTo(cv::Scalar(255, 255, 255));
    return m;
}

bool same_pixels(const cv::Mat& a, const cv::Mat& b) {
    return a.size() == b.size() && a.type() == b.type() && cv::norm(a, b, cv::NORM_INF) == 0.0;
}

}  // namespace

TEST(RenderMeme, NeverModifiesItsSource) {
    const cv::Mat meme = make_meme();
    const cv::Mat original = meme.clone();
    for (int i = 0; i < 3; ++i) {
        render_meme(meme, true, meme.rows);
        render_meme(meme, true, 8);
        render_meme(meme, false, 8);
    }
    EXPECT_TRUE(same_pixels(meme, original));
}

TEST(RenderMeme, FlipMirrorsHorizontally) {
    const cv::Mat meme = make_meme();
    cv::Mat expected;
    cv::flip(meme, expected, 1);
    EXPECT_TRUE(same_pixels(render_meme(meme, true, meme.rows), expected));
    EXPECT_TRUE(same_pixels(render_meme(meme, false, meme.rows), meme));
}

TEST(RenderMeme, ScalesToTheHeightKeepingTheAspectRatio) {
    const cv::Mat out = render_meme(make_meme(), false, 8);
    EXPECT_EQ(out.rows, 8);
    EXPECT_EQ(out.cols, 12);
}

TEST(MemeViewCache, ReusesTheRenderUntilSomethingChanges) {
    const cv::Mat meme = make_meme();
    MemeViewCache cache;

    const uchar* first = cached_meme_view(cache, meme, false, 8).data;
    EXPECT_EQ(cached_meme_view(cache, meme, false, 8).data, first);

    const uchar* flipped = cached_meme_view(cache, meme, true, 8).data;
    EXPECT_NE(flipped, first);
    EXPECT_EQ(cached_meme_view(cache, meme, true, 8).data, flipped);

    const cv::Mat other = make_meme();
    cv::Mat resized;
    cached_meme_view(cache, other, true, 8);
    EXPECT_EQ(cache.source, other.data);
    EXPECT_EQ(cached_meme_view(cache, other, true, 16).rows, 16);
}
