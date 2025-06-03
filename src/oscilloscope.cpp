#include "include/oscilloscope.hpp"

void Oscilloscope::updateView(const sf::Vector2u& newSize) {
    m_center.x = static_cast<float>(newSize.x) / 2.f;
    m_center.y = static_cast<float>(newSize.y) / 2.f;
    m_radius = std::min(static_cast<float>(newSize.x), static_cast<float>(newSize.y)) / 1.5f;
}

bool Oscilloscope::startRecording(const std::string& deviceName) {
    if (!setDevice(deviceName)) {
        // Consider adding error logging here
        // std::cerr << "Failed to set device: " << deviceName << std::endl;
        return false;
    }
    return start(); // Calls sf::SoundRecorder::start()
}

void Oscilloscope::setLayerCount(unsigned int n) {
    n_layers = std::max(1u, n); // Ensure at least one layer
}

unsigned int Oscilloscope::getLayerCount() const {
    return n_layers;
}

void Oscilloscope::setPersistenceFrames(unsigned int n) {
    maxPersistentFrames = n;
}

unsigned int Oscilloscope::getPersistenceFrames() const {
    return maxPersistentFrames;
}

void Oscilloscope::setPersistenceStrength(unsigned int n) {
    persistenceStrength = n;
}

unsigned int Oscilloscope::getPersistenceStrength() const {
    return persistenceStrength;
}

void Oscilloscope::setScale(float s) {
    scale = std::min(std::max(0.f, s), 1.f);
}

float Oscilloscope::getScale() const {
    return scale;
}

void Oscilloscope::setBlurSpread(float b) {
    gaussianBlurSpread = std::max(0.f, b);
}

float Oscilloscope::getBlurSpread() const {
    return gaussianBlurSpread;
}


bool Oscilloscope::onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vertices.clear();
    m_vertices.setPrimitiveType(sf::PrimitiveType::LineStrip);
    m_vertices.append(prev_final_vertex);
    for (std::size_t i = 0; i < sampleCount; i += 2) { // Process stereo samples
        float x_sample = static_cast<float>(samples[i]) / 32768.f; // Normalize left channel
        float y_sample = 0.f;
        if (i + 1 < sampleCount) {
            y_sample = static_cast<float>(samples[i + 1]) / 32768.f; // Normalize right channel
        }

        float x_pos = m_center.x + x_sample * m_radius * scale;
        float y_pos = m_center.y + y_sample * m_radius * scale;

        float sample_distance = dist(prev_x_pos, prev_y_pos, x_pos, y_pos);
        
        uint8_t shift = 255-std::min(static_cast<uint8_t>(sample_distance)*100, 255);
        sf::Vertex vertex({x_pos, y_pos}, sf::Color(shift, 255, shift, 255));
        //vertex.position = {x_pos, y_pos};
        // Default color: green, fully opaque
        //vertex.color = sf::Color(dbg, 255-dbg, 0, 255); 
        //dbg ++;
        //if (dbg > 255) dbg = 0;
        m_vertices.append(vertex);

        prev_x_pos = x_pos;
        prev_y_pos = y_pos;
    }
    prev_final_vertex = m_vertices[m_vertices.getVertexCount() - 1];

    //m_vertices.append(firstVertex);
    //if (dbg == 0) {
    //    dbg = 255;
    //} else {
    //    dbg = 0;
    //}
    return true; // Continue recording
}

void Oscilloscope::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_vertices.getVertexCount() == 0) {
        return; // Nothing to draw
    }

    // Draw the main, centered line
    target.draw(m_vertices, states);
    
    // Create copies with offsets for a thicker line effect
    sf::RenderStates offsetStates = states; // Copy original states
    static constexpr unsigned int nCirclePts = 8; // Number of points to create a "circular" offset

    for (unsigned int i = 0; i < n_layers; ++i) {
        for (unsigned int j = 0; j < nCirclePts; ++j) {
            float angle = (2.f * static_cast<float>(std::numbers::pi) * static_cast<float>(j)) / static_cast<float>(nCirclePts);
            float offsetDistance = (static_cast<float>(i) + 1.f) * m_thickness;
            
            // Reset transform to original state's transform before applying new translation
            offsetStates.transform = states.transform; 
            offsetStates.transform.translate(sf::Vector2f(offsetDistance * std::cos(angle), offsetDistance * std::sin(angle)));
            
            target.draw(m_vertices, offsetStates);
        }
    }
}