#include "include/helpers.hpp"

float dist(float x1, float y1, float x2, float y2) {
    return std::hypot(x1 - x2, y1 - y2);
}
