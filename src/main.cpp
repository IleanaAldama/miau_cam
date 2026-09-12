// Webcam gesture -> meme detector (desktop version). C++ port of
// gesture_meme.py, split into domains (see the other files in src/):
//   geometry            pure vector math
//   hand_classifier     "what is this hand doing"
//   face_signals        "what is this face doing"
//   optical_flow        "is the frame spinning"
//   gesture_state       combines the above into a named gesture
//   meme_catalog        gesture -> asset(s) on disk
//   landmarker_service  owns the MediaPipe sessions
//   hud                 state -> pixels
// This file is just wiring: open the camera/windows, run the loop.
//
// Press q or ESC to quit.

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "gesture.h"
#include "gesture_state.h"
#include "hud.h"
#include "landmarker_service.h"
#include "meme_catalog.h"
#include "optical_flow.h"
#include "result.h"
#include "tuning.h"

namespace miaucam {
namespace {

const std::string kRoot = MIAUCAM_ROOT;
const std::string kModelsDir = kRoot + "/models";
const std::string kMemesDir = kRoot + "/memes";

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

struct VideoGestureCaptures {
    std::unordered_map<Gesture, cv::VideoCapture> caps;
};

Result<VideoGestureCaptures> open_video_gesture_captures(const std::string& memes_dir) {
    VideoGestureCaptures out;
    for (Gesture g : kAllGestures) {
        if (!is_video_gesture(g)) continue;
        std::string path = memes_dir + "/" + video_file_for(g);
        cv::VideoCapture cap(path);
        if (!cap.isOpened()) {
            return Result<VideoGestureCaptures>::Err("missing meme file: " + path);
        }
        out.caps[g] = std::move(cap);
    }
    return Result<VideoGestureCaptures>::Ok(std::move(out));
}

cv::Mat next_video_frame(VideoGestureCaptures& video_caps, Gesture g) {
    cv::Mat frame;
    cv::VideoCapture& cap = video_caps.caps.at(g);
    if (!cap.read(frame)) {
        cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        cap.read(frame);
    }
    return frame;
}

// Randomly picks one of a gesture's meme images (some gestures have
// several - variety on repeat triggers).
class MemePicker {
public:
    explicit MemePicker(const MemeCatalog& catalog) : catalog_(catalog), rng_(std::random_device{}()) {}

    const cv::Mat& pick(Gesture g) {
        const auto& imgs = catalog_.at(g);
        std::uniform_int_distribution<size_t> dist(0, imgs.size() - 1);
        return imgs[dist(rng_)];
    }

private:
    const MemeCatalog& catalog_;
    std::mt19937 rng_;
};

int run() {
    auto landmarkers = create_landmarker_service(kModelsDir);
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

    // every frame's flow numbers get logged here, timestamped - so the raw
    // numbers from a real spin attempt can be inspected afterward.
    std::ofstream flow_log(kRoot + "/flow_debug_log.csv");
    flow_log << "t_ms,magnitude,coherence,score,fraction,peak_2s,gesture\n";

    GestureState state;
    Gesture current_gesture = Gesture::Default;
    Gesture candidate_gesture = Gesture::Default;
    int candidate_streak = 0;
    double last_non_default_at = now_ms();

    MemePicker meme_picker(memes.value());
    cv::Mat current_meme = meme_picker.pick(Gesture::Default);

    cv::Mat prev_flow_gray;
    double start_time_ms = now_ms();

    cv::Mat frame;
    while (cap.read(frame) && !frame.empty()) {
        cv::flip(frame, frame, 1);  // mirror, like a selfie cam

        FlowSignal flow = compute_frame_flow(frame, prev_flow_gray);
        double now = now_ms();
        state = update_flow(std::move(state), flow.magnitude, flow.coherence, now);
        flow_log << std::fixed << std::setprecision(4) << now << "," << flow.magnitude << ","
                 << flow.coherence << "," << state.spin.last_score_debug << ","
                 << state.spin.last_fraction_debug << "," << state.spin.last_peak_debug << ","
                 << to_string(current_gesture) << "\n";

        int64_t ts_ms = static_cast<int64_t>(now - start_time_ms);
        DetectionResult detection = detect(landmarkers.value(), frame, ts_ms);
        state = update_face(std::move(state), detection.face, now);

        Gesture gesture = decide(state, detection.hand, now);

        if (gesture == candidate_gesture) {
            candidate_streak++;
        } else {
            candidate_gesture = gesture;
            candidate_streak = 1;
        }

        if (candidate_streak >= tuning::stable_frames_required && gesture != current_gesture) {
            current_gesture = gesture;
            if (!is_video_gesture(gesture)) {
                current_meme = meme_picker.pick(gesture);
            } else {
                video_caps.value().caps.at(gesture).set(cv::CAP_PROP_POS_FRAMES, 0);
            }
        }

        if (gesture != Gesture::Default) {
            last_non_default_at = now;
        } else if (now - last_non_default_at > tuning::default_fallback_ms &&
                   current_gesture != Gesture::Default) {
            current_gesture = Gesture::Default;
            current_meme = meme_picker.pick(Gesture::Default);
        }

        draw_landmarks(frame, detection.hand);
        draw_debug_hud(frame, state, current_gesture);

        cv::Mat meme_view;
        if (is_video_gesture(current_gesture)) {
            cv::Mat vframe = next_video_frame(video_caps.value(), current_gesture);
            meme_view = fit_to_height(vframe.empty() ? current_meme : vframe, frame.rows);
        } else {
            meme_view = fit_to_height(current_meme, frame.rows);
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
