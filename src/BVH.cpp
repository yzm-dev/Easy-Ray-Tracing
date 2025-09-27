#include "BVH.h"

BVHNode::BVHNode()
    : bounds()
    , left(-1)
    , right(-1)
    , start(0)
    , count(0) {}

BVH::BVH() = default;

int BVH::build_recursive(int start, int end, int maxLeafSize) {
    // compute bounds and centroid bounds
    AABB bounds; 
    AABB centroidBounds;
    for (int i = start; i < end; ++i) {
        bounds.expand(primitives[i].bounds);
        centroidBounds.expand(primitives[i].centroid);
    }

    int nodeIndex = static_cast<int>(nodes.size());
    nodes.emplace_back();
    nodes[nodeIndex].bounds = bounds;

    int nPrims = end - start;
    if (nPrims <= maxLeafSize) {
        nodes[nodeIndex].start = start;
        nodes[nodeIndex].count = nPrims;
        nodes[nodeIndex].left = nodes[nodeIndex].right = -1;
        return nodeIndex;
    }

    // choose split axis by largest extent
    glm::vec3 extent = centroidBounds.bmax - centroidBounds.bmin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if ((axis == 0 ? extent.x : extent.y) < extent.z) axis = 2;

    int mid = (start + end) / 2;
    std::nth_element(primitives.begin() + start, primitives.begin() + mid, primitives.begin() + end,
        [axis](const bvhTri& a, const bvhTri& b) {
            if (axis == 0) return a.centroid.x < b.centroid.x;
            if (axis == 1) return a.centroid.y < b.centroid.y;
            return a.centroid.z < b.centroid.z;
        });

    // if degenerate split, make leaf
    if (mid == start || mid == end) {
        nodes[nodeIndex].start = start;
        nodes[nodeIndex].count = nPrims;
        nodes[nodeIndex].left = nodes[nodeIndex].right = -1;
        return nodeIndex;
    }

    int left = build_recursive(start, mid, maxLeafSize);
    int right = build_recursive(mid, end, maxLeafSize);
    nodes[nodeIndex].left = left;
    nodes[nodeIndex].right = right;
    nodes[nodeIndex].start = 0;
    nodes[nodeIndex].count = 0;
    nodes[nodeIndex].bounds = bounds; // already computed
    return nodeIndex;
}

void BVH::build(std::vector<bvhTri> tris, int maxLeafSize) {
    primitives = std::move(tris);
    nodes.clear();
    if (!primitives.empty()) build_recursive(0, static_cast<int>(primitives.size()), maxLeafSize);
}

/* Example (kept commented as in original):
bool BVH::intersectNearest(const Ray& r, float& out_t, int& out_triIdx, float& out_u, float& out_v) const {
    if (nodes.empty()) return false;
    std::vector<int> stack; stack.reserve(64);
    stack.push_back(0);
    bool hit = false;
    float nearestT = r.tmax;
    int nearestTri = -1;
    float hitu = 0.0f, hitv = 0.0f;

    while (!stack.empty()) {
        int nodeIdx = stack.back(); stack.pop_back();
        const BVHNode& node = nodes[nodeIdx];
        float tnear, tfar;
        if (!intersectAABB(node.bounds, r.o, r.d, r.tmin, nearestT, tnear, tfar)) continue;
        if (node.left == -1 && node.right == -1) {
            for (int i = 0; i < node.count; ++i) {
                int idx = node.start + i;
                const bvhTri& tri = primitives[idx];
                float t, u, v;
                if (intersectTriangle(r, tri, t, u, v)) {
                    if (t < nearestT) {
                        nearestT = t; nearestTri = idx; hit = true; hitu = u; hitv = v;
                    }
                }
            }
        } else {
            if (node.left != -1) stack.push_back(node.left);
            if (node.right != -1) stack.push_back(node.right);
        }
    }

    if (hit) { out_t = nearestT; out_triIdx = nearestTri; out_u = hitu; out_v = hitv; }
    return hit;
}
*/