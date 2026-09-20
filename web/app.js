import create_miaucam_core from "./dist/miaucam_core.mjs";
import {
  FaceLandmarker,
  FilesetResolver,
  HandLandmarker,
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.14/vision_bundle.mjs";

const MEDIAPIPE_WASM = "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.14/wasm";
const FRAME_WIDTH = 480;
const FACE_LANDMARK_COUNT = 468;

const cam = document.getElementById("cam");
const memeImg = document.getElementById("meme");
const memeVideo = document.getElementById("meme-video");
const status = document.getElementById("status");

const canvas = document.createElement("canvas");
const ctx = canvas.getContext("2d", { willReadFrequently: true });

let facing = "user";
let last_ts = 0;
let gesture = -1;
let fps_frames = 0;
let fps_since = performance.now();

async function create_landmarkers() {
  const fileset = await FilesetResolver.forVisionTasks(MEDIAPIPE_WASM);
  const build = async (delegate) => ({
    hand: await HandLandmarker.createFromOptions(fileset, {
      baseOptions: { modelAssetPath: "../models/hand_landmarker.task", delegate },
      runningMode: "VIDEO",
      numHands: 2,
    }),
    face: await FaceLandmarker.createFromOptions(fileset, {
      baseOptions: { modelAssetPath: "../models/face_landmarker.task", delegate },
      runningMode: "VIDEO",
      numFaces: 1,
      outputFaceBlendshapes: true,
      outputFacialTransformationMatrixes: true,
    }),
  });
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

function show_meme(core, index) {
  const files = core.meme_files(index);
  const file = files[Math.floor(Math.random() * files.length)];
  const url = `../memes/${encodeURIComponent(file)}`;

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
  const shapes = faces.faceBlendshapes[0]?.categories ?? [];
  const face_points = (faces.faceLandmarks[0] ?? []).slice(0, FACE_LANDMARK_COUNT);

  const pixels = ctx.getImageData(0, 0, width, height).data;
  core.frame_view(pixels.length).set(pixels);

  return core.advance(
    width,
    height,
    ts,
    hands.landmarks.flatMap(flatten),
    flatten(face_points),
    shapes.map((s) => s.categoryName),
    shapes.map((s) => s.score),
    faces.facialTransformationMatrixes[0]?.data ?? [],
  );
}

function update_fps() {
  fps_frames += 1;
  const now = performance.now();
  if (now - fps_since < 1000) return;
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

  const tick = () => {
    if (cam.readyState >= 2 && cam.videoWidth > 0) {
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
}

main().catch((error) => {
  status.textContent = `error: ${error.message ?? error}`;
  console.error(error);
});
