# miaucam

Webcam gesture → meme detector. Two windows open side by side: **Camera**
(your webcam feed with hand landmarks drawn on top) and **Meme** (the cat
meme matching whatever gesture you're making).

Press `q` or `Esc` to quit.

## Building & running

```
cmake -S . -B build
cmake --build build
./build/miaucam
```

First build fetches and compiles MediaPipe via Bazel (see `bridge/` and
`CMakeLists.txt`) — it's slow once, fast after.

## Architecture

The app is split by domain instead of living in one big file. Each piece
in `src/` does one job and mostly doesn't know the others exist:

`geometry` holds the plain vector math (distances, angles) that everything
else builds on. `hand_classifier` looks at a detected hand and works out
its shape: which fingers are up, whether it's curled into a fist. `face_signals`
does the same for a face: where the mouth is, how wide the eyes are, the
blendshape scores MediaPipe hands back. `optical_flow` is separate from all
of that; it just watches how much the whole frame is moving, which is what
the spin gesture is built on. `gesture_state` is the piece that actually
decides what you're doing, by combining what the other three report. It's
written as plain data plus free functions rather than a class with methods,
so a frame's worth of state gets passed in and a new one comes back out.
`meme_catalog` only knows about gesture names and files on disk, nothing
about detection. `landmarker_sessions` owns the two MediaPipe models and
hides the pixel format details. `hud` turns state into the debug text and
landmark drawing you see on screen. `main.cpp` just wires all of that
together and runs the loop.

MediaPipe itself is the odd one out, since it's built with Bazel rather
than CMake and isn't distributed as a normal library you can just link
against. To work around that, `bridge/` holds a small, hand written wrapper
around MediaPipe's hand and face landmark APIs, built as its own shared
library and linked into the CMake project like any other dependency. Our
own code never touches MediaPipe's internals directly, only the plain
structs the bridge hands back.

Errors are handled with a small `Result<T>` type instead of exceptions, so
setup code (loading models, opening the camera, reading files) can fail
and be checked at the call site rather than needing a try/catch somewhere.

## Data flow

Every frame goes through the same sequence. The webcam hands over a raw
frame, which gets mirrored horizontally so it feels like a selfie camera
instead of a security camera. That mirrored frame is fed to two things at
once: the optical flow tracker, which compares it against the previous
frame to see how much motion happened, and MediaPipe, which runs hand and
face landmark detection on it.

Whatever MediaPipe finds gets folded into the running gesture state:
detected hands are turned into shape descriptions, a detected face is
turned into mouth position, head angle and expression scores, and the
optical flow result updates whether a spin is currently happening. All of
those readings are smoothed a little from frame to frame, since raw
landmark positions jitter even when you're holding still, and jittery
input made gestures flicker before that smoothing was added.

With the state updated, the app asks it for a single answer: which gesture,
if any, matches right now. That answer only becomes the gesture actually
shown once it's held steady for a handful of frames in a row, so a single
noisy frame can't flip the display. Once a gesture is settled on, its meme
gets looked up (or, for the video gestures, the next frame of its clip
gets read) and both windows get redrawn.

## How gestures are read

A couple of things apply to every gesture below:

- **Hold it for ~5 frames** before the meme switches — a single flickery
  frame won't trigger a change.
- If you stop making a gesture, it falls back to the default poker-face cat
  after about 600ms, not instantly.
- **Priority matters.** If two conditions could both match at once (e.g.
  spinning while also making a hand shape), the one listed first below
  wins. Full order: spin → no-hands poses → mouth-open-with-a-hand →
  hand-shape—specific gestures.
- The **Camera** window's top-left corner shows a live debug readout (yaw,
  pitch, jaw-open, flow magnitude, etc.) with the exact threshold each one
  needs to clear — handy for seeing why something isn't triggering.

## Gesture guide

| Gesture | How to trigger it | Meme |
|---|---|---|
| **Spin** | Spin around in your chair for a couple of seconds — needs sustained motion across most of a ~2.2s window, not just one quick turn. Beats every other gesture while active. | `spin cat.mov` (video) |
| **Huh** | Mouth open **and** eyes wide, with **no hands** visible. | `huh.png` |
| **Side-eye** | Turn your head to the side (no hands visible). | `side eye cat.jpg` |
| **Side-eye (down)** | Tilt your head down instead of turning it (no hands visible). | `side eye.png` |
| **Laugh & point** | Open your mouth **with any hand visible** anywhere in frame — the hand's shape doesn't matter. | `laugh and point .jpg` |
| **Two fingers together** | Point with both hands (index finger only) and bring the fingertips close together. | `uwucat.jpg`, `uwucatt.jpg`, or `fingers together muehehe .jpg` |
| **Two hands on head** | Bring both hands up near your face, above the top of your head. | `two hands on head .jpg` |
| **Crash-out** | Both hands clenched into fists, held up near your face. | `crashout cat .jpg` |
| **Dance** | Both hands open (fingers spread), one held near the top of the frame and the other near the bottom — doesn't matter which is which. | `two palms up.mov` (video) |
| **Fist** | One hand, clenched into a fist. | `punchcat.jpg` |
| **Rockstar / shaka** | One hand: thumb and pinky out, other three fingers curled. | `cat.jpg` |
| **Shhh** | One hand, index finger up, fingertip close to your mouth. | `shhcat.jpg` |
| **One finger up** | One hand, index finger up, *not* near your mouth. | `profcat.jpg` or `professorcat.jpg` |
| **Hand covering face** | One hand held roughly where your mouth is. | `hand cover face .jpg` |
| **Hand stretched out** | One open hand (fingers spread), held away from your face. | `hand stretched out, palm facing up .jpg` |
| **Default** | No hands, no notable face pose — the resting state. | `pokercat.jpg` |

## Tuning

All the numeric thresholds above (how far a "turn" has to be to count as
side-eye, how long a spin has to sustain, how close fingertips need to be
for "together," etc.) live in `src/tuning.h`, grouped by which gesture they
belong to, with comments on how each was derived. If a gesture isn't
triggering reliably for you, that's the file to adjust — watch the debug
HUD in the Camera window while you retune.

## Credit

This whole project is a C++ port of a Python original, [meowmeowcatcam by
catherpiee](https://github.com/catherpiee/meowmeowcatcam), which is where
every gesture, threshold and meme choice here actually came from. All the
hard-won tuning (why the spin detector looks at a trailing window instead
of a single burst, why huh cat needed its own jaw threshold, and so on)
happened there first. This port just carries that design over into C++
with a different architecture around it. Thank you to catherpiee for
building it and figuring all of that out.
