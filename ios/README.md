# miaucam ios

SwiftUI app with the same shape as the Android one: a full-screen meme, a
small round camera preview, and a front/back switch. Gesture detection runs
MediaPipe Tasks in Swift and hands the landmarks through `MiaucamCore.mm` to
`advance()` in `src/frame_pipeline.cpp`, the same code as desktop and Android.

The Xcode project is generated from `project.yml` (XcodeGen), so there is no
`.xcodeproj` in git. Everything below needs a Mac, so CI does it for you.

## CI

- `.github/workflows/ios.yml` builds for the simulator on every push touching
  `ios/` or `src/`. No signing, no secrets.
- `.github/workflows/ios-release.yml` (run by hand from the Actions tab)
  archives, signs and uploads to TestFlight.

The OpenCV xcframework (core, imgproc, video) is built from source by
`scripts/build_opencv.sh` and cached, so the first run is slow.

## One-time release setup

1. Register the App ID `com.miaucam.app` and create the app in App Store
   Connect (or change `PRODUCT_BUNDLE_IDENTIFIER` in `project.yml`).
2. In App Store Connect, Users and Access, Integrations, create an API key
   with the App Manager role. Download the `.p8` once.
3. Add these repository secrets: `APPLE_TEAM_ID`, `ASC_KEY_ID`,
   `ASC_ISSUER_ID`, and `ASC_KEY_P8_BASE64` (`base64 -w0 AuthKey_XXXX.p8`).
4. Run the ios-release workflow, then install from the TestFlight app.

## Not done yet

- App icon: App Store uploads are rejected without a 1024x1024 icon.
- Face transform matrix: not passed yet (head-pose gestures like side eye see
  a yaw of 0). The Swift API for it needs checking against a real build.
- Nothing here has been compiled: the first CI runs will surface the errors.
