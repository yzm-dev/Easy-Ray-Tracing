#include "AABB.h"

AABB::AABB()
    : bmin(std::numeric_limits<float>::infinity())
    , bmax(-std::numeric_limits<float>::infinity()) {}

AABB AABB::empty() {
    return AABB();
}

void AABB::expand(const glm::vec3& p) {
    bmin = glm::min(bmin, p);
    bmax = glm::max(bmax, p);
}

void AABB::expand(const AABB& b) {
    bmin = glm::min(bmin, b.bmin);
    bmax = glm::max(bmax, b.bmax);
}

glm::vec3 AABB::centroid() const {
    return 0.5f * (bmin + bmax);
}

//Slab intersection.Returns true and writes tnear, tfar if intersects
//bool AABB::intersectAABB(const AABB& box, const Ray& r, float& out_near, float& out_far) {
//	glm::vec3 invD = 1.0f / r.d;
//    glm::vec3 t0s = (bmin - r.o) * invD;
//    glm::vec3 t1s = (bmax - r.o) * invD;
//    glm::vec3 tsmaller = glm::min(t0s, t1s);
//    glm::vec3 tbigger = glm::max(t0s, t1s);
//	float t_enter = glm::max(r.tmin, glm::max(tsmaller.x, glm::max(tsmaller.y, tsmaller.z)));
//    float t_exit = glm::min(r.tmax, glm::min(tbigger.x, glm::max(tbigger.y, tbigger.z)));
//	return t_enter < t_exit && t_exit > 0.0f;
//}