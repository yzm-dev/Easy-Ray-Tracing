#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

#include "Ray.h"
#include "AABB.h"

// GPU-packing triangle (5 vec4 per tri expected in shader)
class Triangle {
public:
    glm::vec4 v0, v1, v2;
    glm::vec4 albedo;
    glm::vec4 emission;

    Triangle();
    Triangle(const glm::vec4& _v0, const glm::vec4& _v1, const glm::vec4& _v2,
        const glm::vec4& _albedo, const glm::vec4& _emission);
};

// CPU triangle used for BVH
class bvhTri {
public:
    glm::vec3 v0, v1, v2;
    int id; // optional: original index
    AABB bounds;
    glm::vec3 centroid;
    glm::vec3 albedo;
    glm::vec3 emission;

    bvhTri();
    bvhTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int _id = 0,
        const glm::vec3& alb = glm::vec3(0.8f), const glm::vec3& emi = glm::vec3(0.0f));

    // Möller–Trumbore
    // static bool intersectTriangle(const Ray& r, const bvhTri& tri, float& t, float& u, float& v);
};