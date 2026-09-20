#include <jni.h>

#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "frame_pipeline.h"
#include "gesture.h"

namespace {

miaucam::CoreState g_state;

std::vector<miaucam::Point3> to_points(const std::vector<float>& flat) {
    std::vector<miaucam::Point3> points;
    for (size_t i = 0; i + 2 < flat.size(); i += 3) {
        points.push_back({flat[i], flat[i + 1], flat[i + 2]});
    }
    return points;
}

std::vector<float> to_floats(JNIEnv* env, jfloatArray array) {
    const jsize size = env->GetArrayLength(array);
    std::vector<float> out(static_cast<size_t>(size));
    env->GetFloatArrayRegion(array, 0, size, out.data());
    return out;
}

std::string to_string(JNIEnv* env, jstring s) {
    const char* chars = env->GetStringUTFChars(s, nullptr);
    std::string out(chars);
    env->ReleaseStringUTFChars(s, chars);
    return out;
}

miaucam::DetectionResult build_detection(JNIEnv* env, jfloatArray hands, jfloatArray face,
                                         jobjectArray names, jfloatArray scores,
                                         jfloatArray transform) {
    miaucam::DetectionResult detection;

    const auto hand_floats = to_floats(env, hands);
    constexpr size_t floats_per_hand = 21 * 3;
    for (size_t i = 0; i + floats_per_hand <= hand_floats.size(); i += floats_per_hand) {
        std::vector<float> one(hand_floats.begin() + i, hand_floats.begin() + i + floats_per_hand);
        detection.hand.hands.push_back({to_points(one)});
    }

    detection.face.landmarks = to_points(to_floats(env, face));
    detection.face.has_face = !detection.face.landmarks.empty();

    const auto score_values = to_floats(env, scores);
    for (jsize i = 0; i < static_cast<jsize>(score_values.size()); ++i) {
        auto name = static_cast<jstring>(env->GetObjectArrayElement(names, i));
        detection.face.blendshapes.push_back({to_string(env, name), score_values[i]});
        env->DeleteLocalRef(name);
    }

    const auto matrix = to_floats(env, transform);
    if (matrix.size() == 16) {
        detection.face.has_transform = true;
        std::copy(matrix.begin(), matrix.end(), detection.face.transform);
    }
    return detection;
}

}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_com_miaucam_app_NativeCore_reset(JNIEnv*, jobject) {
    g_state = miaucam::CoreState{};
}

JNIEXPORT jint JNICALL Java_com_miaucam_app_NativeCore_advance(
    JNIEnv* env, jobject, jobject rgba, jint width, jint height, jdouble timestamp_ms,
    jfloatArray hands, jfloatArray face, jobjectArray names, jfloatArray scores,
    jfloatArray transform) {
    auto* pixels = static_cast<uint8_t*>(env->GetDirectBufferAddress(rgba));
    cv::Mat rgba_mat(height, width, CV_8UC4, pixels);
    cv::Mat rgb;
    cv::cvtColor(rgba_mat, rgb, cv::COLOR_RGBA2RGB);

    const miaucam::FrameInput input{{rgb.data, width, height, static_cast<int>(rgb.step)},
                                    timestamp_ms};
    auto detection = build_detection(env, hands, face, names, scores, transform);
    auto [next, output] = miaucam::advance(std::move(g_state), input, std::move(detection));
    g_state = std::move(next);
    return static_cast<jint>(output.gesture);
}

JNIEXPORT jobjectArray JNICALL Java_com_miaucam_app_NativeCore_memeFiles(JNIEnv* env, jobject,
                                                                          jint gesture) {
    const auto& files = miaucam::all_gestures().at(static_cast<size_t>(gesture)).files;
    jobjectArray out = env->NewObjectArray(static_cast<jsize>(files.size()),
                                           env->FindClass("java/lang/String"), nullptr);
    for (jsize i = 0; i < static_cast<jsize>(files.size()); ++i) {
        env->SetObjectArrayElement(out, i, env->NewStringUTF(files[i].c_str()));
    }
    return out;
}

JNIEXPORT jboolean JNICALL Java_com_miaucam_app_NativeCore_isVideo(JNIEnv*, jobject, jint gesture) {
    return miaucam::all_gestures().at(static_cast<size_t>(gesture)).is_video;
}

}  // extern "C"
