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

vec2 random2(vec2 p) {
    vec3 p3 = fract(p.xyx * RandomScale3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

vec3 random3(vec3 p) {
    p = fract(p * RandomScale3);
    p += dot(p, p.yxz+19.19);
    return fract((p.xxy + p.yzz)*p.zyx);
}

#endif //RANDOM_GLSL
