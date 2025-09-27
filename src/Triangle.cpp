#include "Triangle.h"

Triangle::Triangle()
    : v0(0.0f), v1(0.0f), v2(0.0f), albedo(0.0f), emission(0.0f) {}

Triangle::Triangle(const glm::vec4& _v0, const glm::vec4& _v1, const glm::vec4& _v2,
                   const glm::vec4& _albedo, const glm::vec4& _emission)
    : v0(_v0), v1(_v1), v2(_v2), albedo(_albedo), emission(_emission) {}

bvhTri::bvhTri()
	: v0(0.0f)
	, v1(0.0f)
	, v2(0.0f)
    , id(0)
    , bounds()
    , centroid(0.0f)
    , albedo(glm::vec3(0.8f))
    , emission(0.0f) {
    bounds.expand(v0);
    bounds.expand(v1);
    bounds.expand(v2);
    centroid = bounds.centroid();
}

bvhTri::bvhTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int _id,
               const glm::vec3& alb, const glm::vec3& emi)
    : v0(a)
    , v1(b)
    , v2(c)
    , id(_id)
    , bounds()
    , centroid(0.0f)
    , albedo(alb)
    , emission(emi) {
    bounds.expand(v0);
    bounds.expand(v1);
    bounds.expand(v2);
    centroid = bounds.centroid();
}

/*
// Möller–Trumbore
bool bvhTri::intersectTriangle(const Ray& r, const bvhTri& tri, float& t, float& u, float& v) {
    const float EPS = 1e-8f;
    glm::vec3 e1 = tri.v1 - tri.v0;
    glm::vec3 e2 = tri.v2 - tri.v0;
    glm::vec3 p = glm::cross(r.d, e2);
    float det = glm::dot(e1, p);
    if (std::fabs(det) < EPS) return false;
    float invDet = 1.0f / det;
    glm::vec3 tvec = r.o - tri.v0;
    u = glm::dot(tvec, p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 q = glm::cross(tvec, e1);
    v = glm::dot(r.d, q) * invDet;
    if (v < 0.0f || (u + v) > 1.0f) return false;
    t = glm::dot(e2, q) * invDet;
    return t > r.tmin && t < r.tmax;
}
*/