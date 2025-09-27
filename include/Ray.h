# pragma once

#include <glm/glm.hpp>

class Ray {
public:
    glm::vec3 o; // origin
    glm::vec3 d; // direction (normalized expected)
    float tmin;
    float tmax;

    Ray();
    Ray(const glm::vec3& _o, const glm::vec3& _d, float _tmin = 1e-4f, float _tmax = 1e30f);
};