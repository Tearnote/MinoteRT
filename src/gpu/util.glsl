#ifndef UTIL_GLSL
#define UTIL_GLSL

const float Pi = 3.14159265359;

// https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/
float Max3(float x, float y, float z) { return max(x, max(y, z)); }
vec3 tonemap(vec3 c) { return c * (1.0 / (Max3(c.r, c.g, c.b) + 1.0)); }
vec3 tonemapInvert(vec3 c) { return c * (1.0 / (1.0 - Max3(c.r, c.g, c.b))); }

// Conversion from linear RGB to sRGB
// http://www.java-gaming.org/topics/fast-srgb-conversion-glsl-snippet/37583/view.html
vec3 srgbEncode(vec3 _color) {
    float r = _color.r < 0.0031308 ? 12.92 * _color.r : 1.055 * pow(_color.r, 1.0/2.4) - 0.055;
    float g = _color.g < 0.0031308 ? 12.92 * _color.g : 1.055 * pow(_color.g, 1.0/2.4) - 0.055;
    float b = _color.b < 0.0031308 ? 12.92 * _color.b : 1.055 * pow(_color.b, 1.0/2.4) - 0.055;
    return vec3(r, g, b);
}

#endif //UTIL_GLSL
