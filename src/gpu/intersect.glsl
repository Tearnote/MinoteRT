#ifndef INTERSECT_GLSL
#define INTERSECT_GLSL

#include "types.glsl"

struct Ray {
    Point origin;
    Vector direction;
};

struct Sphere {
    Point center;
    float radius;
    Color albedo;
};

struct Intersection {
    Point position;
    float t;
    Vector normal;
    Color albedo;
};

Point rayAt(Ray ray, float t) {
    return ray.origin + ray.direction * t;
}

Intersection raySphereIntersect(Ray ray, Sphere sphere) {
    Point oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    Intersection result;
    if (discriminant < 0) {
        result.t = -1;
    } else {
        result.t = (-half_b - sqrt(discriminant)) / a;
        result.position = rayAt(ray, result.t);
        result.normal = normalize(result.position - sphere.center);
        result.albedo = sphere.albedo;
    }
    return result;
}

#endif //INTERSECT_GLSL
