#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "util.glsl"

const vec3 RandomScale3 = {0.1031, 0.1030, 0.0973};

// https://github.com/LWJGL/lwjgl3-demos/blob/main/res/org/lwjgl/demo/opengl/raytracing/randomCommon.glsl
// BSD 3-Clause licensed
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

// https://www.shadertoy.com/view/XlGcRh
uint pcg(inout uint v) {
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    v = (word >> 22u) ^ word;
    return v;
}

float randomFloat(inout uint state) {
    return (pcg(state) & 0xFFFFFFu) / 16777216.0;
}


#endif //RANDOM_GLSL
