#ifndef INTERSECT_GLSL
#define INTERSECT_GLSL

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Sphere {
    vec3 center;
    float radius;
    vec3 albedo;
};

struct Intersection {
    vec3 position;
    float t;
    vec3 normal;
    uint primitiveId;
};

vec3 rayAt(Ray ray, float t) {
    return ray.origin + ray.direction * t;
}

float raySphereIntersect(Ray ray, Sphere sphere) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0.0)
        return -1.0;
    else
        return (-half_b - sqrt(discriminant)) / a;
}

#endif //INTERSECT_GLSL
