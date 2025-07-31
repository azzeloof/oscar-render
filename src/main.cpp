#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <deque>
#include <numbers>
#include <thread>
#include <stdexcept>

#include "include/oscilloscope.hpp"
#include "include/osc.hpp"
#include "RtAudio.h"

const std::size_t nScopes = 4;
std::array<Oscilloscope, nScopes> scopes;
std::vector<int16_t> audioBuffer;
// The global mutex was removed, as locking is handled inside each Oscilloscope instance.

// Audio callback function for RtAudio
int audioCallback(void* /*outputBuffer*/, void* inputBuffer, unsigned int nFrames,
    double /*streamTime*/, RtAudioStreamStatus status, void* /*userData*/) {
    if (status) {
        std::cerr << "Stream overflow detected!" << std::endl;
    }

    // The lock_guard was removed to prevent blocking the high-priority audio thread.
    const int16_t* input = (const int16_t*)inputBuffer;

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


int main() {
    asio::io_context io_context;
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

    // --- RtAudio Setup using JACK Backend ---
    RtAudio audio(RtAudio::UNIX_JACK);
    if (audio.getDeviceCount() < 1) {
        std::cerr << "Error: No audio devices found by the JACK backend.\n"
                  << "Please ensure the PipeWire-JACK compatibility layer is running." << std::endl;
        return -1;
    }
    
    RtAudio::StreamParameters params;
    params.deviceId = audio.getDefaultInputDevice();
    params.nChannels = 8;
    params.firstChannel = 0;
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 256;

    // --- FIX: Add StreamOptions to prevent auto-connecting and set a custom name ---
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_JACK_DONT_CONNECT;
    options.streamName = "OSCAR Renderer";

    try {
        audio.openStream(NULL, &params, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioCallback, NULL, &options);
        audio.startStream();
        std::cout << "Successfully opened 8-channel JACK input stream." << std::endl;
        std::cout << "Application should be visible in qpwgraph as '" << options.streamName << "'." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error opening audio stream: " << e.what() << std::endl;
        return -1;
    }
    
    // --- SFML 3 API Setup ---
    sf::ContextSettings ctx;
    sf::RenderWindow window(sf::VideoMode({width, height}), "OSCAR", sf::State::Windowed, ctx);
    window.setFramerateLimit(60);
    for (unsigned int i=0; i<nScopes; i++) {
        scopes[i].updateView(window.getSize());
    }

    sf::RenderTexture traceTexture({width, height});
    sf::RenderTexture compositeTexture({width, height});
    sf::RenderTexture blurTexture({width, height});
    sf::RenderTexture frameTexture({width, height});

    sf::Shader gaussianBlurShader;
    if (!gaussianBlurShader.loadFromFile("blur.frag", sf::Shader::Type::Fragment)) {
        std::cerr << "Error: Could not load blur.frag shader." << std::endl;
        return -1;
    }
    gaussianBlurShader.setUniform("texture", sf::Shader::CurrentTexture);

    while (window.isOpen()) {
        // SFML 3 Event Loop
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::Vector2u sizeVec = {resized->size.x, resized->size.y};
                sf::FloatRect viewRect({0.f, 0.f}, {static_cast<float>(sizeVec.x), static_cast<float>(sizeVec.y)});
                width = sizeVec.x;
                height = sizeVec.y;
                window.setView(sf::View(viewRect));
                traceTexture = sf::RenderTexture(sizeVec);
                blurTexture = sf::RenderTexture(sizeVec);
                frameTexture = sf::RenderTexture(sizeVec);
                compositeTexture = sf::RenderTexture(sizeVec);
                for (unsigned int i=0; i<nScopes; i++) {
                    scopes[i].updateView(sizeVec);
                }
            }
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

        window.clear(sf::Color::Transparent);

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

            window.draw(sf::Sprite(frameTexture.getTexture()));
        }
        window.display();
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
