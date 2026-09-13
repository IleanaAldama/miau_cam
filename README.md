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
