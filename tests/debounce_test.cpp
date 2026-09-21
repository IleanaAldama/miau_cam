#include "debounce.h"

#include <gtest/gtest.h>

#include "tuning.h"

using namespace miaucam;

namespace {

constexpr int frames_required = tuning::stability::frames_required;

DebounceState feed(DebounceState state, Gesture g, int frames, double start_ms,
                   bool* changed_out = nullptr) {
    for (int i = 0; i < frames; ++i) {
        auto step = debounce(state, g, start_ms + i * 33.0);
        state = step.state;
        if (changed_out && step.changed) *changed_out = true;
    }
    return state;
}

}  // namespace

TEST(Debounce, SwitchesOnlyAfterTheRequiredFrames) {
    bool changed = false;
    DebounceState state = feed({}, Gesture::Fist, frames_required - 1, 0.0, &changed);
    EXPECT_EQ(state.current, Gesture::Default);
    EXPECT_FALSE(changed);

    auto step = debounce(state, Gesture::Fist, 1000.0);
    EXPECT_EQ(step.state.current, Gesture::Fist);
    EXPECT_TRUE(step.changed);
}

TEST(Debounce, ChangedIsTrueOnlyOnTheSwitchFrame) {
    DebounceState state = feed({}, Gesture::Fist, frames_required, 0.0);
    ASSERT_EQ(state.current, Gesture::Fist);
    EXPECT_FALSE(debounce(state, Gesture::Fist, 500.0).changed);
}

TEST(Debounce, InterruptedCandidateRestartsTheStreak) {
    DebounceState state = feed({}, Gesture::Fist, frames_required - 1, 0.0);
    state = debounce(state, Gesture::Rockstar, 200.0).state;
    state = feed(state, Gesture::Fist, frames_required - 1, 300.0);
    EXPECT_EQ(state.current, Gesture::Default);
}

TEST(Debounce, FallsBackToDefaultAfterTheTimeout) {
    DebounceState shown;
    shown.current = Gesture::Fist;
    shown.candidate = Gesture::Fist;
    shown.last_non_default_at = 1000.0;

    auto early = debounce(shown, Gesture::Default, 1000.0 + tuning::stability::default_fallback_ms - 1);
    EXPECT_EQ(early.state.current, Gesture::Fist);
    EXPECT_FALSE(early.changed);

    auto late = debounce(shown, Gesture::Default, 1000.0 + tuning::stability::default_fallback_ms + 1);
    EXPECT_EQ(late.state.current, Gesture::Default);
    EXPECT_TRUE(late.changed);
}

TEST(Debounce, ANonDefaultDetectionRefreshesTheFallbackTimer) {
    auto step = debounce({}, Gesture::Fist, 5000.0);
    EXPECT_DOUBLE_EQ(step.state.last_non_default_at, 5000.0);
}
