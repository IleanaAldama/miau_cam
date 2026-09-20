#include "frame_pipeline.h"

#include "optical_flow.h"
#include "tuning.h"

namespace miaucam {

std::pair<CoreState, StepOutput> advance(CoreState state, const FrameInput& input,
                                           DetectionResult detection) {
    // Zero-copy view over the caller's own buffer, just for the flow pass.
    cv::Mat frame(input.frame.height, input.frame.width, CV_8UC3,
                  const_cast<uint8_t*>(input.frame.data), input.frame.stride);

    FlowSignal flow = compute_frame_flow(frame, state.prev_flow_gray);
    state.gesture_state = update_flow(std::move(state.gesture_state), flow.magnitude,
                                        flow.coherence, input.timestamp_ms);

    state.gesture_state =
        update_face(std::move(state.gesture_state), detection.face, input.timestamp_ms);

    Gesture detected = decide(state.gesture_state, detection.hand, input.timestamp_ms);

    if (detected == state.candidate_gesture) {
        state.candidate_streak++;
    } else {
        state.candidate_gesture = detected;
        state.candidate_streak = 1;
    }

    bool changed = false;
    if (state.candidate_streak >= tuning::stability::frames_required &&
        detected != state.current_gesture) {
        state.current_gesture = detected;
        changed = true;
    }

    if (detected != Gesture::Default) {
        state.last_non_default_at = input.timestamp_ms;
    } else if (input.timestamp_ms - state.last_non_default_at >
                   tuning::stability::default_fallback_ms &&
               state.current_gesture != Gesture::Default) {
        state.current_gesture = Gesture::Default;
        changed = true;
    }

    StepOutput output{state.current_gesture, changed, std::move(detection.hand)};
    return {std::move(state), std::move(output)};
}

}  // namespace miaucam
