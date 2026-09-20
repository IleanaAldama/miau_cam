package com.miaucam.app

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Matrix
import android.os.SystemClock
import androidx.camera.core.ImageProxy
import com.google.mediapipe.framework.image.BitmapImageBuilder
import com.google.mediapipe.tasks.core.BaseOptions
import com.google.mediapipe.tasks.vision.core.RunningMode
import com.google.mediapipe.tasks.vision.facelandmarker.FaceLandmarker
import com.google.mediapipe.tasks.vision.handlandmarker.HandLandmarker
import java.io.Closeable
import java.nio.ByteBuffer
import kotlin.math.abs

class GestureEngine(context: Context) : Closeable {
    private val hand = HandLandmarker.createFromOptions(
        context,
        HandLandmarker.HandLandmarkerOptions.builder()
            .setBaseOptions(BaseOptions.builder().setModelAssetPath("hand_landmarker.task").build())
            .setRunningMode(RunningMode.VIDEO)
            .setNumHands(2)
            .build(),
    )

    private val face = FaceLandmarker.createFromOptions(
        context,
        FaceLandmarker.FaceLandmarkerOptions.builder()
            .setBaseOptions(BaseOptions.builder().setModelAssetPath("face_landmarker.task").build())
            .setRunningMode(RunningMode.VIDEO)
            .setNumFaces(1)
            .setOutputFaceBlendshapes(true)
            .setOutputFacialTransformationMatrixes(true)
            .build(),
    )

    private var lastTimestampMs = 0L
    private var pixels: ByteBuffer? = null

    init {
        NativeCore.reset()
    }

    fun process(image: ImageProxy, mirror: Boolean): Int {
        val bitmap = upright(image, mirror)
        val timestampMs = maxOf(SystemClock.uptimeMillis(), lastTimestampMs + 1)
        lastTimestampMs = timestampMs

        val mpImage = BitmapImageBuilder(bitmap).build()
        val hands = hand.detectForVideo(mpImage, timestampMs)
        val faces = face.detectForVideo(mpImage, timestampMs)

        val handFloats = hands.landmarks().flatMap { one -> one.flatMap { listOf(it.x(), it.y(), it.z()) } }
        val faceFloats = faces.faceLandmarks().firstOrNull()
            ?.take(FACE_LANDMARK_COUNT)
            ?.flatMap { listOf(it.x(), it.y(), it.z()) }
            .orEmpty()
        val blendshapes = faces.faceBlendshapes().orElse(emptyList()).firstOrNull().orEmpty()
        val transform = faces.facialTransformationMatrixes().orElse(emptyList()).firstOrNull()

        return NativeCore.advance(
            rgbaBuffer(bitmap),
            bitmap.width,
            bitmap.height,
            timestampMs.toDouble(),
            handFloats.toFloatArray(),
            faceFloats.toFloatArray(),
            blendshapes.map { it.categoryName() }.toTypedArray(),
            blendshapes.map { it.score() }.toFloatArray(),
            transform?.let(::rowMajor) ?: FloatArray(0),
        )
    }

    private fun upright(image: ImageProxy, mirror: Boolean): Bitmap {
        val matrix = Matrix().apply {
            postRotate(image.imageInfo.rotationDegrees.toFloat())
            if (mirror) postScale(-1f, 1f)
        }
        val source = image.toBitmap()
        return Bitmap.createBitmap(source, 0, 0, source.width, source.height, matrix, true)
    }

    private fun rgbaBuffer(bitmap: Bitmap): ByteBuffer {
        val needed = bitmap.width * bitmap.height * 4
        val buffer = pixels?.takeIf { it.capacity() == needed } ?: ByteBuffer.allocateDirect(needed)
        pixels = buffer
        buffer.rewind()
        bitmap.copyPixelsToBuffer(buffer)
        buffer.rewind()
        return buffer
    }

    // A pose matrix has its translation in one edge and zeros on the other.
    private fun rowMajor(m: FloatArray): FloatArray {
        val rightColumn = abs(m[3]) + abs(m[7]) + abs(m[11])
        val bottomRow = abs(m[12]) + abs(m[13]) + abs(m[14])
        return if (rightColumn >= bottomRow) m else FloatArray(16) { m[(it % 4) * 4 + it / 4] }
    }

    override fun close() {
        hand.close()
        face.close()
    }

    private companion object {
        const val FACE_LANDMARK_COUNT = 468
    }
}
