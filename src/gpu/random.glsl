#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "util.glsl"

const vec3 RandomScale3 = {0.1031, 0.1030, 0.0973};

// https://github.com/LWJGL/lwjgl3-demos/blob/main/res/org/lwjgl/demo/opengl/raytracing/randomCommon.glsl
vec3 randomSpherePoint(vec3 rand) {
    float ang1 = (rand.x + 1.0) * Pi; // [-1..1) -> [0..2*PI)
    float u = rand.y; // [-1..1), cos and acos(2v-1) cancel each other out, so we arrive at [-1..1)
    float u2 = u * u;
    float sqrt1MinusU2 = sqrt(1.0 - u2);
    float x = sqrt1MinusU2 * cos(ang1);
    float y = sqrt1MinusU2 * sin(ang1);
    float z = u;
    return vec3(x, y, z);
}

uint xorshift(inout uint state) {
    uint x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 15;
    state = x;
    return x;
}

float randomFloat(inout uint state) {
    return (xorshift(state) & 0xFFFFFFu) / 16777216.0f;
}

#endif //RANDOM_GLSL
