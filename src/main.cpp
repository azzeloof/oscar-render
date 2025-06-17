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

#include "include/oscilloscope.hpp"
#include "include/osc.hpp"

int main() {
    asio::io_context io_context;
    OSCListener osc_listener_handler;

    std::unique_ptr<AsioOscReceiver> osc_receiver;
    try {
        osc_receiver = std::make_unique<AsioOscReceiver>(io_context, osc_listener_handler);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OSC receiver: " << e.what() << std::endl;
        return -1; // Or handle error appropriately
    }

    // Create and run the Asio I/O context in a separate thread
    std::thread asio_thread([&io_context]() {
        try {
            // Add a work guard to keep io_context::run() active
            // as long as there are asynchronous operations (like our listener).
            asio::executor_work_guard<asio::io_context::executor_type> work_guard = asio::make_work_guard(io_context);
            io_context.run();
        } catch (const std::exception& e) {
            std::cerr << "Asio thread exception: " << e.what() << std::endl;
        }
    });

    unsigned int width = 800;
    unsigned int height = 600;
    // Device Selection
    auto availableDevices = sf::SoundRecorder::getAvailableDevices();
    
    if (availableDevices.empty()) {
        std::cerr << "Error: No audio devices available." << std::endl;
        return -1;
    }

    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < availableDevices.size(); ++i) { std::cout << i << ": " << availableDevices[i] << std::endl; }
    std::cout << "Enter the number of the device to use: ";
    int deviceIndex;
    std::cin >> deviceIndex;
    if (deviceIndex < 0 || static_cast<size_t>(deviceIndex) >= availableDevices.size()) { /* ... */ return -1; }
    const std::string& selectedDevice = availableDevices[deviceIndex];
    
    // Oscilloscope and Window Setup
    //Oscilloscope oscilloscope;
    const std::size_t nScopes = 4;
    std::array<Oscilloscope, nScopes> scopes;
    for (int i=0; i<nScopes; i++) {
        scopes[i].setChannelCount(2);
        if (!scopes[i].startRecording(selectedDevice)) { /* ... */ return -1; }
    }
    std::cout << "Listening to device: " << selectedDevice << std::endl;
    sf::ContextSettings ctx;
    //ctx.antiAliasingLevel = 16;
    sf::RenderWindow window(sf::VideoMode({width, height}), "OSCAR", sf::State::Windowed, ctx);
    window.setFramerateLimit(60);
    for (int i=0; i<nScopes; i++) {
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
    gaussianBlurShader.setUniform("texture", sf::Shader::CurrentTexture); // This will be the input texture to the shader

    // Main Application Loop
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (event->is<sf::Event::Resized>()) {
                sf::Vector2u sizeVec = window.getSize();
                sf::FloatRect viewRect({0.f, 0.f}, {static_cast<float>(sizeVec.x), static_cast<float>(sizeVec.y)});
                width = sizeVec.x;
                height = sizeVec.y;
                window.setView(sf::View(viewRect));
                traceTexture = sf::RenderTexture(sizeVec);
                blurTexture = sf::RenderTexture(sizeVec);
                frameTexture = sf::RenderTexture(sizeVec);
                compositeTexture = sf::RenderTexture(sizeVec);
                for (int i=0; i<nScopes; i++) {
                    scopes[i].updateView(sizeVec);
                }
            }
        }
        int scope_index = osc_listener_handler.getIndex();

        if (auto val_opt = osc_listener_handler.getPendingTraceThickness()) {
            // val_opt is a std::optional<unsigned int>.
            // Check if it contains a value (it will if an update was queued).
            if (val_opt) {
                scopes[scope_index].setTraceThickness(*val_opt); // Use *val_opt to get the value
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

        for (int i=0; i<nScopes; i++) {

            traceTexture.clear(sf::Color::Transparent);
            traceTexture.draw(scopes[i]);
        
            traceTexture.display();

            // Gaussian Blur Pass 1: Horizontal
            gaussianBlurShader.setUniform("texture", compositeTexture.getTexture());
            gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(traceTexture.getSize()));
            gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
            gaussianBlurShader.setUniform("blur_spread_px", scopes[i].getBlurSpread());
            
            blurTexture.clear(sf::Color::Transparent);
            blurTexture.draw(sf::Sprite(traceTexture.getTexture()), &gaussianBlurShader);
            blurTexture.display();

            // Gaussian Blur Pass 2: Vertical
            gaussianBlurShader.setUniform("texture", blurTexture.getTexture());
            gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

            // Composite and Display
            frameTexture.clear(sf::Color::Transparent);
            frameTexture.draw(sf::Sprite(blurTexture.getTexture()), &gaussianBlurShader);
            frameTexture.display();

            window.draw(sf::Sprite(frameTexture.getTexture()));
        }
        window.display();
    }

    std::cout << "Stopping OSC receiver and Asio context..." << std::endl;
    if (osc_receiver) {
        osc_receiver->stop(); // Request the OSC receiver to stop and close its socket
    }
    io_context.stop();    // Stop the io_context event loop (this will cause io_context.run() to return)

    // Wait for the Asio thread to finish its work
    if (asio_thread.joinable()) {
        asio_thread.join();
    }
    std::cout << "Application finished." << std::endl;
    
    // scopes[0].stop(); // sf::SoundRecorder::stop(), might be useful if you want to explicitly stop mic.

    return 0;
}