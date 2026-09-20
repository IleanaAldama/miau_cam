# miaucam android

A real app now: a full-screen meme, a small round camera preview in the
corner, and a button to switch between front and back camera. The meme
shown is a static placeholder for now, not gesture-driven yet - see "What's
not here yet" below.

## One-time setup

You need the Android SDK (platform, build-tools, a system image if you
want an emulator) and the NDK. If you don't already have these from
Android Studio, here's how to get them without it, using just the
command-line tools:

```
mkdir -p ~/Android/Sdk/cmdline-tools
curl -sL https://dl.google.com/android/repository/commandlinetools-linux-9862592_latest.zip -o /tmp/cmdline-tools.zip
unzip -q /tmp/cmdline-tools.zip -d ~/Android/Sdk/cmdline-tools/
mv ~/Android/Sdk/cmdline-tools/cmdline-tools ~/Android/Sdk/cmdline-tools/latest

export ANDROID_HOME=~/Android/Sdk
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"
yes | sdkmanager --licenses
sdkmanager "platforms;android-34" "build-tools;34.0.0" "platform-tools"
```

For the emulator, also grab a system image and create an AVD:

```
sdkmanager "system-images;android-34;google_apis;x86_64"
avdmanager create avd -n miaucam_test -k "system-images;android-34;google_apis;x86_64" -d pixel_5
```

Write `sdk.dir=/path/to/your/Android/Sdk` into `android/local.properties`
(machine-specific, gitignored - not something to commit).

## Building the app

```
cd android
./gradlew assembleDebug
```

The wrapper downloads its own pinned Gradle (8.7) on first run, so you
don't need Gradle installed system-wide - useful since distro-packaged
Gradle tends to be too old for a current Android Gradle Plugin. Output:
`android/app/build/outputs/apk/debug/app-debug.apk`.

## Running on the emulator

Boot it (headless works fine if you're just driving it through adb; drop
`-no-window` if you want to actually watch it):

```
export PATH="$ANDROID_HOME/emulator:$ANDROID_HOME/platform-tools:$PATH"
emulator -avd miaucam_test -no-window -no-audio -gpu swiftshader_indirect &
adb wait-for-device
until [ "$(adb shell getprop sys.boot_completed | tr -d '\r')" = "1" ]; do sleep 3; done
```

`swiftshader_indirect` is software-rendered GL, which matters if the
machine can't use KVM (e.g. your user isn't in the `kvm` group) - the
emulator still boots, just slower than with hardware acceleration.

Install, grant the camera permission, and launch:

```
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell pm grant com.miaucam.app android.permission.CAMERA
adb shell am start -n com.miaucam.app/.MainActivity
```

Freshly-created AVDs sometimes ship with the front camera disabled
(`hw.camera.front = none` in `~/.android/avd/<name>.avd/config.ini`),
which throws when the app tries to bind it. The app itself checks
`hasCamera()` and falls back to whichever camera actually exists rather
than crashing, but if you want both to work for testing, edit that line to
`emulated` and reboot the emulator for it to take effect.

Useful while it's running:

```
adb logcat -d '*:E' | grep -i miaucam    # crash check
adb shell screencap -p /sdcard/screen.png && adb pull /sdcard/screen.png .
```

## Native code

`app/src/main/cpp/` builds `libmiaucam_native.so` through Gradle. It compiles
the shared domain sources from `../src` (gesture, hand_classifier,
face_signals, optical_flow, gesture_state) against OpenCV's Android SDK,
for `arm64-v8a` and `x86_64`. `NativeCore.selfTest()` runs a flow pass on a
synthetic frame at startup and logs the result under the `miaucam` tag.

One-time setup: unpack OpenCV's Android SDK into `android/third_party/opencv`
(gitignored), and make the NDK visible to the SDK:

```
curl -L -o /tmp/opencv.zip https://github.com/opencv/opencv/releases/download/4.9.0/opencv-4.9.0-android-sdk.zip
unzip -q /tmp/opencv.zip -d android/third_party && mv android/third_party/OpenCV-android-sdk android/third_party/opencv
mkdir -p ~/Android/Sdk/ndk && ln -s /usr/lib/android-sdk/ndk/25.0.8775105 ~/Android/Sdk/ndk/25.0.8775105
```

## What's not here yet

MediaPipe for Android (Bazel), the JNI call into `step()` from
`src/miaucam_core.h`, and feeding CameraX `ImageAnalysis` frames to it. The
meme is still a static placeholder.
