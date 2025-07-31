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
#include <deque>
#include <iostream>


sf::Vector2f normalize(const sf::Vector2f& source);
sf::Vector2f perpendicular(const sf::Vector2f& source);
float distance(float x1, float y1, float x2, float y2);
float distance(const sf::Vector2f& p1, const sf::Vector2f& p2);

/**
 * @class Oscilloscope
 * @brief Captures and visualizes stereo audio data in real-time.
 */
class Oscilloscope : public sf::Drawable {
public:
    // Constructor (default is sufficient here)
    Oscilloscope(); 

    /**
     * @brief Updates the view parameters based on the new window/target size.
     * @param newSize The new size of the render target.
     */
    void updateView(const sf::Vector2u& newSize);

    /**
     * @brief Processes a new chunk of audio samples.
     * @param samples Pointer to the array of audio samples.
     * @param sampleCount Number of samples in the array.
     */
    void processSamples(const std::int16_t* samples, std::size_t sampleCount);

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
     * @brief Sets the trace color (RGBA).
     * @param c SFML color.
     */
    void setTraceColor(sf::Color c);

    /**
     * @brief Gets the trace color.
     * @return SFML color (RGBA).
     */
    sf::Color getTraceColor() const;

    /**
     * @brief Sets the maximum number of frames for persistence effect.
     * @param n Number of frames.
     */
    void setPersistenceSamples(unsigned int n);

    /**
     * @brief Gets the maximum number of points for persistence effect.
     * @return Number of persistence points.
     */
    unsigned int getPersistenceSamples() const;

    /**
     * @brief Sets the strength of the oldest persistent points's visibility.
     * @param n Strength value (typically 0-255).
     */
    void setPersistenceStrength(unsigned int n);

    /**
     * @brief Gets the strength of the oldest persistent points's visibility.
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

    /**
     * @brief Sets the alpha scale.
     * @param a Alpha scale value (typically 0-10000).
     */
    void setAlphaScale(unsigned int a);

    /**
     * @brief Gets the alpha scale.
     * @return BAlpha scale.
     */
    unsigned int getAlphaScale() const;

private:
    /**
     * @brief Called by SFML to draw the oscilloscope to a render target.
     * @param target Render target to draw to.
     * @param states Current render states.
     */
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    float m_radius = 0.f;
    sf::Vector2f m_center;
    mutable std::mutex m_mutex;

    sf::Vertex prev_vertex;
    sf::VertexArray m_triangle_strip;
    std::deque<sf::Vertex> center_line_points;
    std::deque<uint8_t> alpha_values;
    bool m_has_valid_last_point;

    // Parameters
    float scale = 1.f;
    float m_thickness = 1.f;
    unsigned int maxPersistentSamples = 10000;
    unsigned int persistenceStrength = 0;
    float gaussianBlurSpread = 0.f;
    sf::Color trace_color = sf::Color::Green;
    unsigned int alpha_scale = 5000;
};

#endif // OSCILLOSCOPE_HPP
