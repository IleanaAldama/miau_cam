package com.miaucam.app

import java.nio.ByteBuffer

object NativeCore {
    init {
        System.loadLibrary("miaucam_native")
    }

    external fun reset()

    external fun advance(
        rgba: ByteBuffer,
        width: Int,
        height: Int,
        timestampMs: Double,
        hands: FloatArray,
        face: FloatArray,
        blendshapeNames: Array<String>,
        blendshapeScores: FloatArray,
        transform: FloatArray,
    ): Int

    external fun memeFiles(gesture: Int): Array<String>
    external fun isVideo(gesture: Int): Boolean
}
