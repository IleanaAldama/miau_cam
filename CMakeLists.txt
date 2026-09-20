cmake_minimum_required(VERSION 3.16)

# clangd's --query-driver allowlist expects g++, not the generic c++ alias.
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(MIAUCAM_GXX g++)
    if(MIAUCAM_GXX)
        set(CMAKE_CXX_COMPILER ${MIAUCAM_GXX})
    endif()
endif()

project(miaucam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Not opencv_dnn: its system libprotobuf clashes with MediaPipe's own
# statically-linked protobuf (two runtimes, one process -> abort).
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui video)

# --- MediaPipe: fetched + built via Bazel, our own code stays in bridge/ ---
set(MEDIAPIPE_DIR ${CMAKE_SOURCE_DIR}/third_party/mediapipe)
set(MEDIAPIPE_COMMIT d4f197be5af7c0db497a9eea6ee07d1d4710d2be)
set(BRIDGE_SRC_DIR ${CMAKE_SOURCE_DIR}/bridge)
set(MEDIAPIPE_BRIDGE_PKG mediapipe/examples/desktop/miaucam)
set(MEDIAPIPE_BRIDGE_PKG_DIR ${MEDIAPIPE_DIR}/${MEDIAPIPE_BRIDGE_PKG})
set(MEDIAPIPE_BRIDGE_LIB ${MEDIAPIPE_DIR}/bazel-bin/${MEDIAPIPE_BRIDGE_PKG}/libmiaucam_mediapipe.so)

find_program(BAZEL_EXECUTABLE bazel REQUIRED)
find_package(Git REQUIRED)

# third_party/mediapipe/ is gitignored, disposable build input - fetched
# once at the pinned commit, patched for Ubuntu's opencv4 layout.
if(NOT EXISTS ${MEDIAPIPE_DIR}/.git)
    message(STATUS "Fetching MediaPipe @ ${MEDIAPIPE_COMMIT} (one-time, ~100MB)")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone https://github.com/google/mediapipe.git ${MEDIAPIPE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} checkout ${MEDIAPIPE_COMMIT}
        WORKING_DIRECTORY ${MEDIAPIPE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} apply ${BRIDGE_SRC_DIR}/opencv_linux.patch
        WORKING_DIRECTORY ${MEDIAPIPE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# Bazel needs our bridge's BUILD file inside the MediaPipe tree to resolve
# its target labels, so mirror bridge/ in on every build.
set(BRIDGE_SOURCES
    ${BRIDGE_SRC_DIR}/miaucam_bridge.h
    ${BRIDGE_SRC_DIR}/miaucam_bridge.cc
    ${BRIDGE_SRC_DIR}/miaucam_bridge.lds
    ${BRIDGE_SRC_DIR}/BUILD
)

add_custom_command(
    OUTPUT ${MEDIAPIPE_BRIDGE_LIB}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MEDIAPIPE_BRIDGE_PKG_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${BRIDGE_SOURCES} ${MEDIAPIPE_BRIDGE_PKG_DIR}/
    COMMAND ${BAZEL_EXECUTABLE} build -c opt //${MEDIAPIPE_BRIDGE_PKG}:libmiaucam_mediapipe.so
    WORKING_DIRECTORY ${MEDIAPIPE_DIR}
    DEPENDS ${BRIDGE_SOURCES}
    COMMENT "Building MediaPipe bridge library via Bazel (first run can take a long time)"
    VERBATIM
    USES_TERMINAL
)
add_custom_target(mediapipe_bridge_build DEPENDS ${MEDIAPIPE_BRIDGE_LIB})

add_library(mediapipe_bridge SHARED IMPORTED)
set_target_properties(mediapipe_bridge PROPERTIES
    IMPORTED_LOCATION ${MEDIAPIPE_BRIDGE_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${BRIDGE_SRC_DIR}
)
add_dependencies(mediapipe_bridge mediapipe_bridge_build)

add_executable(miaucam
    src/main.cpp
    src/gesture.cpp
    src/hand_classifier.cpp
    src/face_signals.cpp
    src/optical_flow.cpp
    src/gesture_state.cpp
    src/meme_catalog.cpp
    src/landmarker_sessions.cpp
    src/frame_pipeline.cpp
    src/miaucam_core.cpp
    src/hud.cpp
)
target_include_directories(miaucam PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(miaucam PRIVATE ${OpenCV_LIBS} mediapipe_bridge)

# Binary runs from build/, so bake the source root in for asset paths.
target_compile_definitions(miaucam PRIVATE MIAUCAM_ROOT="${CMAKE_SOURCE_DIR}")

# --- Tests: the pure-logic domains only, no MediaPipe/camera needed ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(miaucam_tests
    tests/geometry_test.cpp
    tests/hand_classifier_test.cpp
    tests/gesture_test.cpp
    tests/gesture_state_test.cpp
    tests/meme_catalog_test.cpp
    src/gesture.cpp
    src/hand_classifier.cpp
    src/face_signals.cpp
    src/optical_flow.cpp
    src/gesture_state.cpp
    src/meme_catalog.cpp
)
target_include_directories(miaucam_tests PRIVATE ${OpenCV_INCLUDE_DIRS} src ${BRIDGE_SRC_DIR})
target_link_libraries(miaucam_tests PRIVATE ${OpenCV_LIBS} GTest::gtest_main)
target_compile_definitions(miaucam_tests PRIVATE MIAUCAM_ROOT="${CMAKE_SOURCE_DIR}")

include(GoogleTest)
gtest_discover_tests(miaucam_tests)

# Keep compile_commands.json symlinked at the root for clangd.
file(REMOVE ${CMAKE_SOURCE_DIR}/compile_commands.json)
file(CREATE_LINK
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    SYMBOLIC
)
