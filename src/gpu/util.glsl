#ifndef UTIL_GLSL
#define UTIL_GLSL

const float Pi = 3.14159265359;

// https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/
float Max3(float x, float y, float z) { return max(x, max(y, z)); }
vec3 tonemap(vec3 c) { return c * (1.0 / (Max3(c.r, c.g, c.b) + 1.0)); }
vec3 tonemapInvert(vec3 c) { return c * (1.0 / (1.0 - Max3(c.r, c.g, c.b))); }

#endif //UTIL_GLSL
