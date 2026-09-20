#include <jni.h>

#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "frame_pipeline.h"
#include "detection_input.h"
#include "gesture.h"

namespace {

miaucam::CoreState g_state;

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

miaucam::RawDetection read_raw(JNIEnv* env, jfloatArray hands, jfloatArray face,
                               jobjectArray names, jfloatArray scores, jfloatArray transform) {
    miaucam::RawDetection raw{to_floats(env, hands), to_floats(env, face), {},
                              to_floats(env, scores), to_floats(env, transform)};
    for (jsize i = 0; i < env->GetArrayLength(names); ++i) {
        auto name = static_cast<jstring>(env->GetObjectArrayElement(names, i));
        raw.blendshape_names.push_back(to_string(env, name));
        env->DeleteLocalRef(name);
    }
    return raw;
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
    auto detection =
        miaucam::to_detection(read_raw(env, hands, face, names, scores, transform));
    auto [next, output] = miaucam::advance(std::move(g_state), input, std::move(detection));
    g_state = std::move(next);
    return static_cast<jint>(output.gesture);
}

JNIEXPORT jboolean JNICALL Java_com_miaucam_app_NativeCore_flipMeme(JNIEnv*, jobject) {
    return miaucam::flip_meme(g_state);
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
