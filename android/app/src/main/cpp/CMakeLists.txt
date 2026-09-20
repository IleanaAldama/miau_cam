cmake_minimum_required(VERSION 3.22)
project(miaucam_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(MIAUCAM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)

set(OpenCV_STATIC ON)
set(OpenCV_DIR "${MIAUCAM_ROOT}/android/third_party/opencv/sdk/native/jni")
find_package(OpenCV REQUIRED core imgproc video)

add_library(miaucam_native SHARED
    native_bridge.cpp
    ${MIAUCAM_ROOT}/src/gesture.cpp
    ${MIAUCAM_ROOT}/src/hand_classifier.cpp
    ${MIAUCAM_ROOT}/src/face_signals.cpp
    ${MIAUCAM_ROOT}/src/optical_flow.cpp
    ${MIAUCAM_ROOT}/src/gesture_state.cpp
)
target_include_directories(miaucam_native PRIVATE ${MIAUCAM_ROOT}/src ${MIAUCAM_ROOT}/bridge)
target_link_libraries(miaucam_native PRIVATE ${OpenCV_LIBS} log)
