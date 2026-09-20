// Desktop entry point: MediaPipe C++ sessions plus the shared pipeline.
#pragma once

#include <utility>

#include "frame_pipeline.h"
#include "landmarker_sessions.h"

namespace miaucam {

// Non-const because DetectForVideo mutates the sessions' graph state.
struct Environment {
    LandmarkerSessions landmarkers;
};

std::pair<CoreState, StepOutput> step(CoreState state, Environment& env,
                                        const FrameInput& input);

}  // namespace miaucam
