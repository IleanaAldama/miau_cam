#include "frame_pipeline.h"

#include "optical_flow.h"

namespace miaucam {

bool flip_meme(const CoreState& state) {
    return state.debounce.current == Gesture::SideEyeCat && state.gesture_state.last_face &&
           state.gesture_state.last_face->yaw_deg >= 0.0;
}

std::pair<CoreState, StepOutput> advance(CoreState state, const FrameInput& input,
                                           DetectionResult detection) {
    // Zero-copy view over the caller's own buffer, just for the flow pass.
    const cv::Mat frame(input.frame.height, input.frame.width, CV_8UC3,
                        const_cast<uint8_t*>(input.frame.data), input.frame.stride);

    FlowResult flow = compute_frame_flow(frame, state.prev_flow_gray);
    state.prev_flow_gray = std::move(flow.small_gray);
    state.gesture_state = update_flow(std::move(state.gesture_state), flow.signal.magnitude,
                                      flow.signal.coherence, input.timestamp_ms);

    state.gesture_state =
        update_face(std::move(state.gesture_state), detection.face, input.timestamp_ms);

    const Gesture detected = decide(state.gesture_state, detection.hand, input.timestamp_ms);

    auto [next_debounce, changed] = debounce(state.debounce, detected, input.timestamp_ms);
    state.debounce = next_debounce;

    StepOutput output{state.debounce.current, changed, std::move(detection.hand)};
    return {std::move(state), std::move(output)};
}

}  // namespace miaucam
