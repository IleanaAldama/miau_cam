#!/usr/bin/env bash
# Builds static OpenCV (core, imgproc, video) for WebAssembly into web/third_party/opencv-install.
set -euo pipefail

cd "$(dirname "$0")/.."
source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" > /dev/null

version=4.9.0
[ -d third_party/opencv-$version ] || {
  mkdir -p third_party
  curl -sL https://github.com/opencv/opencv/archive/refs/tags/$version.tar.gz | tar xz -C third_party
}

emcmake cmake -S third_party/opencv-$version -B third_party/opencv-build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/third_party/opencv-install" \
  -DBUILD_LIST=core,imgproc,video \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_opencv_apps=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF -DBUILD_DOCS=OFF -DBUILD_JAVA=OFF \
  -DWITH_PTHREADS_PF=OFF -DWITH_OPENCL=OFF -DWITH_IPP=OFF -DWITH_ITT=OFF \
  -DWITH_JPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_OPENEXR=OFF \
  -DWITH_EIGEN=OFF -DWITH_LAPACK=OFF \
  -DCV_ENABLE_INTRINSICS=OFF -DCPU_BASELINE= -DCPU_DISPATCH=

cmake --build third_party/opencv-build -j"$(nproc)"
cmake --install third_party/opencv-build
