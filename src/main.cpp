// Webcam gesture -> meme detector. This file is just wiring; see the
// other files in src/ for the actual domains. Press q or ESC to quit.

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "gesture.h"
#include "hud.h"
#include "meme_catalog.h"
#include "miaucam_core.h"
#include "result.h"

namespace miaucam {
namespace {

const std::string kRoot = MIAUCAM_ROOT;
const std::string kModelsDir = kRoot + "/models";
const std::string kMemesDir = kRoot + "/memes";

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int run() {
    auto landmarkers = create_landmarker_sessions(kModelsDir);
    if (!landmarkers) {
        std::cerr << "Failed to create landmarkers: " << landmarkers.error() << std::endl;
        return 1;
    }

    auto memes = load_memes(kMemesDir);
    if (!memes) {
        std::cerr << memes.error() << std::endl;
        return 1;
    }

    auto video_caps = open_video_gesture_captures(kMemesDir);
    if (!video_caps) {
        std::cerr << video_caps.error() << std::endl;
        return 1;
    }

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Could not open webcam (index 0)" << std::endl;
        return 1;
    }

    cv::namedWindow("Camera");
    cv::namedWindow("Meme");
    cv::moveWindow("Camera", 40, 80);
    cv::moveWindow("Meme", 720, 80);

    // per-frame flow numbers, for tuning spin detection afterward.
    std::ofstream flow_log(kRoot + "/flow_debug_log.csv");
    flow_log << "t_ms,magnitude,coherence,score,fraction,peak_2s,gesture\n";

    Environment env{std::move(landmarkers.value())};
    CoreState state;

    std::mt19937 meme_rng{std::random_device{}()};
    cv::Mat current_meme = pick_meme(memes.value(), Gesture::Default, meme_rng);

    cv::Mat frame;
    while (cap.read(frame) && !frame.empty()) {
        cv::flip(frame, frame, 1);  // mirror, like a selfie cam

        cv::Mat rgb;
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        double now = now_ms();
        FrameInput input{{rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step)}, now};

        auto [next_state, output] = step(std::move(state), env, input);
        state = std::move(next_state);

        flow_log << std::fixed << std::setprecision(4) << now << ","
                 << state.gesture_state.spin.last_magnitude_debug << ","
                 << state.gesture_state.spin.last_coherence_debug << ","
                 << state.gesture_state.spin.last_score_debug << ","
                 << state.gesture_state.spin.last_fraction_debug << ","
                 << state.gesture_state.spin.last_peak_debug << ","
                 << to_string(output.gesture) << "\n";

        if (output.gesture_changed) {
            if (!is_video_gesture(output.gesture)) {
                current_meme = pick_meme(memes.value(), output.gesture, meme_rng);
            } else {
                video_caps.value().caps.at(output.gesture).set(cv::CAP_PROP_POS_FRAMES, 0);
            }
        }

        draw_landmarks(frame, output.hand);
        draw_debug_hud(frame, state.gesture_state, output.gesture);

        cv::Mat meme_view;
        if (is_video_gesture(output.gesture)) {
            cv::Mat vframe = next_video_frame(video_caps.value(), output.gesture);
            meme_view = fit_to_height(vframe.empty() ? current_meme : vframe, frame.rows);
        } else {
            cv::Mat meme;
            if (flip_meme(state)) {
                cv::flip(current_meme, meme, 1);
            } else {
                meme = current_meme;
            }
            meme_view = fit_to_height(meme, frame.rows);
        }
        cv::imshow("Camera", frame);
        cv::imshow("Meme", meme_view);

        int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 27) break;
    }

    cap.release();
    for (auto& [gesture, vcap] : video_caps.value().caps) vcap.release();
    cv::destroyAllWindows();
    return 0;
}

}  // namespace
}  // namespace miaucam

int main() { return miaucam::run(); }
