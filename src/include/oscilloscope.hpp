#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <algorithm> // For std::min, std::max
#include <cmath>


sf::Vector2f normalize(const sf::Vector2f& source);
sf::Vector2f perpendicular(const sf::Vector2f& source);
float distance(float x1, float y1, float x2, float y2);
float distance(const sf::Vector2f& p1, const sf::Vector2f& p2);

/**
 * @class Oscilloscope
 * @brief Captures and visualizes stereo audio data in real-time.
 */
class Oscilloscope : public sf::SoundRecorder, public sf::Drawable {
public:
    // Constructor (default is sufficient here)
    Oscilloscope(); 

    // Destructor (default is sufficient here)
    // /virtual ~Oscilloscope();

    /**
     * @brief Updates the view parameters based on the new window/target size.
     * @param newSize The new size of the render target.
     */
    void updateView(const sf::Vector2u& newSize);

    /**
     * @brief Sets the recording device and starts capturing audio.
     * @param deviceName The name of the audio input device.
     * @return True if recording started successfully, false otherwise.
     */
    bool startRecording(const std::string& deviceName);
    
    // Make setChannelCount from sf::SoundRecorder publicly accessible
    using sf::SoundRecorder::setChannelCount;

    /**
     * @brief Sets the trace thickness.
     * @param thickness Trace width (px)
     */
    void setTraceThickness(float thickness);

    /**
     * @brief Gets the current trace thickness.
     * @return Thickness (px).
     */
    float getTraceThickness() const;

    /**
     * @brief Sets the maximum number of frames for persistence effect.
     * @param n Number of frames.
     */
    void setPersistenceFrames(unsigned int n);

    /**
     * @brief Gets the maximum number of frames for persistence effect.
     * @return Number of persistence frames.
     */
    unsigned int getPersistenceFrames() const;

    /**
     * @brief Sets the strength of the oldest persistent frame's visibility.
     * @param n Strength value (typically 0-255).
     */
    void setPersistenceStrength(unsigned int n);

    /**
     * @brief Gets the strength of the oldest persistent frame's visibility.
     * @return Persistence strength.
     */
    unsigned int getPersistenceStrength() const;

    /**
     * @brief Sets the display scale.
     * @param s Scale value (typically 0-1).
     */
    void setScale(float s);

    /**
     * @brief Gets the display scale.
     * @return Scale.
     */
    float getScale() const;
    
    /**
     * @brief Sets the gaussian blur spread.
     * @param s Blur spread value (typically 0-10).
     */
    void setBlurSpread(float b);

    /**
     * @brief Gets the gaussian blue spread.
     * @return Blue spread.
     */
    float getBlurSpread() const;

private:
    /**
     * @brief Called by SFML when new audio samples are available.
     * @param samples Pointer to the array of audio samples.
     * @param sampleCount Number of samples in the array.
     * @return True to continue recording, false to stop.
     */
    virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override;

    /**
     * @brief Called by SFML to draw the oscilloscope to a render target.
     * @param target Render target to draw to.
     * @param states Current render states.
     */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    float m_radius = 0.f;               // Radius of the oscilloscope display area.
    sf::Vector2f m_center;              // Center point of the oscilloscope display area.
    sf::VertexArray m_vertices;         // Vertex array for drawing the audio waveform.
    mutable std::mutex m_mutex;         // Mutex for thread-safe access to m_vertices.
    
    // This member was in the original class but not used by its methods.
    // It might be intended for future use or external management.
    std::vector<std::string> m_availableDevices; 

    float prev_x_pos = 0.f;
    float prev_y_pos = 0.f;
    sf::Vertex m_last_processed_center_point;
    sf::VertexArray m_triangle_strip;
    bool m_has_valid_last_point;
    //sf::Vertex prev_final_vertex;

    // Parameters
    float scale = 1.f;                      // Scale of the displayed trace
    float m_thickness = 1.f;                // Thickness of the trace.
    unsigned int n_layers = 3;              // Number of layers for visual thickness.
    unsigned int maxPersistentFrames = 1;   // Max frames for persistence effect (used by main loop).
    unsigned int persistenceStrength = 0;   // Strength of oldest persistent frame (used by main loop).
    float gaussianBlurSpread = 0.f;         // Controls how "wide" the blur is.
};

#endif // OSCILLOSCOPE_HPP