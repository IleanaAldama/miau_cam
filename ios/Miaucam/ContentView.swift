import SwiftUI

struct ContentView: View {
    @StateObject private var model = MiaucamModel()

    var body: some View {
        ZStack {
            Color.black.ignoresSafeArea()

            if let url = model.videoURL {
                MemeVideo(url: url).ignoresSafeArea()
            } else if let meme = model.meme {
                Image(uiImage: meme)
                    .resizable()
                    .scaledToFill()
                    .ignoresSafeArea()
            }

            VStack {
                HStack {
                    Spacer()
                    CameraPreview(session: model.camera.session)
                        .frame(width: 120, height: 120)
                        .clipShape(Circle())
                        .padding(24)
                }
                Spacer()
                Button("Switch camera") { model.switchCamera() }
                    .buttonStyle(.borderedProminent)
                    .padding(32)
            }
        }
        .onAppear { model.start() }
    }
}
