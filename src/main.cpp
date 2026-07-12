#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include <thread>

#include "include/blur_frag.hpp"
#include "include/oscilloscope.hpp"
#include "include/osc.hpp"
#include "RtAudio.h"

#if defined(__unix__) || defined(__APPLE__)
#include <jack/jack.h>
#endif

constexpr size_t nScopes = 4;
std::array<Oscilloscope, nScopes> scopes;
std::vector<int16_t> audioBuffer;

// Audio callback function for RtAudio
int audioCallback(void* /*outputBuffer*/, const void* inputBuffer, const unsigned int nFrames,
    double /*streamTime*/, RtAudioStreamStatus status, void* /*userData*/) {
    if (status) {
        std::cerr << "Stream overflow detected!" << std::endl;
    }

    const auto* input = (const int16_t*)inputBuffer;

    for (unsigned int i = 0; i < nScopes; ++i) {
        // This temporary buffer is fine, as it's local to the audio thread.
        std::vector<int16_t> scopeAudioBuffer;
        scopeAudioBuffer.reserve(nFrames * 2);
        for (unsigned int j = 0; j < nFrames; ++j) {
            scopeAudioBuffer.push_back(input[j * 8 + i * 2]);
            scopeAudioBuffer.push_back(input[j * 8 + i * 2 + 1]);
        }
        // Each scope's processSamples method handles its own thread safety.
        scopes[i].processSamples(scopeAudioBuffer.data(), scopeAudioBuffer.size());
    }

    return 0;
}


int main(int argc, char** argv) {
    float framerate = 60.f;
    bool headless = false;

    for (int i=1; i<argc; ++i) {
        if (std::string(argv[i]) == "--headless") {
            headless = true;
        }
    }

    asio::io_context io_context;

    // Setup TCP Video Feed Socket
    asio::ip::tcp::socket tcp_socket(io_context);
    bool tcp_connected = false;
    try {
        asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 5557);
        tcp_socket.connect(endpoint);
        tcp_connected = true;
        std::cout << "Connected to IDE video feed on port 5557." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Could not connect to IDE video feed (is the IDE running?)" << std::endl;
    }

    OSCListener osc_listener_handler;

    std::unique_ptr<AsioOscReceiver> osc_receiver;
    try {
        osc_receiver = std::make_unique<AsioOscReceiver>(io_context, osc_listener_handler);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OSC receiver: " << e.what() << std::endl;
        return -1;
    }

    std::thread asio_thread([&io_context]() {
        try {
            asio::executor_work_guard<asio::io_context::executor_type> work_guard = asio::make_work_guard(io_context);
            io_context.run();
        } catch (const std::exception& e) {
            std::cerr << "Asio thread exception: " << e.what() << std::endl;
        }
    });

    unsigned int width = 800;
    unsigned int height = 600;

    RtAudio::StreamParameters params;
#ifdef __APPLE__
    // --- RtAudio Setup for macOS using CoreAudio ---
    RtAudio audio;
    if (audio.getDeviceCount() < 1) {
        std::cerr << "Error: No audio devices found." << std::endl;
        return -1;
    }

    // Find the BlackHole audio device
    unsigned int blackHoleDeviceId = 0;
    std::vector<unsigned int> deviceIds = audio.getDeviceIds();
    for (unsigned int i : deviceIds) {
        try {
            RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
            std::cout << "Found device: " << info.name << " with ID: " << i << std::endl;
            if (std::string(info.name).find("BlackHole") != std::string::npos) {
                blackHoleDeviceId = i;
                break;
            }
        } catch (const RtAudioErrorType& e) {
            std::cerr << "Error getting device info for device " << i << ": " << e << std::endl;
        }
    }

    if (blackHoleDeviceId == 0) {
        std::cerr << "Error: BlackHole audio device not found." << std::endl;
        std::cerr << "Please install BlackHole from https://github.com/ExistentialAudio/BlackHole" << std::endl;
        return -1;
    }

    params.deviceId = blackHoleDeviceId;
