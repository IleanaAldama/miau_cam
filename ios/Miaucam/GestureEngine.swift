import AVFoundation
import MediaPipeTasksVision

enum EngineError: Error {
    case missingModel(String)
}

final class GestureEngine {
    private let hand: HandLandmarker
    private let face: FaceLandmarker
    private var lastTimestampMs = 0

    init() throws {
        let handOptions = HandLandmarkerOptions()
        handOptions.baseOptions.modelAssetPath = try Self.modelPath("hand_landmarker.task")
        handOptions.runningMode = .video
        handOptions.numHands = 2
        hand = try HandLandmarker(options: handOptions)

        let faceOptions = FaceLandmarkerOptions()
        faceOptions.baseOptions.modelAssetPath = try Self.modelPath("face_landmarker.task")
        faceOptions.runningMode = .video
        faceOptions.numFaces = 1
        faceOptions.outputFaceBlendshapes = true
        faceOptions.outputFacialTransformationMatrixes = true
        face = try FaceLandmarker(options: faceOptions)

        MiaucamCore.reset()
    }

    func process(_ buffer: CMSampleBuffer) -> Int? {
        guard let pixels = CMSampleBufferGetImageBuffer(buffer) else { return nil }

        let timestampMs = max(Int(ProcessInfo.processInfo.systemUptime * 1000), lastTimestampMs + 1)
        lastTimestampMs = timestampMs

        guard let image = try? MPImage(sampleBuffer: buffer, orientation: .up),
              let hands = try? hand.detect(videoFrame: image, timestampInMilliseconds: timestampMs),
              let faces = try? face.detect(videoFrame: image, timestampInMilliseconds: timestampMs)
        else { return nil }

        let handFloats = hands.landmarks.flatMap { $0.flatMap { [$0.x, $0.y, $0.z] } }
        let faceFloats = (faces.faceLandmarks.first ?? []).prefix(468).flatMap { [$0.x, $0.y, $0.z] }
        let shapes = faces.faceBlendshapes.first?.categories ?? []

        CVPixelBufferLockBaseAddress(pixels, .readOnly)
        defer { CVPixelBufferUnlockBaseAddress(pixels, .readOnly) }
        guard let base = CVPixelBufferGetBaseAddress(pixels) else { return nil }

        return Int(MiaucamCore.advance(
            withBGRA: base.assumingMemoryBound(to: UInt8.self),
            width: Int32(CVPixelBufferGetWidth(pixels)),
            height: Int32(CVPixelBufferGetHeight(pixels)),
            stride: Int32(CVPixelBufferGetBytesPerRow(pixels)),
            timestampMs: Double(timestampMs),
            hands: Self.data(handFloats),
            face: Self.data(Array(faceFloats)),
            blendshapeNames: shapes.map { $0.categoryName ?? "" },
            blendshapeScores: Self.data(shapes.map { $0.score }),
            transform: Data()
        ))
    }

    private static func data(_ floats: [Float]) -> Data {
        floats.withUnsafeBufferPointer { Data(buffer: $0) }
    }

    private static func modelPath(_ name: String) throws -> String {
        guard let path = Bundle.main.path(forResource: name, ofType: nil, inDirectory: "models") else {
            throw EngineError.missingModel(name)
        }
        return path
    }
}
