#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <cmath>

#include "Ray.h"

class AABB {
public:
	glm::vec3 bmin;
	glm::vec3 bmax;

	AABB();

	static AABB empty();

	void expand(const glm::vec3& p);
	void expand(const AABB& b);

	glm::vec3 centroid() const;

	//bool intersectAABB(const AABB& box, const Ray& r, float& out_near, float& out_far);
};