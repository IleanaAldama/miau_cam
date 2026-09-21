#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "detection_input.h"
#include "frame_pipeline.h"
#include "gesture.h"

namespace {

miaucam::CoreState g_state;
std::vector<uint8_t> g_frame;

std::vector<float> floats(const emscripten::val& array) {
    return emscripten::convertJSArrayToNumberVector<float>(array);
}

}  // namespace

// A Uint8Array over wasm memory for the caller to copy RGBA pixels into.
emscripten::val frame_view(int size) {
    g_frame.resize(static_cast<size_t>(size));
    return emscripten::val(emscripten::typed_memory_view(g_frame.size(), g_frame.data()));
}

void reset() { g_state = miaucam::CoreState{}; }

int advance(int width, int height, double timestamp_ms, const emscripten::val& hands,
            const emscripten::val& face, const emscripten::val& expression,
            const emscripten::val& transform) {
    cv::Mat rgba(height, width, CV_8UC4, g_frame.data());
    cv::Mat rgb;
    cv::cvtColor(rgba, rgb, cv::COLOR_RGBA2RGB);

    miaucam::RawDetection raw{floats(hands), floats(face), floats(expression), floats(transform)};

    const miaucam::FrameInput input{{rgb.data, width, height, static_cast<int>(rgb.step)},
                                    timestamp_ms};
    auto [next, output] = miaucam::advance(std::move(g_state), input, miaucam::to_detection(raw));
    g_state = std::move(next);
    return static_cast<int>(output.gesture);
}

emscripten::val meme_files(int gesture) {
    const auto& files = miaucam::all_gestures().at(static_cast<size_t>(gesture)).files;
    return emscripten::val::array(files);
}

int gesture_count() { return static_cast<int>(miaucam::all_gestures().size()); }

bool flip_meme_now() { return miaucam::flip_meme(g_state); }

bool is_video(int gesture) {
    return miaucam::all_gestures().at(static_cast<size_t>(gesture)).is_video;
}

EMSCRIPTEN_BINDINGS(miaucam) {
    emscripten::function("frame_view", &frame_view);
    emscripten::function("reset", &reset);
    emscripten::function("advance", &advance);
    emscripten::function("meme_files", &meme_files);
    emscripten::function("is_video", &is_video);
    emscripten::function("flip_meme", &flip_meme_now);
    emscripten::function("gesture_count", &gesture_count);
}
