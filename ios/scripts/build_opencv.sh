#!/usr/bin/env bash
# Builds a slim opencv2.xcframework (core, imgproc, video) into ios/third_party.
set -euo pipefail

version=4.9.0
cd "$(dirname "$0")/.."
mkdir -p third_party
cd third_party

[ -d opencv-src ] || git clone --depth 1 --branch "$version" https://github.com/opencv/opencv.git opencv-src

python3 opencv-src/platforms/apple/build_xcframework.py \
  --out opencv-build \
  --iphoneos_archs arm64 \
  --iphonesimulator_archs arm64,x86_64 \
  --disable-swift \
  --without calib3d --without features2d --without flann --without dnn --without gapi \
  --without highgui --without imgcodecs --without ml --without objdetect --without photo \
  --without stitching --without videoio --without java --without python

rm -rf opencv2.xcframework
mv opencv-build/opencv2.xcframework opencv2.xcframework
