#include "meme_catalog.h"

#include <gtest/gtest.h>

using namespace miaucam;

namespace {
const std::string kMemesDir = std::string(MIAUCAM_ROOT) + "/memes";
}

TEST(MemeCatalog, LoadMemesFindsEveryStillImage) {
    auto result = load_memes(kMemesDir);
    ASSERT_TRUE(result.ok());
    for (const auto& info : all_gestures()) {
        if (info.is_video) continue;
        EXPECT_EQ(result.value().count(info.id), 1u);
    }
}

TEST(MemeCatalog, OpenVideoGestureCapturesFindsBothClips) {
    EXPECT_TRUE(open_video_gesture_captures(kMemesDir).ok());
}
