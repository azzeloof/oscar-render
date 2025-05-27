#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>   // Required for std::uint8_t
#include <algorithm> // For std::min/max

/**
 * @class Oscilloscope
 * @brief Captures and visualizes stereo audio data in real-time.
 */
class Oscilloscope : public sf::SoundRecorder, public sf::Drawable {
public:
    Oscilloscope() {
        m_availableDevices = getAvailableDevices();
    }

    void updateView(const sf::Vector2u& newSize) {
        m_center.x = static_cast<float>(newSize.x) / 2.f;
        m_center.y = static_cast<float>(newSize.y) / 2.f;
        m_radius = std::min(static_cast<float>(newSize.x), static_cast<float>(newSize.y)) / 2.5f;
    }

    bool startRecording(const std::string& deviceName) {
        if (!setDevice(deviceName)) {
            return false;
        }
        return start();
    }
    
    using sf::SoundRecorder::setChannelCount;

private:
    virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_vertices.clear();
        m_vertices.setPrimitiveType(sf::PrimitiveType::LineStrip);

        for (std::size_t i = 0; i < sampleCount; i += 2) {
            float x_sample = samples[i] / 32768.f;
            float y_sample = (i + 1 < sampleCount) ? samples[i + 1] / 32768.f : 0.f;

            float x_pos = m_center.x + x_sample * m_radius;
            float y_pos = m_center.y + y_sample * m_radius;
            
            sf::Vertex vertex;
            vertex.position = {x_pos, y_pos};
            vertex.color = sf::Color::Green;
            m_vertices.append(vertex);
        }
        return true;
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        target.draw(m_vertices, states);
    }

    sf::Vector2f m_center;
    float m_radius = 250.f;
    sf::VertexArray m_vertices;
    mutable std::mutex m_mutex;
    std::vector<std::string> m_availableDevices;
};


int main() {
    // ---- 1. Device Selection ----
    auto availableDevices = sf::SoundRecorder::getAvailableDevices();
    if (availableDevices.empty()) { /* ... error handling ... */ return -1; }
    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < availableDevices.size(); ++i) {
        std::cout << i << ": " << availableDevices[i] << std::endl;
    }
    std::cout << "Enter the number of the device to use: ";
    int deviceIndex;
    std::cin >> deviceIndex;
    if (deviceIndex < 0 || static_cast<size_t>(deviceIndex) >= availableDevices.size()) { /* ... error handling ... */ return -1; }
    const std::string& selectedDevice = availableDevices[deviceIndex];
    
    // ---- 2. Oscilloscope and Window Setup ----
    Oscilloscope oscilloscope;
    oscilloscope.setChannelCount(2);
    if (!oscilloscope.startRecording(selectedDevice)) { /* ... error handling ... */ return -1; }
    std::cout << "Listening to device: " << selectedDevice << std::endl;
    
    sf::RenderWindow window(sf::VideoMode({800, 600}), "C++ Virtual Oscilloscope");
    window.setFramerateLimit(60);
    oscilloscope.updateView(window.getSize());

    // ---- 3. Graphics Setup for Persistence Effect ----
    sf::RenderTexture persistenceTexture({800, 600});
    persistenceTexture.clear(sf::Color::Black);
    sf::RectangleShape fadeRect;
    fadeRect.setSize(sf::Vector2f(window.getSize()));
    int fadeAlpha = 10;
    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << "Persistence Alpha: " << fadeAlpha << " (Use +/- keys to change)" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;

    // ---- 4. Main Application Loop ----
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // --- FIXED: Resized Event Handling ---
            if (event->is<sf::Event::Resized>()) {
                // Get the size directly from the window, which is more robust
                sf::Vector2u sizeVec = window.getSize();

                // Use the SFML 3 constructor style for sf::FloatRect
                sf::FloatRect viewRect({0.f, 0.f}, {static_cast<float>(sizeVec.x), static_cast<float>(sizeVec.y)});
                window.setView(sf::View(viewRect));
                
                // Use the constructor/move-assignment we know works
                persistenceTexture = sf::RenderTexture(sizeVec);
                
                fadeRect.setSize(sf::Vector2f(sizeVec));
                oscilloscope.updateView(sizeVec);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Equal || keyPressed->code == sf::Keyboard::Key::Add) {
                    fadeAlpha = std::min(fadeAlpha + 1, 255);
                    std::cout << "Persistence Alpha: " << fadeAlpha << std::endl;
                } else if (keyPressed->code == sf::Keyboard::Key::Hyphen || keyPressed->code == sf::Keyboard::Key::Subtract) {
                    fadeAlpha = std::max(fadeAlpha - 1, 0);
                    std::cout << "Persistence Alpha: " << fadeAlpha << std::endl;
                }
            }
        }

        // ---- 5. Drawing Logic ----
        // --- FIXED: Use standard C++ type std::uint8_t instead of sf::Uint8 ---
        fadeRect.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(fadeAlpha)));

        persistenceTexture.draw(fadeRect);
        persistenceTexture.draw(oscilloscope);
        persistenceTexture.display();

        window.clear();
        window.draw(sf::Sprite(persistenceTexture.getTexture()));
        window.display();
    }

    return 0;
}