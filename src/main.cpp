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
#include "include/helpers.hpp"

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
    Oscilloscope oscilloscope;
    oscilloscope.setChannelCount(2);
    if (!oscilloscope.startRecording(selectedDevice)) { /* ... */ return -1; }
    std::cout << "Listening to device: " << selectedDevice << std::endl;
    sf::RenderWindow window(sf::VideoMode({width, height}), "C++ Virtual Oscilloscope");
    window.setFramerateLimit(60);
    oscilloscope.updateView(window.getSize());

    sf::RenderTexture traceTexture({width, height});
    sf::RenderTexture compositeTexture({width, height});
    sf::RenderTexture blurTexture({width, height});
    sf::RenderTexture frameTexture({width, height});

    std::deque<sf::Texture> persistentFrames;

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
                persistentFrames.clear();
                oscilloscope.updateView(sizeVec);
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingLayerCount()) {
            // val_opt is a std::optional<unsigned int>.
            // Check if it contains a value (it will if an update was queued).
            if (val_opt) {
                oscilloscope.setLayerCount(*val_opt); // Use *val_opt to get the value
                std::cout << "Main: Applied Layers set to: " << oscilloscope.getLayerCount() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingPersistenceFrames()) {
            if (val_opt) {
                oscilloscope.setPersistenceFrames(*val_opt);
                persistentFrames.clear();
                std::cout << "Main: Applied Persistence Frames set to: " << oscilloscope.getPersistenceFrames() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingPersistenceStrength()) {
            if (val_opt) {
                oscilloscope.setPersistenceStrength(*val_opt);
                std::cout << "Main: Applied Persistence Strength set to: " << oscilloscope.getPersistenceStrength() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingBlurSpread()) {
            if (val_opt) {
                oscilloscope.setBlurSpread(*val_opt);
                std::cout << "Main: Applied Gaussian Blur Spread set to: " << oscilloscope.getBlurSpread() << std::endl;
            }
        }

        if (auto val_opt = osc_listener_handler.getPendingScale()) {
            if (val_opt) {
                oscilloscope.setScale(*val_opt);
                std::cout << "Main: Applied Scale set to: " << oscilloscope.getScale() << std::endl;
            }
        }

        traceTexture.clear(sf::Color::Transparent);
        traceTexture.draw(oscilloscope);
        traceTexture.display();

        if (persistentFrames.size() >= oscilloscope.getPersistenceFrames()) {
            persistentFrames.pop_back(); 
        }
        persistentFrames.push_front(traceTexture.getTexture()); // Add a copy

        compositeTexture.clear(sf::Color::Black);

        for (size_t i = 0; i < persistentFrames.size(); ++i) {
        //for (size_t i = persistentFrames.size(); i > 0; --i) {
            sf::Sprite frameSprite(persistentFrames[i]);
            
            float alphaRatio = 1.0f;
            if (persistentFrames.size() > 1) { // Avoid division by zero if only 1 frame
                 alphaRatio = 1.0f - (static_cast<float>(i) / (persistentFrames.size() -1) );
            }
            std::uint8_t alpha = 0;
            if (i == 0) { 
                alpha = 255;
            } else {
                float fadeFactor = static_cast<float>(oscilloscope.getPersistenceStrength()) / 255.f; // How much the oldest frame retains opacity
                alpha = static_cast<std::uint8_t>(255.f * ( ( (oscilloscope.getPersistenceFrames() - 1.f - i) / (oscilloscope.getPersistenceFrames() -1.f) ) * (1.f - fadeFactor) + fadeFactor ) );
                if (i == oscilloscope.getPersistenceFrames() -1) alpha = static_cast<std::uint8_t>(255.f * fadeFactor); // Oldest frame

                alpha = static_cast<std::uint8_t>(255.f * (1.f - static_cast<float>(i) / static_cast<float>(oscilloscope.getPersistenceFrames())));
            }

            sf::Color oc = frameSprite.getColor();
            frameSprite.setColor(sf::Color(oc.r, oc.g, oc.b, alpha));
            compositeTexture.draw(frameSprite, sf::BlendAlpha);
        }

                // Gaussian Blur Pass 1: Horizontal
        gaussianBlurShader.setUniform("texture", compositeTexture.getTexture());
        gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(traceTexture.getSize()));
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
        gaussianBlurShader.setUniform("blur_spread_px", oscilloscope.getBlurSpread());
        
        blurTexture.clear(sf::Color::Transparent);
        blurTexture.draw(sf::Sprite(compositeTexture.getTexture()), &gaussianBlurShader);
        blurTexture.display();

        // Gaussian Blur Pass 2: Vertical
        gaussianBlurShader.setUniform("texture", blurTexture.getTexture());
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

        // Composite and Display
        frameTexture.clear(sf::Color::Transparent);
        frameTexture.draw(sf::Sprite(blurTexture.getTexture()), &gaussianBlurShader);
        frameTexture.display();

        window.clear(sf::Color::Black);
        window.draw(sf::Sprite(frameTexture.getTexture()));

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
    
    // oscilloscope.stop(); // sf::SoundRecorder::stop(), might be useful if you want to explicitly stop mic.

    return 0;
}