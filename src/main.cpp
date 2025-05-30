#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.1415926
#endif

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

float dist(sf::Vertex A, sf::Vertex B) {
    return std::sqrt(std::pow((A.position.x - B.position.x),2) + std::pow((A.position.y - B.position.y),2));
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
            
            sf::Vertex vertex;
            vertex.position = {x_pos, y_pos};
            //if (i < 1000) {
            vertex.color = sf::Color::Green;
            //} else {
            //int pt = i / 2;
            //int nPt = sampleCount / 2;
            //std::cout << i*255/sampleCount << "\n";
            //sf::Color vc(i * 255 / sampleCount, 0, 0, 255);
            //    vertex.color = sf::Color::Blue;
            //}
            //vertex.color = vc;
            m_vertices.append(vertex);
            prev_x_pos = x_pos;
            prev_y_pos = y_pos;
        }
        /*
        float max_dist = 0.f;
        for (int i=0; i<m_vertices.getVertexCount()-2; i++) {
            float d = dist(m_vertices[i], m_vertices[i+1]);
            max_dist = (max_dist < d) ? d : max_dist;
        }
        for (int i=0; i<m_vertices.getVertexCount()-1; i++) {
            uint8_t alpha = (i+1 < sampleCount) ? (max_dist-dist(m_vertices[i], m_vertices[i+1]))/max_dist: 0;
            sf::Color vc(0, 255, 0, alpha);
            m_vertices[i].color = vc;
        }
        */
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

    sf::Vector2f m_center;
    float m_radius = 0.f;
    float m_thickness = 1.f;
    int n_layers = 10;

    sf::VertexArray m_vertices;
    mutable std::mutex m_mutex;
    std::vector<std::string> m_availableDevices;
};


int main() {
    unsigned int width = 800;
    unsigned int height = 600;
    // ---- 1. Device Selection ----
    auto availableDevices = sf::SoundRecorder::getAvailableDevices();
    if (availableDevices.empty()) { /* ... */ return -1; }
    std::cout << "Available audio devices:" << std::endl;
    for (size_t i = 0; i < availableDevices.size(); ++i) { std::cout << i << ": " << availableDevices[i] << std::endl; }
    std::cout << "Enter the number of the device to use: ";
    int deviceIndex;
    std::cin >> deviceIndex;
    if (deviceIndex < 0 || static_cast<size_t>(deviceIndex) >= availableDevices.size()) { /* ... */ return -1; }
    const std::string& selectedDevice = availableDevices[deviceIndex];
    
    // ---- 2. Oscilloscope and Window Setup ----
    Oscilloscope oscilloscope;
    oscilloscope.setChannelCount(2);
    if (!oscilloscope.startRecording(selectedDevice)) { /* ... */ return -1; }
    std::cout << "Listening to device: " << selectedDevice << std::endl;
    sf::RenderWindow window(sf::VideoMode({width, height}), "C++ Virtual Oscilloscope");
    window.setFramerateLimit(60);
    oscilloscope.updateView(window.getSize());

    // ---- 3. Graphics Setup for Persistence Effect ----
    sf::RenderTexture traceTexture({width, height});
    traceTexture.clear(sf::Color::Black);
    sf::RectangleShape traceRect;
    traceRect.setSize(sf::Vector2f(window.getSize()));
    //int fadeAlpha = 10;
    std::cout << "----------------------------------------------------" << std::endl;
    //std::cout << "Persistence Alpha: " << fadeAlpha << " (Use +/- keys to change)" << std::endl;
    std::cout << "Beam Offset/Thickness: " << oscilloscope.getThickness() << " (Use [ and ] keys to change)" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;
    
    // ---- Gaussian Blur Setup ----
    sf::Shader gaussianBlurShader;
    if (!gaussianBlurShader.loadFromFile("blur.frag", sf::Shader::Type::Fragment)) { /* ... */ return -1; }
    gaussianBlurShader.setUniform("texture", sf::Shader::CurrentTexture); // This will be the input texture to the shader

    float gaussianBlurSpread = 1.0f; // Controls how "wide" the blur is. 
    std::cout << "Gaussian Blur Spread: " << gaussianBlurSpread << " (Use PageUp/PageDown keys to change)" << std::endl;

    sf::RenderTexture horizontalBlurTexture({width, height});
    horizontalBlurTexture.clear(sf::Color::Transparent);

    sf::RenderTexture blurRender({width, height});
    blurRender.clear(sf::Color::Transparent); // Final blur output texture

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
                horizontalBlurTexture = sf::RenderTexture(sizeVec);
                blurRender = sf::RenderTexture(sizeVec);
                traceRect.setSize(sf::Vector2f(sizeVec));
                oscilloscope.updateView(sizeVec);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                //if (keyPressed->code == sf::Keyboard::Key::Equal || keyPressed->code == sf::Keyboard::Key::Add) {
                //    fadeAlpha = std::min(fadeAlpha + 1, 255);
                //    std::cout << "Persistence Alpha: " << fadeAlpha << std::endl;
                //} else if (keyPressed->code == sf::Keyboard::Key::Hyphen || keyPressed->code == sf::Keyboard::Key::Subtract) {
                //    fadeAlpha = std::max(fadeAlpha - 1, 0);
                //    std::cout << "Persistence Alpha: " << fadeAlpha << std::endl;
                //} 
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


        // ---- 5. Drawing Logic ----
        traceRect.setFillColor(sf::Color(0, 0, 0, 255));
        traceTexture.draw(traceRect);
        traceTexture.draw(oscilloscope);
        traceTexture.display();

        window.clear();

        // ---- Gaussian Blur Pass 1: Horizontal ----
        gaussianBlurShader.setUniform("texture_size", sf::Glsl::Vec2(traceTexture.getSize()));
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(1.f, 0.f));
        gaussianBlurShader.setUniform("blur_spread_px", gaussianBlurSpread);
        
        horizontalBlurTexture.clear(sf::Color::Transparent);
        horizontalBlurTexture.draw(sf::Sprite(traceTexture.getTexture()), &gaussianBlurShader);
        horizontalBlurTexture.display();

        // ---- Gaussian Blur Pass 2: Vertical ----
        // texture_size is the same, blur_spread_px is the same
        gaussianBlurShader.setUniform("blur_direction", sf::Glsl::Vec2(0.f, 1.f));

        blurRender.clear(sf::Color::Transparent);
        blurRender.draw(sf::Sprite(horizontalBlurTexture.getTexture()), &gaussianBlurShader);
        blurRender.display();

        // ---- Draw the final blurred result to the window ----
        window.draw(sf::Sprite(blurRender.getTexture()));
        // DO NOT draw persistenceTexture directly to the window here, as it will cover the blur.
        window.display();
    }

    return 0;
}