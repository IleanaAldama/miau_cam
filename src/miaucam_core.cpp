#include "miaucam_core.h"

namespace miaucam {

std::pair<CoreState, StepOutput> step(CoreState state, Environment& env,
                                        const FrameInput& input) {
    DetectionResult detection =
        detect(env.landmarkers, input.frame, static_cast<int64_t>(input.timestamp_ms));
    return advance(std::move(state), input, std::move(detection));
}

}  // namespace miaucam
