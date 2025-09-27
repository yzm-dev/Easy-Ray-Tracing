#pragma once

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include "AABB.h"
#include "Triangle.h"
#include "Ray.h"

class BVHNode {
public:
    AABB bounds;
    int left;    // index of left child (-1 if leaf)
    int right;   // index of right child (-1 if leaf)
    int start;   // start index into primitive array for leaf
    int count;   // number of primitives in leaf

    BVHNode();
};

class BVH {
public:
    std::vector<BVHNode> nodes;
    std::vector<bvhTri> primitives; // will be reordered during build

    BVH();

    int build_recursive(int start, int end, int maxLeafSize);
    void build(std::vector<bvhTri> tris, int maxLeafSize = 8);

    /*bool intersectNearest(const Ray& r, float& out_t, int& out_triIdx, float& out_u, float& out_v) const;*/
};