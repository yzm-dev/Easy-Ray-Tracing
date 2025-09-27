#include "Ray.h"

Ray::Ray()
    : o(0.0f), d(0.0f, 0.0f, 1.0f), tmin(1e-4f), tmax(1e30f) {}

Ray::Ray(const glm::vec3& _o, const glm::vec3& _d, float _tmin, float _tmax)
    : o(_o), d(_d), tmin(_tmin), tmax(_tmax) {}