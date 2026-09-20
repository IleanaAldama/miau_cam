import AVFoundation
import SwiftUI

final class MiaucamModel: ObservableObject {
    @Published var meme: UIImage?
    @Published var videoURL: URL?
    @Published var position: AVCaptureDevice.Position = .front

    let camera = CameraController()
    private var engine: GestureEngine?
    private var gesture = -1

    func start() {
        engine = try? GestureEngine()
        camera.onFrame = { [weak self] buffer in self?.handle(buffer) }
        AVCaptureDevice.requestAccess(for: .video) { granted in
            if granted { self.camera.start(position: self.position) }
        }
    }

    func switchCamera() {
        position = position == .front ? .back : .front
        camera.start(position: position)
    }

    private func handle(_ buffer: CMSampleBuffer) {
        guard let next = engine?.process(buffer), next != gesture else { return }
        gesture = next
        DispatchQueue.main.async { self.show(gesture: next) }
    }

    private func show(gesture: Int) {
        let id = Int32(gesture)
        guard let file = MiaucamCore.memeFiles(forGesture: id).randomElement(),
              let url = Bundle.main.url(forResource: file, withExtension: nil, subdirectory: "memes")
        else { return }

        if MiaucamCore.isVideoGesture(id) {
            videoURL = url
        } else {
            videoURL = nil
            meme = UIImage(contentsOfFile: url.path) ?? meme
        }
    }
}
