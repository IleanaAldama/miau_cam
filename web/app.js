import create_miaucam_core from "./dist/miaucam_core.mjs";
import {
  FaceLandmarker,
  FilesetResolver,
  HandLandmarker,
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.14/vision_bundle.mjs";

const MEDIAPIPE_WASM = "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.14/wasm";
const FRAME_WIDTH = 480;
const MIN_HAND_CONFIDENCE = 0.6;

// Face-mesh points the core reads (upper lip, lower lip, right cheek, left
// cheek) and the blendshapes it reads, in the order the core expects.
const FACE_KEYPOINTS = [13, 14, 234, 454];
const EXPRESSION_KEYS = [
  "jawOpen",
  "mouthSmileLeft",
  "mouthSmileRight",
  "browInnerUp",
  "eyeBlinkLeft",
  "eyeBlinkRight",
  "eyeWideLeft",
  "eyeWideRight",
];
const EXPRESSION_INDEX = new Map(EXPRESSION_KEYS.map((key, index) => [key, index]));

const cam = document.getElementById("cam");
const memeImg = document.getElementById("meme");
const memeVideo = document.getElementById("meme-video");
const status = document.getElementById("status");

const canvas = document.createElement("canvas");
const ctx = canvas.getContext("2d", { willReadFrequently: true });

let facing = "user";
let paused = false;
let last_ts = 0;
let gesture = -1;
let fps_frames = 0;
let fps_since = performance.now();

async function create_landmarkers() {
  const fileset = await FilesetResolver.forVisionTasks(MEDIAPIPE_WASM);
  const build = async (delegate) => {
    const [hand, face] = await Promise.all([
      HandLandmarker.createFromOptions(fileset, {
        baseOptions: { modelAssetPath: "../models/hand_landmarker.task", delegate },
        runningMode: "VIDEO",
        numHands: 2,
        minHandDetectionConfidence: MIN_HAND_CONFIDENCE,
        minHandPresenceConfidence: MIN_HAND_CONFIDENCE,
      }),
      FaceLandmarker.createFromOptions(fileset, {
        baseOptions: { modelAssetPath: "../models/face_landmarker.task", delegate },
        runningMode: "VIDEO",
        numFaces: 1,
        outputFaceBlendshapes: true,
        outputFacialTransformationMatrixes: true,
      }),
    ]);
    return { hand, face };
  };
  return build("GPU").catch(() => build("CPU"));
}

async function start_camera() {
  if (!navigator.mediaDevices?.getUserMedia) {
    throw new Error("the camera needs a secure page: open this over https:// (or localhost)");
  }
  cam.srcObject?.getTracks().forEach((track) => track.stop());
  const stream = await navigator.mediaDevices.getUserMedia({
    video: { facingMode: { ideal: facing }, width: { ideal: 640 }, height: { ideal: 480 } },
  });
  cam.srcObject = stream;
  cam.classList.toggle("mirrored", facing === "user");
  await cam.play();
}

function flatten(points) {
  return points.flatMap((p) => [p.x, p.y, p.z]);
}

function face_keypoints(landmarks) {
  return landmarks ? flatten(FACE_KEYPOINTS.map((index) => landmarks[index])) : [];
}

function expression_scores(categories) {
  const scores = new Float32Array(EXPRESSION_KEYS.length);
  for (const category of categories) {
    const index = EXPRESSION_INDEX.get(category.categoryName);
    if (index !== undefined) scores[index] = category.score;
  }
  return scores;
}

const meme_url = (file) => `../memes/${encodeURIComponent(file)}`;

// Warms the HTTP cache one file at a time so later gestures show instantly.
async function preload_memes(core) {
  for (let index = 0; index < core.gesture_count(); index++) {
    for (const file of core.meme_files(index)) {
      await fetch(meme_url(file)).then((response) => response.blob()).catch(() => {});
    }
  }
}

function show_meme(core, index) {
  const files = core.meme_files(index);
  const file = files[Math.floor(Math.random() * files.length)];
  const url = meme_url(file);

  if (core.is_video(index)) {
    memeVideo.src = url;
    memeVideo.hidden = false;
    memeImg.hidden = true;
    memeVideo.play();
  } else {
    memeVideo.pause();
    memeVideo.hidden = true;
    memeImg.src = url;
    memeImg.hidden = false;
  }
}

function process_frame(core, landmarkers) {
  const width = FRAME_WIDTH;
  const height = Math.round((cam.videoHeight * width) / cam.videoWidth);
  canvas.width = width;
  canvas.height = height;

  ctx.save();
  if (facing === "user") {
    ctx.translate(width, 0);
    ctx.scale(-1, 1);
  }
  ctx.drawImage(cam, 0, 0, width, height);
  ctx.restore();

  const ts = Math.max(Math.floor(performance.now()), last_ts + 1);
  last_ts = ts;

  const hands = landmarkers.hand.detectForVideo(canvas, ts);
  const faces = landmarkers.face.detectForVideo(canvas, ts);

  const pixels = ctx.getImageData(0, 0, width, height).data;
  core.frame_view(pixels.length).set(pixels);

  return core.advance(
    width,
    height,
    ts,
    hands.landmarks.flatMap(flatten),
    face_keypoints(faces.faceLandmarks[0]),
    expression_scores(faces.faceBlendshapes[0]?.categories ?? []),
    faces.facialTransformationMatrixes[0]?.data ?? [],
  );
}

function update_fps() {
  fps_frames += 1;
  const now = performance.now();
  if (now - fps_since < 1000) return;
  status.classList.remove("blink");
  status.textContent = `${fps_frames} fps, gesture ${gesture}`;
  fps_frames = 0;
  fps_since = now;
}

async function main() {
  const [core, landmarkers] = await Promise.all([create_miaucam_core(), create_landmarkers()]);
  core.reset();
  await start_camera();

  document.getElementById("switch").onclick = async () => {
    facing = facing === "user" ? "environment" : "user";
    await start_camera().catch(() => {
      facing = facing === "user" ? "environment" : "user";
    });
    core.reset();
  };

  const pause_button = document.getElementById("pause");
  pause_button.onclick = () => {
    paused = !paused;
    pause_button.textContent = paused ? "Resume" : "Pause";
    if (paused) {
      cam.pause();
      memeVideo.pause();
      status.textContent = "paused";
    } else {
      cam.play();
      if (!memeVideo.hidden) memeVideo.play();
      core.reset();
    }
  };

  const tick = () => {
    if (!paused && cam.readyState >= 2 && cam.videoWidth > 0) {
      const next = process_frame(core, landmarkers);
      if (next !== gesture) {
        gesture = next;
        show_meme(core, next);
      }
      memeImg.style.transform = core.flip_meme() ? "scaleX(-1)" : "";
      update_fps();
    }
    requestAnimationFrame(tick);
  };
  requestAnimationFrame(tick);
  preload_memes(core);
}

main().catch((error) => {
  status.classList.remove("blink");
  status.textContent = `error: ${error.message ?? error}`;
  console.error(error);
});
