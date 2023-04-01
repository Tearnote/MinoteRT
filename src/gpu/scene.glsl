#ifndef SCENE_GLSL
#define SCENE_GLSL

const uint SphereCount = 5;
const Sphere Spheres[SphereCount] = {
    {{0.0, -1.7,    0.0}, 0.5, {0.2, 0.7, 0.0}},
    {{-0.8, -1.2,    -0.17}, 0.33, {0.0, 0.2, 0.7}},
    {{0.8, -1.2,    -0.17}, 0.33, {0.7, 0.0, 0.2}},
    {{0.0, -0.8,    -0.25}, 0.25, {1.0, 1.0, 1.0}},
    {{0.0, -1.0, -100.5}, 100, {0.2, 0.2, 0.2}}
};

#endif //SCENE_GLSL
