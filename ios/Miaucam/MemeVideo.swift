import AVFoundation
import SwiftUI

struct MemeVideo: UIViewRepresentable {
    let url: URL

    func makeUIView(context: Context) -> LoopingPlayerView {
        let view = LoopingPlayerView()
        view.play(url)
        return view
    }

    func updateUIView(_ view: LoopingPlayerView, context: Context) {
        if view.url != url { view.play(url) }
    }

    final class LoopingPlayerView: UIView {
        private let player = AVQueuePlayer()
        private var looper: AVPlayerLooper?
        private(set) var url: URL?

        override class var layerClass: AnyClass { AVPlayerLayer.self }
        private var playerLayer: AVPlayerLayer { layer as! AVPlayerLayer }

        func play(_ url: URL) {
            self.url = url
            playerLayer.player = player
            playerLayer.videoGravity = .resizeAspect
            player.isMuted = true
            looper = AVPlayerLooper(player: player, templateItem: AVPlayerItem(url: url))
            player.play()
        }
    }
}
