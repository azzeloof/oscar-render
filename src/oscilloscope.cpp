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
    m_prev_normalized_pos = {0.f, 0.f};
}

void Oscilloscope::updateView(const sf::Vector2u& /*newSize*/) {
    // No longer needed — geometry is built in normalized space.
    // Kept for API compatibility.
}

void Oscilloscope::setTraceThickness(float thickness) {
    m_thickness = std::max(thickness, 1.f);
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
    // Trim if needed
    while (m_normalized_points.size() > maxPersistentSamples) {
        m_normalized_points.pop_back();
    }
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


void Oscilloscope::processSamples(const float* samples, std::size_t sampleCount) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Work entirely in normalized [-1, 1] coordinate space.
    // Audio samples are already in [-1, 1].
    // We apply `scale` here so the waveform amplitude is controlled.

    sf::Vector2f prev_norm;
    if (m_has_valid_last_point) {
        prev_norm = m_prev_normalized_pos;
    } else if (sampleCount > 0) {
        float x_sample0 = samples[0];
        float y_sample0 = (sampleCount > 1) ? samples[1] : 0.f;
        prev_norm = {x_sample0 * scale, y_sample0 * scale};
    } else {
        return;
    }

    for (std::size_t i = 0; i < sampleCount; i += 2) {
        float x_sample = samples[i];
        float y_sample = 0.f;
        if (i + 1 < sampleCount) {
            y_sample = samples[i + 1];
        }

        sf::Vector2f current_norm(x_sample * scale, -y_sample * scale);

        // Distance in normalized space for alpha computation
        float sample_dist = distance(prev_norm, current_norm);
        uint8_t alpha = static_cast<uint8_t>(255.f - std::min(sample_dist * alpha_scale, 255.f));

        NormalizedPoint pt;
        pt.pos = current_norm;
        pt.alpha = alpha;
        m_normalized_points.push_front(pt);

        if (m_normalized_points.size() > maxPersistentSamples) {
            m_normalized_points.pop_back();
        }
        prev_norm = current_norm;
    }

    if (!m_normalized_points.empty()) {
        m_prev_normalized_pos = m_normalized_points.front().pos;
        m_has_valid_last_point = true;
    } else {
        m_has_valid_last_point = false;
    }
}


sf::VertexArray Oscilloscope::buildTriangleStrip(const sf::Vector2u& targetSize) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    sf::VertexArray strip;
    strip.setPrimitiveType(sf::PrimitiveType::TriangleStrip);

    if (m_normalized_points.size() < 2) {
        return strip;
    }

    // Map normalized [-1, 1] to pixel coordinates for this target
    float centerX = static_cast<float>(targetSize.x) / 2.f;
    float centerY = static_cast<float>(targetSize.y) / 2.f;
    float radius = std::min(centerX, centerY);

    // First pass: compute screen positions and apply persistence fade to alpha
    struct ScreenPoint {
        sf::Vector2f pos;
        sf::Color color;
    };
    std::vector<ScreenPoint> screenPoints;
    screenPoints.reserve(m_normalized_points.size());

    for (std::size_t i = 0; i < m_normalized_points.size(); i++) {
        const auto& np = m_normalized_points[i];

        sf::Vector2f screenPos(centerX + np.pos.x * radius,
                               centerY + np.pos.y * radius);

        // Apply persistence fade
        float da = 255.f * static_cast<float>(i) / static_cast<float>(m_normalized_points.size());
        uint8_t alpha = 0;
        if (np.alpha >= static_cast<uint8_t>(da)) {
            alpha = np.alpha - static_cast<uint8_t>(da);
        }

        screenPoints.push_back({screenPos, sf::Color(trace_color.r, trace_color.g, trace_color.b, alpha)});
    }

    // Second pass: build triangle strip with thickness in pixel space
    for (std::size_t i = 0; i < screenPoints.size(); ++i) {
        const auto& P_i = screenPoints[i];
        sf::Vector2f normal_vec;

        if (i == 0) {
            sf::Vector2f tangent = normalize(screenPoints[i + 1].pos - P_i.pos);
            normal_vec = perpendicular(tangent);
        } else if (i == screenPoints.size() - 1) {
            sf::Vector2f tangent = normalize(P_i.pos - screenPoints[i - 1].pos);
            normal_vec = perpendicular(tangent);
        } else {
            sf::Vector2f tangent_prev = normalize(P_i.pos - screenPoints[i - 1].pos);
            sf::Vector2f tangent_next = normalize(screenPoints[i + 1].pos - P_i.pos);
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

        // Thickness is always in pixels — same for main window and preview
        strip.append(sf::Vertex(P_i.pos + normal_vec * (m_thickness / 2.f), P_i.color));
        strip.append(sf::Vertex(P_i.pos - normal_vec * (m_thickness / 2.f), P_i.color));
    }

    return strip;
}


void Oscilloscope::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    sf::VertexArray strip = buildTriangleStrip(target.getSize());
    if (strip.getVertexCount() == 0) {
        return;
    }
    target.draw(strip, states);
}
