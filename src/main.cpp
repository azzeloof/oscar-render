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

#ifndef M_PI
#define M_PI 3.14159265359
#endif

float dist(sf::Vertex A, sf::Vertex B) {
    return std::sqrt(std::pow((A.position.x - B.position.x),2) + std::pow((A.position.y - B.position.y),2));
}

float dist(int x1, int y1, int x2, int y2) {
    return std::sqrt(std::pow((x1 - x2), 2) + std::pow((y1 - y2), 2));
}

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
        m_radius = std::min(static_cast<float>(newSize.x), static_cast<float>(newSize.y)) / 1.5f;
    }

    bool startRecording(const std::string& deviceName) {
        if (!setDevice(deviceName)) {
            return false;
        }
        return start();
    }
    
    using sf::SoundRecorder::setChannelCount;

    void setThickness(int n) {
        n_layers = std::max(1, n);
    }
    float getThickness() const {
        return n_layers;
    }
    void setPersistenceFrames(unsigned int n) {
        maxPersistentFrames = n;
    }
    unsigned int getPersistenceFrames() const {
        return maxPersistentFrames;
    }
    void setPersistenceStrength(unsigned int n) {
        persistenceStrength = n;
    }
    unsigned int getPersistenceStrength() const {
        return persistenceStrength;
    }


private:
    virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_vertices.clear();
        m_vertices.setPrimitiveType(sf::PrimitiveType::LineStrip);
        //float x_pos = 0;
        //float y_pos = 0;
        float prev_x_pos = 0;
        float prev_y_pos = 0;
        for (std::size_t i = 0; i < sampleCount; i += 2) {
            float x_sample = samples[i] / 32768.f;
            float y_sample = (i+1 < sampleCount) ? samples[i + 1] / 32768.f : 0.f;

            float x_pos = m_center.x + x_sample * m_radius;
            float y_pos = m_center.y + y_sample * m_radius;
            float distance = dist(x_pos, y_pos, prev_x_pos, prev_y_pos);
            
            sf::Vertex vertex;
            vertex.position = {x_pos, y_pos};
            //sf::Color vc(0, 255, 0, std::max(0.f,(1.f-distance/50))*255);
            sf::Color vc(0, 255, 0, 255);
            vertex.color = vc;
            m_vertices.append(vertex);
            prev_x_pos = x_pos;
            prev_y_pos = y_pos;
        }
        return true;
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_vertices.getVertexCount() == 0) {
            return;
        }

        // Draw the main, centered line
        target.draw(m_vertices, states);
        
        // Create copies with offsets for a thicker line
        sf::RenderStates offsetStates = states;
        int nCirclePts = 8;
        for (int i=0; i<n_layers; i++) {
            for (int j=0; j<nCirclePts; j++) {
                float angle = 2*M_PI/(nCirclePts-1-j);
                float offset = (i+1)*m_thickness;
                offsetStates.transform = states.transform;
                offsetStates.transform.translate(sf::Vector2f(offset*std::cos(angle), offset*std::sin(angle)));
                target.draw(m_vertices, offsetStates);
            }
        }
    }

    float m_radius = 0.f;
    sf::Vector2f m_center;
    sf::VertexArray m_vertices;
    mutable std::mutex m_mutex;
    std::vector<std::string> m_availableDevices;

    // Parameters
    float m_thickness = 0.5f;
    unsigned int n_layers = 3;
    unsigned int maxPersistentFrames = 20;
    unsigned int persistenceStrength = 200;
};


