#import "MiaucamCore.h"

#include <opencv2/imgproc.hpp>

#include "detection_input.h"
#include "frame_pipeline.h"
#include "gesture.h"

namespace {

miaucam::CoreState g_state;

std::vector<float> to_floats(NSData* data) {
    const float* first = static_cast<const float*>(data.bytes);
    return std::vector<float>(first, first + data.length / sizeof(float));
}

}  // namespace

@implementation MiaucamCore

+ (void)reset {
    g_state = miaucam::CoreState{};
}

+ (int)advanceWithBGRA:(const uint8_t *)pixels
                 width:(int)width
                height:(int)height
                stride:(int)stride
           timestampMs:(double)timestampMs
                 hands:(NSData *)hands
                  face:(NSData *)face
       blendshapeNames:(NSArray<NSString *> *)names
      blendshapeScores:(NSData *)scores
             transform:(NSData *)transform {
    cv::Mat bgra(height, width, CV_8UC4, const_cast<uint8_t*>(pixels), static_cast<size_t>(stride));
    cv::Mat rgb;
    cv::cvtColor(bgra, rgb, cv::COLOR_BGRA2RGB);

    miaucam::RawDetection raw{to_floats(hands), to_floats(face), {}, to_floats(scores),
                              to_floats(transform)};
    for (NSString* name in names) raw.blendshape_names.push_back(name.UTF8String);

    const miaucam::FrameInput input{{rgb.data, width, height, static_cast<int>(rgb.step)},
                                    timestampMs};
    auto [next, output] =
        miaucam::advance(std::move(g_state), input, miaucam::to_detection(raw));
    g_state = std::move(next);
    return static_cast<int>(output.gesture);
}

+ (NSArray<NSString *> *)memeFilesForGesture:(int)gesture {
    NSMutableArray<NSString *>* files = [NSMutableArray array];
    for (const auto& file : miaucam::all_gestures().at(static_cast<size_t>(gesture)).files) {
        [files addObject:[NSString stringWithUTF8String:file.c_str()]];
    }
    return files;
}

+ (BOOL)isVideoGesture:(int)gesture {
    return miaucam::all_gestures().at(static_cast<size_t>(gesture)).is_video;
}

@end
