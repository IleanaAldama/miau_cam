# miaucam

Webcam gesture → meme detector. Two windows open side by side: **Camera** (webcam feed with landmarks) and **Meme** (cat meme matching your gesture).

## Projects

### Desktop (C++)
The main application using MediaPipe via a custom C++ bridge.

**Building & running**
```bash
cmake -S . -B build
cmake --build build
./build/miaucam
```
*Note: First build fetches/compiles MediaPipe via Bazel (slow once, fast after).*

**Tests**
```bash
cmake --build build --target miaucam_tests
ctest --test-dir build
```

### Web (Wasm)
A browser version using MediaPipe Tasks Web and the core C++ logic compiled to WebAssembly.

**Building**
Requires the [Emscripten SDK](https://emscripten.org/).
```bash
web/scripts/build_opencv.sh   # once: static OpenCV for wasm
web/scripts/build.sh          # builds web/dist/miaucam_core.mjs + .wasm
```

**Running**
Serve the repo root and open `/web/`:
```bash
python3 -m http.server 8000
```
Then open `http://localhost:8000/web/`.

---

## Architecture

The app is split by domain in `src/`:
- `geometry`: Vector math.
- `hand_classifier`: Hand shape detection.
- `face_signals`: Smoothed face signals (`FaceSignals`) from four keypoints and the expression scores.
- `optical_flow`: Frame-to-frame motion, reduced in one pass to a spin signal.
- `gesture_state`: Perception state, plus `decide()`, an ordered list of priority rules.
- `debounce`: Pure step that turns the jumpy per-frame gesture into the one shown.
- `frame_pipeline`: `advance()` composes flow, face, decide and debounce; shared by every front end.
- `meme_catalog`, `meme_view`: Gesture to files, and pure meme rendering with a cache.
- `landmarker_sessions`: Manages MediaPipe models.
- `hud`: Debug overlay.

A `bridge/` layer wraps MediaPipe's Bazel-built APIs for use with CMake.

## Gesture guide

| Gesture | How to trigger it | Meme |
|---|---|---|
| **Spin** | Sustained motion/spinning | `spin cat.mov` |
| **Huh** | Mouth open + eyes wide (no hands) | `huh.png` |
| **Side-eye** | Turn head to the side | `side eye cat.jpg` |
| **Laugh & point** | Open mouth + any hand visible | `laugh and point .jpg` |
| **Two fingers** | Both index fingers close together | `uwucat.jpg` |
| **Two hands on head** | Hands above head | `two hands on head .jpg` |
| **Crash-out** | Both hands in fists near face | `crashout cat .jpg` |
| **Dance** | Both hands open, top/bottom of frame | `two palms up.mov` |
| **Fist** | One hand in a fist | `punchcat.jpg` |
| **Rockstar** | Thumb and pinky out | `cat.jpg` |
| **Shhh** | Index finger near mouth | `shhcat.jpg` |
| **One finger up** | One index finger up | `profcat.jpg` |
| **Hand covering face** | Hand over mouth | `hand cover face .jpg` |
| **Hand stretched** | Open hand away from face | `hand stretched out.jpg` |
| **Default** | No notable gesture | `pokercat.jpg` |

## Tuning

Thresholds are in `src/tuning.h`. Watch the debug HUD in the Camera window to retune.

## Credit

C++ port of [meowmeowcatcam by catherpiee](https://github.com/catherpiee/meowmeowcatcam).
