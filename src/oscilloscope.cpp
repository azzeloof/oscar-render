#include "include/oscilloscope.hpp"

sf::Vector2f normalize(const sf::Vector2f& source) {
    float length = std::hypot(source.x, source.y);
    if (length != 0)
        return {source.x / length, source.y / length};
    else
        return {0.f, 0.f};
}

sf::Vector2f perpendicular(const sf::Vector2f& source) {
    return {-source.y, source.x};
}

float distance(const sf::Vector2f& p1, const sf::Vector2f& p2) {
    return std::hypot(p1.x - p2.x, p1.y - p2.y);
}

float distance(float x1, float y1, float x2, float y2) {
    return std::hypot(x1 - x2, y1 - y2);
    return std::hypot(x1 - x2, y1 - y2);
}

Oscilloscope::Oscilloscope() : m_has_valid_last_point(false), m_thickness(1.f) {
    prev_vertex.position = {0.f, 0.f};
    prev_vertex.color = sf::Color::Transparent;
} 

void Oscilloscope::updateView(const sf::Vector2u& newSize) {
    m_center.x = static_cast<float>(newSize.x) / 2.f;
    m_center.y = static_cast<float>(newSize.y) / 2.f;
    m_radius = std::min(static_cast<float>(newSize.x), static_cast<float>(newSize.y)) / 2.0f;
}

void Oscilloscope::setTraceThickness(float thickness) {
    m_thickness = std::max(thickness, 1.f);;
}

float Oscilloscope::getTraceThickness() const {
    return m_thickness;
}

void Oscilloscope::setTraceColor(sf::Color c) {
    trace_color = c;
}

sf::Color Oscilloscope::getTraceColor() const {
    return trace_color;
}

void Oscilloscope::setPersistenceSamples(unsigned int n) {
    maxPersistentSamples = n;
    alpha_values.resize(n);
    center_line_points.resize(n);
}

unsigned int Oscilloscope::getPersistenceSamples() const {
    return maxPersistentSamples;
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

void Oscilloscope::setAlphaScale(unsigned int a) {
    alpha_scale = a;
}

unsigned int Oscilloscope::getAlphaScale() const {
    return alpha_scale;
}


void Oscilloscope::processSamples(const std::int16_t* samples, std::size_t sampleCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    sf::Vector2f prev_xy;
    if (m_has_valid_last_point) {
        prev_xy = prev_vertex.position;
    } else if (sampleCount > 0) {
        float x_sample0 = static_cast<float>(samples[0]) / 32768.f;
        float y_sample0 = (sampleCount > 1) ? static_cast<float>(samples[1]) / 32768.f : 0.f;
        prev_xy = {m_center.x + x_sample0 * m_radius * scale,
                                          m_center.y + y_sample0 * m_radius * scale};
    } else {
        m_triangle_strip.clear();
        return;
    }
    for (std::size_t i = 0; i < sampleCount; i += 2) {
        float x_sample = static_cast<float>(samples[i]) / 32768.f;
        float y_sample = 0.f;
        if (i + 1 < sampleCount) {
            y_sample = static_cast<float>(samples[i + 1]) / 32768.f;
        }

        sf::Vector2f current_screen_pos(m_center.x + x_sample * m_radius * scale,
                                        m_center.y - y_sample * m_radius * scale);

        float sample_dist = distance(prev_xy, current_screen_pos)/(m_radius*scale);
        uint8_t alpha = static_cast<uint8_t>(255.f - std::min(sample_dist * alpha_scale, 255.f));

        center_line_points.push_front(sf::Vertex(current_screen_pos,sf::Color(trace_color.r, trace_color.g, trace_color.b, alpha)));
        alpha_values.push_front(alpha);

        if (center_line_points.size() > maxPersistentSamples) {
            center_line_points.pop_back();
            alpha_values.pop_back();
        }
        prev_xy = current_screen_pos;
    }

    if (!center_line_points.empty()) {
        prev_vertex = center_line_points.front();
        m_has_valid_last_point = true;
    } else {
        m_has_valid_last_point = false;
    }

    m_triangle_strip.clear();
    m_triangle_strip.setPrimitiveType(sf::PrimitiveType::TriangleStrip);

    if (center_line_points.size() < 2) {
        return;
    }

    for (std::size_t i = 0; i < center_line_points.size(); i++) {
        sf::Color oc = center_line_points[i].color;
        float da = 255.f*static_cast<float>(i)/static_cast<float>(center_line_points.size());
        uint8_t alpha = 0;
        if (alpha_values[i] >= da) {
            alpha = alpha_values[i]-static_cast<uint8_t>(da);
        }
        center_line_points[i].color = sf::Color(oc.r, oc.g, oc.b, static_cast<uint8_t>(alpha));
    }

    for (std::size_t i = 0; i < center_line_points.size(); ++i) {
        const sf::Vertex& P_i = center_line_points[i];
        sf::Vector2f normal_vec;

        if (i == 0) {
            const sf::Vertex& P_next = center_line_points[i + 1];
            sf::Vector2f tangent = normalize(P_next.position - P_i.position);
            normal_vec = perpendicular(tangent);
        } else if (i == center_line_points.size() - 1) {
            const sf::Vertex& P_prev = center_line_points[i - 1];
            sf::Vector2f tangent = normalize(P_i.position - P_prev.position);
            normal_vec = perpendicular(tangent);
        } else {
            const sf::Vertex& P_prev = center_line_points[i - 1];
            const sf::Vertex& P_next = center_line_points[i + 1];
            sf::Vector2f tangent_prev = normalize(P_i.position - P_prev.position);
            sf::Vector2f tangent_next = normalize(P_next.position - P_i.position);
            sf::Vector2f n1 = perpendicular(tangent_prev);
            sf::Vector2f n2 = perpendicular(tangent_next);
            normal_vec = normalize(n1 + n2);
            if (distance(normal_vec, {0.f, 0.f}) < 0.0001f) {
                normal_vec = n1;
            }
        }

        if (distance(normal_vec, {0.f, 0.f}) < 0.0001f) {
            normal_vec = sf::Vector2f(0.f, 1.f);
        }

        sf::Vertex v_strip_top(P_i.position + normal_vec * (m_thickness / 2.f), P_i.color);
        sf::Vertex v_strip_bottom(P_i.position - normal_vec * (m_thickness / 2.f), P_i.color);

        m_triangle_strip.append(v_strip_top);
        m_triangle_strip.append(v_strip_bottom);
    }
}

void Oscilloscope::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_triangle_strip.getVertexCount() == 0) {
        return;
    }
    target.draw(m_triangle_strip, states);
}
