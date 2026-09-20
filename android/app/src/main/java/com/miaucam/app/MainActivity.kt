package com.miaucam.app

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.BitmapFactory
import android.util.Log
import android.os.Bundle
import android.util.Size
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.core.resolutionselector.ResolutionSelector
import androidx.camera.core.resolutionselector.ResolutionStrategy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import java.util.concurrent.Executors
import kotlin.coroutines.resume

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme {
                Surface(color = Color.Black) {
                    var hasCameraPermission by remember {
                        mutableStateOf(
                            ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) ==
                                PackageManager.PERMISSION_GRANTED
                        )
                    }

                    val permissionLauncher = rememberLauncherForActivityResult(
                        ActivityResultContracts.RequestPermission()
                    ) { granted -> hasCameraPermission = granted }

                    LaunchedEffect(Unit) {
                        if (!hasCameraPermission) {
                            permissionLauncher.launch(Manifest.permission.CAMERA)
                        }
                    }

                    if (hasCameraPermission) {
                        MiaucamScreen()
                    } else {
                        Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                            Text("Camera permission is required", color = Color.White)
                        }
                    }
                }
            }
        }
    }
}

// CameraX's ListenableFuture as a suspend call, so binding doesn't block the main thread.
private suspend fun awaitCameraProvider(context: Context): ProcessCameraProvider =
    suspendCancellableCoroutine { cont ->
        val future = ProcessCameraProvider.getInstance(context)
        future.addListener({ cont.resume(future.get()) }, ContextCompat.getMainExecutor(context))
    }

private val analysisResolution = ResolutionSelector.Builder()
    .setResolutionStrategy(
        ResolutionStrategy(Size(640, 480), ResolutionStrategy.FALLBACK_RULE_CLOSEST_HIGHER_THEN_LOWER)
    )
    .build()

private fun loadMeme(context: Context, gesture: Int): ImageBitmap? {
    if (NativeCore.isVideo(gesture)) return null
    val file = NativeCore.memeFiles(gesture).randomOrNull() ?: return null
    return context.assets.open(file).use { BitmapFactory.decodeStream(it) }?.asImageBitmap()
}

@Composable
fun MiaucamScreen() {
    val context = LocalContext.current
    var lensFacing by remember { mutableStateOf(CameraSelector.LENS_FACING_FRONT) }
    val previewView = remember { PreviewView(context) }
    val engine = remember {
        runCatching { GestureEngine(context) }
            .onFailure { Log.w("miaucam", "gesture engine unavailable: $it") }
            .getOrNull()
    }
    val analysisExecutor = remember { Executors.newSingleThreadExecutor() }
    var gesture by remember { mutableIntStateOf(0) }
    var meme by remember { mutableStateOf<ImageBitmap?>(null) }

    DisposableEffect(Unit) {
        onDispose {
            analysisExecutor.shutdown()
            engine?.close()
        }
    }

    LaunchedEffect(gesture) {
        meme = withContext(Dispatchers.IO) { loadMeme(context, gesture) } ?: meme
    }

    LaunchedEffect(lensFacing) {
        val cameraProvider = awaitCameraProvider(context)
        val preview = Preview.Builder().setResolutionSelector(analysisResolution).build().also {
            it.setSurfaceProvider(previewView.surfaceProvider)
        }

        // Not every device (or emulator) has both a front and back camera -
        // fall back to whichever exists instead of crashing on the missing one.
        val requested = CameraSelector.Builder().requireLensFacing(lensFacing).build()
        val fallback = CameraSelector.Builder().requireLensFacing(
            if (lensFacing == CameraSelector.LENS_FACING_FRONT) CameraSelector.LENS_FACING_BACK
            else CameraSelector.LENS_FACING_FRONT
        ).build()
        val cameraSelector = when {
            cameraProvider.hasCamera(requested) -> requested
            cameraProvider.hasCamera(fallback) -> fallback
            else -> null
        }

        val analysis = ImageAnalysis.Builder()
            .setResolutionSelector(analysisResolution)
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
            .build()
        val mirror = cameraSelector === requested && lensFacing == CameraSelector.LENS_FACING_FRONT ||
            cameraSelector === fallback && lensFacing == CameraSelector.LENS_FACING_BACK
        if (engine != null) {
            analysis.setAnalyzer(analysisExecutor) { image ->
                try {
                    gesture = engine.process(image, mirror)
                } finally {
                    image.close()
                }
            }
        }

        cameraProvider.unbindAll()
        if (cameraSelector != null) {
            cameraProvider.bindToLifecycle(context as LifecycleOwner, cameraSelector, preview, analysis)
        }
    }

    Box(Modifier.fillMaxSize()) {
        val shown = meme
        if (shown != null) {
            Image(
                bitmap = shown,
                contentDescription = "meme",
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop,
            )
        } else {
            Image(
                painter = painterResource(id = R.drawable.pokercat),
                contentDescription = "meme",
                modifier = Modifier.fillMaxSize(),
                contentScale = ContentScale.Crop,
            )
        }

        // Small round camera preview.
        AndroidView(
            factory = { previewView },
            modifier = Modifier
                .align(Alignment.TopEnd)
                .padding(24.dp)
                .size(120.dp)
                .clip(CircleShape),
        )

        // Front/back camera switch.
        Button(
            onClick = {
                lensFacing = if (lensFacing == CameraSelector.LENS_FACING_FRONT) {
                    CameraSelector.LENS_FACING_BACK
                } else {
                    CameraSelector.LENS_FACING_FRONT
                }
            },
            modifier = Modifier.align(Alignment.BottomCenter).padding(32.dp),
        ) {
            Text("Switch camera")
        }
    }
}
