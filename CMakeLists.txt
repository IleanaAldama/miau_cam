cmake_minimum_required(VERSION 3.16)
project(miaucam LANGUAGES CXX)

# C++17 (the app code uses std::variant/std::visit, std::optional,
# std::clamp and structured bindings - all "C++14 or newer" is happy with,
# but they specifically need 17).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only pull in the modules gesture_meme.py actually uses. In particular we
# must NOT link opencv_dnn: Ubuntu's build of it dynamically links the
# system libprotobuf, while MediaPipe (below) statically links its own
# (different, incompatible) protobuf into the same process - both try to
# register the same well-known .proto descriptors into the global
# DescriptorPool at startup, which aborts with a protobuf CHECK failure.
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui video)

# --- MediaPipe (built via Bazel, linked in as a prebuilt shared library) ---
#
# MediaPipe's own build system is Bazel, not CMake, and it isn't published
# as a prebuilt package anywhere (no apt/vcpkg/conan, no CMake config) - the
# only way to get its C++ Tasks API is to compile it from its own source
# tree. That source tree (third_party/mediapipe/) is pure build INPUT, not
# our code: it's fetched automatically below, pinned to a known-good commit,
# and gitignored entirely - deleting it and reconfiguring regenerates it.
#
# Our own bridge code lives in bridge/ (tracked normally in git) and wraps
# the HandLandmarker/FaceLandmarker Tasks C++ API behind a plain header, so
# CMake/main.cpp never sees MediaPipe's internal build graph or types - only
# bridge/miaucam_bridge.h and the resulting .so. Bazel needs our bridge's
# BUILD file to live inside the MediaPipe workspace to resolve its internal
# target labels, so the build step below mirrors bridge/ into the vendored
# tree before invoking bazel; bridge/ is what you actually edit.
set(MEDIAPIPE_DIR ${CMAKE_SOURCE_DIR}/third_party/mediapipe)
set(MEDIAPIPE_COMMIT d4f197be5af7c0db497a9eea6ee07d1d4710d2be)
set(BRIDGE_SRC_DIR ${CMAKE_SOURCE_DIR}/bridge)
set(MEDIAPIPE_BRIDGE_PKG mediapipe/examples/desktop/miaucam)
set(MEDIAPIPE_BRIDGE_PKG_DIR ${MEDIAPIPE_DIR}/${MEDIAPIPE_BRIDGE_PKG})
set(MEDIAPIPE_BRIDGE_LIB ${MEDIAPIPE_DIR}/bazel-bin/${MEDIAPIPE_BRIDGE_PKG}/libmiaucam_mediapipe.so)

find_program(BAZEL_EXECUTABLE bazel REQUIRED)
find_package(Git REQUIRED)

# One-time setup: fetch MediaPipe at the pinned commit and apply the local
# patch that fixes its OpenCV BUILD rule for Ubuntu/Debian's opencv4 layout
# (see bridge/opencv_linux.patch for why). Only runs if the tree is missing,
# so it survives `rm -rf third_party/mediapipe` but not repeated
# reconfigures.
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

# Every build: mirror our tracked bridge sources into the vendored tree,
# then ask Bazel to build the .so. Depending on the bridge sources (not
# just running once at configure time) means editing bridge/miaucam_bridge.cc
# and running `cmake --build build` alone picks up the change.
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
    src/hand_classifier.cpp
    src/face_signals.cpp
    src/optical_flow.cpp
    src/gesture_state.cpp
    src/meme_catalog.cpp
    src/landmarker_service.cpp
    src/hud.cpp
)
target_include_directories(miaucam PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(miaucam PRIVATE ${OpenCV_LIBS} mediapipe_bridge)

# The binary lands in build/, not the project root, so relative paths like
# "models/..." only work if you happen to run from the root. Bake the root
# in at compile time instead (mirrors gesture_meme.py's
# ROOT = Path(__file__).parent, which is likewise independent of cwd).
target_compile_definitions(miaucam PRIVATE MIAUCAM_ROOT="${CMAKE_SOURCE_DIR}")
