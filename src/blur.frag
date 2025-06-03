#version 120 // SFML typically uses older GLSL versions

uniform sampler2D texture;
uniform float blur_spread_px; // How many pixels apart to space the samples (e.g., 1.0, 2.0)
uniform vec2 texture_size;   // The dimensions of the texture being blurred (e.g., 800.0, 600.0)
uniform vec2 blur_direction; // (1.0, 0.0) for horizontal, (0.0, 1.0) for vertical

void main() {
    // Calculate the offset for one pixel in the direction of the blur
    vec2 base_offset = blur_direction / texture_size;

    // Gaussian weights for 9 samples (center + 4 on each side).
    // These are pre-calculated and normalized (sum to ~1.0).
    float weight[5];
    weight[0] = 0.2270270270; // Center pixel
    weight[1] = 0.1945945946; // Pixels at +/- 1 * spread offset
    weight[2] = 0.1216216216; // Pixels at +/- 2 * spread offset
    weight[3] = 0.0540540541; // Pixels at +/- 3 * spread offset
    weight[4] = 0.0162162162; // Pixels at +/- 4 * spread offset

    // Accumulate color, starting with the center pixel
    vec4 sum = texture2D(texture, gl_TexCoord[0].xy) * weight[0];

    // Sample neighboring pixels along the blur_direction
    // The blur_spread_px scales how far apart these samples are.
    for (int i = 1; i < 5; i++) {
        vec2 current_offset = base_offset * float(i) * blur_spread_px;
        sum += texture2D(texture, gl_TexCoord[0].xy + current_offset) * weight[i];
        sum += texture2D(texture, gl_TexCoord[0].xy - current_offset) * weight[i];
    }

    // Modulate with vertex color (sf::Sprite defaults to white)
    gl_FragColor = gl_Color * sum;
}

