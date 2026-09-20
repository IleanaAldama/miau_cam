# miaucam web

The browser version: MediaPipe Tasks Web finds the hand and face landmarks,
and the shared C++ core (`src/`, compiled to WebAssembly with our own slim
OpenCV) turns them into a gesture. Same `advance()` as desktop, Android and
iOS. Nothing leaves the browser.

## Build

Needs the Emscripten SDK in `~/emsdk` (`emsdk install latest && emsdk activate latest`).

```
web/scripts/build_opencv.sh   # once: static OpenCV (core, imgproc, video) for wasm
web/scripts/build.sh          # builds web/dist/miaucam_core.mjs + .wasm
```

## Run

Serve the repo root (the page reads `models/` and `memes/` from it) and open
`/web/`. `localhost` counts as a secure context, so the camera works:

```
python3 -m http.server 8000
```

Then open http://localhost:8000/web/. Anywhere else needs HTTPS.

MediaPipe's own wasm files are loaded from the jsDelivr CDN for now.

## Deploy

`.github/workflows/web.yml` builds the wasm and publishes `web/`, `models/` and
`memes/` to GitHub Pages on every push to main. One-time setup: in the repo,
Settings, Pages, set Source to "GitHub Actions". The page is then at
`https://<user>.github.io/<repo>/`.