#else
    // --- RtAudio Setup using JACK Backend (for Linux) ---
    RtAudio audio(RtAudio::UNIX_JACK);
    if (audio.getDeviceCount() < 1) {
        std::cerr << "Error: No audio devices found by the JACK backend.\n"
                  << "Please ensure the PipeWire-JACK compatibility layer is running." << std::endl;
        return -1;
    }
    params.deviceId = audio.getDefaultInputDevice();
#endif


    params.nChannels = 8;
    params.firstChannel = 0;
    unsigned int bufferFrames = 256;

    // Query the default input device for its preferred sample rate
    RtAudio::DeviceInfo info = audio.getDeviceInfo(params.deviceId);
    unsigned int sampleRate = info.preferredSampleRate;
    if (sampleRate == 0) {
        std::cerr << "Warning: Could not determine preferred sample rate from JACK. Falling back to 44100." << std::endl;
        sampleRate = 44100; // Fallback
    }
    std::cout << "Using sample rate: " << sampleRate << std::endl;

#ifdef __APPLE__
    // For macOS, use default stream options
    RtAudio::StreamOptions options;
    try {
        audio.openStream(nullptr, &params, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioCallback, nullptr, &options);
        audio.startStream();
        std::cout << "Successfully opened CoreAudio input stream." << std::endl;
    }
#else
    // For Linux, use JACK-specific stream options
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_JACK_DONT_CONNECT;
    options.streamName = "OSCAR Renderer";

    try {
        audio.openStream(NULL, &params, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioCallback, NULL, &options);
        audio.startStream();
        std::cout << "Successfully opened JACK input stream." << std::endl;
        std::cout << "Application should be visible in qjackctl or qpwgraph as '" << options.streamName << "'." << std::endl;
    }
