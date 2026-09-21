cmake_minimum_required(VERSION 3.22)
project(miaucam_web LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(MIAUCAM_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(OPENCV_PREFIX "${CMAKE_CURRENT_LIST_DIR}/third_party/opencv-install")

add_executable(miaucam_core
    src/wasm_bridge.cpp
    ${MIAUCAM_ROOT}/src/gesture.cpp
    ${MIAUCAM_ROOT}/src/hand_classifier.cpp
    ${MIAUCAM_ROOT}/src/face_signals.cpp
    ${MIAUCAM_ROOT}/src/optical_flow.cpp
    ${MIAUCAM_ROOT}/src/gesture_state.cpp
    ${MIAUCAM_ROOT}/src/frame_pipeline.cpp
    ${MIAUCAM_ROOT}/src/debounce.cpp
    ${MIAUCAM_ROOT}/src/detection_input.cpp
)
target_include_directories(miaucam_core PRIVATE
    ${MIAUCAM_ROOT}/src ${MIAUCAM_ROOT}/bridge ${OPENCV_PREFIX}/include/opencv4)
target_link_directories(miaucam_core PRIVATE ${OPENCV_PREFIX}/lib ${OPENCV_PREFIX}/lib/opencv4/3rdparty)
target_link_libraries(miaucam_core PRIVATE opencv_video opencv_imgproc opencv_core zlib embind)

set_target_properties(miaucam_core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/dist
    SUFFIX ".mjs")
target_link_options(miaucam_core PRIVATE
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=create_miaucam_core
    -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web,node -O3)
