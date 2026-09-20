#include <jni.h>

#include <string>

#include <opencv2/core.hpp>

#include "gesture.h"
#include "optical_flow.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_miaucam_app_NativeCore_selfTest(JNIEnv* env, jobject) {
    cv::Mat prev_gray;
    cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(40, 80, 120));
    miaucam::compute_frame_flow(frame, prev_gray);
    const auto flow = miaucam::compute_frame_flow(frame, prev_gray);

    const std::string report = "opencv " + std::string(CV_VERSION) + ", " +
                               std::to_string(miaucam::all_gestures().size()) +
                               " gestures, static-frame flow " + std::to_string(flow.magnitude);
    return env->NewStringUTF(report.c_str());
}