#endif
    catch (const std::exception& e) {
        std::cerr << "Error opening audio stream: " << e.what() << std::endl;
        return -1;
    }

    // --- SFML 3 API Setup ---
    sf::ContextSettings ctx;
    std::optional<sf::RenderWindow> window;
    if (!headless) {
        window.emplace(sf::VideoMode({width, height}), "OSCAR", sf::State::Windowed, ctx);
        window->setFramerateLimit(static_cast<int>(framerate));
    }

    for (unsigned int i=0; i<nScopes; i++) {
        sf::Vector2u viewSize = headless ? sf::Vector2u{width, height} : window->getSize();
        scopes[i].updateView(viewSize);
    }

    sf::RenderTexture traceTexture({width, height});
    sf::RenderTexture compositeTexture({width, height});
    sf::RenderTexture blurTexture({width, height});
    sf::RenderTexture frameTexture({width, height});
    sf::RenderTexture finalOutputTexture({width, height});

    // --- Fixed-size preview render textures for TCP streaming ---
    constexpr unsigned int previewSize = 600;
    sf::RenderTexture previewTraceTexture({previewSize, previewSize});
    sf::RenderTexture previewBlurTexture({previewSize, previewSize});
    sf::RenderTexture previewFrameTexture({previewSize, previewSize});
    sf::RenderTexture previewCompositeTexture({previewSize, previewSize});
    sf::RenderTexture previewFinalOutputTexture({previewSize, previewSize});

    sf::Shader gaussianBlurShader;
    if (!gaussianBlurShader.loadFromMemory(BLUR_FRAG_SRC, sf::Shader::Type::Fragment)) {
        std::cerr << "Error: Could not load blur.frag shader." << std::endl;
        return -1;
    }
    gaussianBlurShader.setUniform("texture", sf::Shader::CurrentTexture);

    sf::Clock frameClock;
    sf::Time frameTime = sf::seconds(1.f / framerate);

    while (window->isOpen()) {
        if (!headless && window) {
            // SFML 3 Event Loop
            while (const auto event = window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window->close();
                }

                if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    sf::Vector2u sizeVec = {resized->size.x, resized->size.y};
                    sf::FloatRect viewRect({0.f, 0.f}, {static_cast<float>(sizeVec.x), static_cast<float>(sizeVec.y)});
                    window->setView(sf::View(viewRect));
                    traceTexture = sf::RenderTexture(sizeVec);
                    blurTexture = sf::RenderTexture(sizeVec);
                    frameTexture = sf::RenderTexture(sizeVec);
                    compositeTexture = sf::RenderTexture(sizeVec);
                    finalOutputTexture = sf::RenderTexture(sizeVec);
                    /*for (unsigned int i=0; i<nScopes; i++) {
                        scopes[i].updateView(sizeVec);
                    }*/
                }
            }
        } else {
            // Manage framerate manually if we don't have window->setFramerateLimit
            sf::Time elapsed = frameClock.getElapsedTime();
            if (elapsed < frameTime) {
                sf::sleep(frameTime - elapsed);
            }
            frameClock.restart();
        }

        int scope_index = osc_listener_handler.getIndex();

        if (auto val_opt = osc_listener_handler.getPendingTraceThickness()) {
            if (val_opt) {
                scopes[scope_index].setTraceThickness(*val_opt);
                std::cout << "Main: Applied Layers set to: " << scopes[scope_index].getTraceThickness() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingTraceColor()) {
            if (val_opt) {
                uint8_t R = (*val_opt) >> 24;
                uint8_t G = (*val_opt) >> 16;
                uint8_t B = (*val_opt) >> 8;
                uint8_t A = (*val_opt);
                std::cout << R << G << B << A << '\n';
                scopes[scope_index].setTraceColor(sf::Color(R, G, B, A));
                std::cout << "Main: Applied Color Changed" << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingPersistenceSamples()) {
            if (val_opt) {
                scopes[scope_index].setPersistenceSamples(*val_opt);
                std::cout << "Main: Applied Persistence Frames set to: " << scopes[scope_index].getPersistenceSamples() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingPersistenceStrength()) {
            if (val_opt) {
                scopes[scope_index].setPersistenceStrength(*val_opt);
                std::cout << "Main: Applied Persistence Strength set to: " << scopes[scope_index].getPersistenceStrength() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingBlurSpread()) {
            if (val_opt) {
                scopes[scope_index].setBlurSpread(*val_opt);
                std::cout << "Main: Applied Gaussian Blur Spread set to: " << scopes[scope_index].getBlurSpread() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingAlphaScale()) {
            if (val_opt) {
                scopes[scope_index].setAlphaScale(*val_opt);
                std::cout << "Main: Applied Alpha Scale set to: " << scopes[scope_index].getAlphaScale() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingScale()) {
            if (val_opt) {
                scopes[scope_index].setScale(*val_opt);
                std::cout << "Main: Applied Scale set to: " << scopes[scope_index].getScale() << std::endl;
            }
        }

        window->clear(sf::Color::Transparent);

        finalOutputTexture.clear(sf::Color::Transparent);

        // --- Main window render pass (at window resolution) ---
        for (unsigned int i=0; i<nScopes; i++) {

        traceTexture.clear(sf::Color::Transparent);
        traceTexture.draw(scopes[i]);
    
        traceTexture.display();

        gaussianBlurShader.setUniform("texture", compositeTexture.getTexture());
        gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(traceTexture.getSize()));
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
        gaussianBlurShader.setUniform("blur_spread_px", scopes[i].getBlurSpread());
    
        blurTexture.clear(sf::Color::Transparent);
        blurTexture.draw(sf::Sprite(traceTexture.getTexture()), &gaussianBlurShader);
        blurTexture.display();

        gaussianBlurShader.setUniform("texture", blurTexture.getTexture());
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

        frameTexture.clear(sf::Color::Transparent);
        frameTexture.draw(sf::Sprite(blurTexture.getTexture()), &gaussianBlurShader);
        frameTexture.display();
        finalOutputTexture.draw(sf::Sprite(frameTexture.getTexture()));
    }
    finalOutputTexture.display();

    if (!headless) {
        window->clear(sf::Color::Transparent);
        window->draw(sf::Sprite(finalOutputTexture.getTexture()));
        window->display();
    }

    // --- TCP Video Streaming ---
    static int frameCounter = 0;
    frameCounter++;
    bool shouldSendFrame = tcp_connected && (frameCounter % 2 == 0);

    if (shouldSendFrame || (!tcp_connected && frameCounter % static_cast<int>(framerate) == 0)) {
        if (!tcp_connected) {
            // Auto-reconnect: Try to connect once per second
            try {
                tcp_socket.close();
                asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 5557);
                tcp_socket.connect(endpoint);
                tcp_connected = true;
                std::cout << "Connected to IDE video feed." << std::endl;
                shouldSendFrame = (frameCounter % 2 == 0);
            } catch (...) {
                // IDE not running yet, fail silently and try again later
            }
        }

        if (tcp_connected && shouldSendFrame) {
            // Render at fixed preview size — draw() auto-adapts to the target texture size
            previewFinalOutputTexture.clear(sf::Color::Transparent);

            for (unsigned int i = 0; i < nScopes; i++) {
                previewTraceTexture.clear(sf::Color::Transparent);
                previewTraceTexture.draw(scopes[i]);
                previewTraceTexture.display();

                gaussianBlurShader.setUniform("texture", previewCompositeTexture.getTexture());
                gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(previewTraceTexture.getSize()));
                gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
                gaussianBlurShader.setUniform("blur_spread_px", scopes[i].getBlurSpread());

                previewBlurTexture.clear(sf::Color::Transparent);
                previewBlurTexture.draw(sf::Sprite(previewTraceTexture.getTexture()), &gaussianBlurShader);
                previewBlurTexture.display();

                gaussianBlurShader.setUniform("texture", previewBlurTexture.getTexture());
                gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

                previewFrameTexture.clear(sf::Color::Transparent);
                previewFrameTexture.draw(sf::Sprite(previewBlurTexture.getTexture()), &gaussianBlurShader);
                previewFrameTexture.display();
                previewFinalOutputTexture.draw(sf::Sprite(previewFrameTexture.getTexture()));
            }
            previewFinalOutputTexture.display();

            sf::Image img = previewFinalOutputTexture.getTexture().copyToImage();
            std::optional<std::vector<uint8_t>> bufferOpt = img.saveToMemory("jpg");

            if (bufferOpt) {
                const std::vector<uint8_t>& buffer = *bufferOpt;
                uint32_t size = buffer.size();
                std::array<uint8_t, 4> header = {
                    static_cast<uint8_t>((size >> 24) & 0xFF),
                    static_cast<uint8_t>((size >> 16) & 0xFF),
                    static_cast<uint8_t>((size >> 8) & 0xFF),
                    static_cast<uint8_t>(size & 0xFF)
                };

                asio::error_code ec;
                asio::write(tcp_socket, asio::buffer(header), ec);
                if (!ec) {
                    asio::write(tcp_socket, asio::buffer(buffer), ec);
                }

                if (ec) {
                    std::cerr << "TCP send error, dropping feed: " << ec.message() << std::endl;
                    tcp_connected = false;
                }
            }
        }
    }
}
    std::cout << "Stopping OSC receiver and Asio context..." << std::endl;
    if (osc_receiver) {
        osc_receiver->stop();
    }
    io_context.stop();

    if (asio_thread.joinable()) {
        asio_thread.join();
    }

    if (audio.isStreamOpen()) {
        audio.stopStream();
        audio.closeStream();
    }
    
    std::cout << "Application finished." << std::endl;
    
    return 0;
}