int main() {
    unsigned int width = 800;
    unsigned int height = 600;
    // ---- Device Selection ----
    auto availableDevices = sf::SoundRecorder::getAvailableDevices();
    if (availableDevices.empty()) { /* ... */ return -1; }
    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < availableDevices.size(); ++i) { std::cout << i << ": " << availableDevices[i] << std::endl; }
    std::cout << "Enter the number of the device to use: ";
    int deviceIndex;
    std::cin >> deviceIndex;
    if (deviceIndex < 0 || static_cast<size_t>(deviceIndex) >= availableDevices.size()) { /* ... */ return -1; }
    const std::string& selectedDevice = availableDevices[deviceIndex];
    
    // ---- Oscilloscope and Window Setup ----
    Oscilloscope oscilloscope;
    oscilloscope.setChannelCount(2);
    if (!oscilloscope.startRecording(selectedDevice)) { /* ... */ return -1; }
    std::cout << "Listening to device: " << selectedDevice << std::endl;
    sf::RenderWindow window(sf::VideoMode({width, height}), "C++ Virtual Oscilloscope");
    window.setFramerateLimit(60);
    oscilloscope.updateView(window.getSize());

    sf::RenderTexture traceTexture({width, height});
    sf::RenderTexture blurTexture({width, height});
    sf::RenderTexture frameTexture({width, height});

    std::deque<sf::Texture> persistentFrames;
    
    //std::cout << "----------------------------------------------------" << std::endl;
    //std::cout << "Persistence Alpha: " << fadeAlpha << " (Use +/- keys to change)" << std::endl;
    //std::cout << "Beam Offset/Thickness: " << oscilloscope.getThickness() << " (Use [ and ] keys to change)" << std::endl;
    //std::cout << "----------------------------------------------------" << std::endl;

    sf::Shader gaussianBlurShader;
    if (!gaussianBlurShader.loadFromFile("blur.frag", sf::Shader::Type::Fragment)) { /* ... */ return -1; }
    gaussianBlurShader.setUniform("texture", sf::Shader::CurrentTexture); // This will be the input texture to the shader

    float gaussianBlurSpread = 1.0f; // Controls how "wide" the blur is. 
    std::cout << "Gaussian Blur Spread: " << gaussianBlurSpread << " (Use PageUp/PageDown keys to change)" << std::endl;

    // ---- 4. Main Application Loop ----
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
                persistentFrames.clear();
                oscilloscope.updateView(sizeVec);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::RBracket) {
                    oscilloscope.setThickness(oscilloscope.getThickness() + 1);
                    std::cout << "Layers: " << oscilloscope.getThickness() << std::endl;
                } else if (keyPressed->code == sf::Keyboard::Key::LBracket) {
                    oscilloscope.setThickness(oscilloscope.getThickness() - 1);
                    std::cout << "Layers: " << oscilloscope.getThickness() << std::endl;
                } else if (keyPressed->code == sf::Keyboard::Key::PageUp) {
                    gaussianBlurSpread = std::min(gaussianBlurSpread + 0.2f, 10.f);
                    std::cout << "Gaussian Blur Spread: " << gaussianBlurSpread << std::endl;
                } else if (keyPressed->code == sf::Keyboard::Key::PageDown) {
                    gaussianBlurSpread = std::max(gaussianBlurSpread - 0.2f, 0.2f);
                    std::cout << "Gaussian Blur Spread: " << gaussianBlurSpread << std::endl;
                }
            }
        }

        traceTexture.clear(sf::Color::Transparent);
        traceTexture.draw(oscilloscope);
        traceTexture.display();

        // ---- Gaussian Blur Pass 1: Horizontal ----
        gaussianBlurShader.setUniform("texture", traceTexture.getTexture());
        gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(traceTexture.getSize()));
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
        gaussianBlurShader.setUniform("blur_spread_px", gaussianBlurSpread);
        
        blurTexture.clear(sf::Color::Transparent);
        blurTexture.draw(sf::Sprite(traceTexture.getTexture()), &gaussianBlurShader);
        blurTexture.display();

        // ---- Gaussian Blur Pass 2: Vertical ----
        gaussianBlurShader.setUniform("texture", blurTexture.getTexture());
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

        frameTexture.clear(sf::Color::Transparent);
        frameTexture.draw(sf::Sprite(blurTexture.getTexture()), &gaussianBlurShader);
        frameTexture.display();

        if (persistentFrames.size() >= oscilloscope.getPersistenceFrames()) {
            persistentFrames.pop_back(); 
        }
        persistentFrames.push_front(frameTexture.getTexture()); // Add a copy

        window.clear(sf::Color::Black);

        for (size_t i = 0; i < persistentFrames.size(); ++i) {
            sf::Sprite frameSprite(persistentFrames[i]);
            
            float alphaRatio = 1.0f;
            if (persistentFrames.size() > 1) { // Avoid division by zero if only 1 frame
                 alphaRatio = 1.0f - (static_cast<float>(i) / (persistentFrames.size() -1) );
            }
            std::uint8_t alpha = 0;
            if (i == 0) { // Newest frame
                alpha = 255;
            } else {
                float fadeFactor = static_cast<float>(oscilloscope.getPersistenceStrength()) / 255.f; // How much the oldest frame retains opacity
                alpha = static_cast<std::uint8_t>(255.f * ( ( (oscilloscope.getPersistenceFrames() - 1.f - i) / (oscilloscope.getPersistenceFrames() -1.f) ) * (1.f - fadeFactor) + fadeFactor ) );
                if (i == oscilloscope.getPersistenceFrames() -1) alpha = static_cast<std::uint8_t>(255.f * fadeFactor); // Oldest frame

                // Simplified linear fade from 255 to ~0 for oldest
                alpha = static_cast<std::uint8_t>(255.f * (1.f - static_cast<float>(i) / oscilloscope.getPersistenceFrames()));

            }


            frameSprite.setColor(sf::Color(0, 255, 0, alpha));
            window.draw(frameSprite);
        }
        

        window.display();
    }

    return 0;
}