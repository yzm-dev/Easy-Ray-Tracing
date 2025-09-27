#version 420 core

out vec4 fragColor;

// Scene triangles packed in a texture buffer. Each triangle uses 5 RGBA32F texels:
// 0: v0.xyz, w unused
// 1: v1.xyz, w unused
// 2: v2.xyz, w unused
// 3: albedo.rgb, w unused
// 4: emission.rgb, w unused

uniform samplerBuffer uTriangles;
uniform int uTriangleCount;

// BVH nodes packed as 3 texels per node:
// texel0: bmin.xyz, w = left index (float bits)
// texel1: bmax.xyz, w = right index (float bits)
// texel2: start, count, unused, unused
uniform samplerBuffer uBVHNodes;

// Camera
uniform vec3 uCamPos;
uniform mat4 uInvViewProj;
uniform vec2 uResolution;

// Optional top-level scene bounds for early-out
uniform vec3 uSceneMin;
uniform vec3 uSceneMax;

// Integrator params
uniform int uSpp;
uniform int uMaxDepth;
uniform int uFrame; // varies per frame for RNG decorrelation

// Hash-based RNG
uint hash(uvec3 x) {
    x = x * 1664525u + 1013904223u;
    x.x += x.y * x.z; x.y += x.z * x.x; x.z += x.x * x.y;
    x ^= x >> 16u;
    x += x << 3u;
    x ^= x >> 4u;
    x *= 0x27d4eb2du;
    x ^= x >> 15u;
    return x.x;
}

float rand(inout uvec3 state) {
    state = uvec3(hash(state), hash(state.yxz), hash(state.zxy));
    return float(state.x) / 4294967296.0;
}

struct Ray { 
    vec3 o; 
    vec3 d; 
    float tMin; 
    float tMax; 
};

struct AABB { 
    vec3 bmin; 
    vec3 bmax; 
};

AABB aabb_make(vec3 mn, vec3 mx) { 
    AABB b; 
    b.bmin = mn; 
    b.bmax = mx; 
    return b; 
}

bool intersectAABB(Ray r, AABB b) {
    vec3 invD = 1.0 / r.d;
    vec3 t0s = (b.bmin - r.o) * invD;
    vec3 t1s = (b.bmax - r.o) * invD;
    vec3 tsm = min(t0s, t1s);
    vec3 tsM = max(t0s, t1s);
    float t_enter = max(r.tMin, max(tsm.x, max(tsm.y, tsm.z)));
    float t_exit = min(r.tMax, min(tsM.x, min(tsM.y, tsM.z)));
    return t_exit >= t_enter && t_exit > 0;
}

struct TriangleData {
    vec3 v0; 
    vec3 v1; 
    vec3 v2; 
    vec3 albedo; 
    vec3 emission; 
};

TriangleData getTriangle(int i) {
    int base = i * 5;
    vec4 t0 = texelFetch(uTriangles, base + 0);
    vec4 t1 = texelFetch(uTriangles, base + 1);
    vec4 t2 = texelFetch(uTriangles, base + 2);
    vec4 t3 = texelFetch(uTriangles, base + 3);
    vec4 t4 = texelFetch(uTriangles, base + 4);
    TriangleData T;
    T.v0 = t0.xyz; T.v1 = t1.xyz; T.v2 = t2.xyz; T.albedo = t3.rgb; T.emission = t4.rgb;
    return T;
}

bool intersectTriangle(Ray ray, TriangleData T, out float t, out float u, out float v) {
    vec3 e1 = T.v1 - T.v0;
    vec3 e2 = T.v2 - T.v0;
    vec3 p = cross(ray.d, e2);
    float det = dot(e1, p);
    if (abs(det) < 1e-8) return false;
    float invDet = 1.0 / det;
    vec3 tvec = ray.o - T.v0;
    u = dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0) return false;
    vec3 q = cross(tvec, e1);
    v = dot(ray.d, q) * invDet; 
    if (v < 0.0 || (u + v) > 1.0) return false;
    t = dot(e2, q) * invDet;
    return t > ray.tMin && t < ray.tMax;
}

// BVH accessors
struct BVHNode { 
    AABB b; 
    int left; 
    int right; 
    int start; 
    int count; 
};

BVHNode getBVHNode(int idx) {
    int base = idx * 3;
    vec4 a = texelFetch(uBVHNodes, base + 0);
    vec4 b = texelFetch(uBVHNodes, base + 1);
    vec4 c = texelFetch(uBVHNodes, base + 2);
    BVHNode n;
    n.b.bmin = a.xyz;
    n.left = int(a.w);
    n.b.bmax = b.xyz;
    n.right = int(b.w);
    n.start = int(c.x);
    n.count = int(c.y);
    return n;
}

#define MAX_BVH_NODES 64
bool intersectSceneBVH(Ray ray, out float tHit, out int triIdx, out float outU, out float outV) {
    tHit = ray.tMax; 
    triIdx = -1; 
    outU = 0.0; 
    outV = 0.0;
    // root node
    int stack[MAX_BVH_NODES]; 
    int sp = 0; 
    stack[sp++] = 0; 
    while (sp > 0) {
        int ni = stack[--sp];
        BVHNode node = getBVHNode(ni);
        if (node.count == 0 && node.left == -1 && node.right == -1) continue; // empty
        Ray r = ray; 
        r.tMax = tHit;
        if (!intersectAABB(r, node.b)) continue;
        if (node.left == -1 && node.right == -1) { // leaf node
            for (int i = 0; i < node.count; ++i) {
                int idx = node.start + i;
                TriangleData T = getTriangle(idx);
                float tu, tv, tt;
                if (intersectTriangle(ray, T, tt, tu, tv)) {
                    if (tt < tHit) { 
                        tHit = tt; 
                        triIdx = idx; 
                        outU = tu; 
                        outV = tv; 
                    }
                }
            }
        } else {
            if (node.left != -1) stack[sp++] = node.left;
            if (node.right != -1) stack[sp++] = node.right;
        }
    }
    return triIdx >= 0;
}

// Cosine-weighted hemisphere sampling
vec3 cosineSampleHemisphere(float u1, float u2) {
    float r = sqrt(u1);
    float theta = 6.2831853 * u2;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u1));
    return vec3(x, y, z);
}

mat3 basisFromNormal(vec3 n) {
    vec3 w = normalize(n);
    vec3 a = abs(w.z) < 0.999 ? vec3(0,0,1) : vec3(0,1,0);
    vec3 v = normalize(cross(w, a));
    vec3 u = cross(v, w);
    return mat3(u, v, w);
}

// Sample emissive triangles uniformly
bool sampleLight(inout uvec3 rng, out vec3 pos, out vec3 n, out vec3 Le, out float pdf) {
    int M = 0;
    for (int i = 0; i < uTriangleCount; ++i) {
        TriangleData T = getTriangle(i);
        if (max(max(T.emission.r, T.emission.g), T.emission.b) > 0.0) M++;
    }
    if (M == 0) { pdf = 0.0; return false; }
    int pick = int(floor(rand(rng) * float(M))); pick = clamp(pick, 0, M - 1);
    int idx = -1; 
    int c = 0;
    for (int i = 0; i < uTriangleCount; ++i) {
        TriangleData T = getTriangle(i);
        if (max(max(T.emission.r, T.emission.g), T.emission.b) > 0.0) { 
            if (c == pick) { 
                idx = i; 
                break; 
            } 
            c++;
        }
    }
    if (idx < 0) { pdf = 0.0; return false; }
    TriangleData T = getTriangle(idx);
    float r1 = rand(rng); float r2 = rand(rng);
    float su1 = sqrt(r1);
    float b0 = 1.0 - su1;
    float b1 = r2 * su1;
    float b2 = 1.0 - b0 - b1;
    pos = T.v0 * b0 + T.v1 * b1 + T.v2 * b2;
    n = normalize(cross(T.v1 - T.v0, T.v2 - T.v0));
    Le = T.emission;
    float area = length(cross(T.v1 - T.v0, T.v2 - T.v0)) * 0.5;
    pdf = (1.0 / float(M)) * (1.0 / area);
    return true;
}

// Ray generation via inverse view-projection
vec3 generateRayDir(vec2 pixelUV, inout uvec3 rng) {
    vec2 jitter = vec2(rand(rng), rand(rng));
    vec2 ndc = ((pixelUV + jitter) / uResolution) * 2.0 - 1.0; // [-1,1]
    // openGL nearP.z = -1, Vulkan / DirectX / Metal nearP.z = 0
    vec4 nearP = uInvViewProj * vec4(ndc, -1.0, 1.0); 
    nearP /= nearP.w;
    vec4 farP  = uInvViewProj * vec4(ndc, 1.0, 1.0);
    farP /= farP.w;
    vec3 dir = normalize(farP.xyz - nearP.xyz);
    return dir;
}

void main() {
    uvec3 rng = uvec3(uint(gl_FragCoord.x) + 4096u * uint(gl_FragCoord.y), uint(uFrame), 1234567u);
    vec3 col = vec3(0.0);

    for (int s = 0; s < uSpp; ++s) {
        vec3 ro = uCamPos;
        vec3 rd = generateRayDir(gl_FragCoord.xy, rng);

        vec3 throughput = vec3(1.0);
        vec3 L = vec3(0.0);

        Ray ray; 
        ray.o = ro; 
        ray.d = rd; 
        ray.tMin = 1e-4; 
        ray.tMax = 1e30;

        // Early out if outside scene AABB
        AABB sceneB = aabb_make(uSceneMin, uSceneMax);
        if (!intersectAABB(ray, sceneB)) {
            col += vec3(0.0);
            continue;
        }

        for (int depth = 0; depth < uMaxDepth; ++depth) {
            float tHit, u, v; 
            int triIdx;
            if (!intersectSceneBVH(ray, tHit, triIdx, u, v)) {
                break; // background black
            }
            TriangleData T = getTriangle(triIdx);
            vec3 hit = ray.o + ray.d * tHit;
            vec3 N = normalize(cross(T.v1 - T.v0, T.v2 - T.v0));
            if (dot(N, ray.d) > 0.0) N = -N;

            // Emission on hit
            if (max(max(T.emission.r, T.emission.g), T.emission.b) > 0.0) {
                L += throughput * T.emission;
                break;
            }

            // Next Event Estimation (direct light)
            vec3 lp, ln, Le; float pdfL;
            if (sampleLight(rng, lp, ln, Le, pdfL) && pdfL > 0.0) {
                vec3 toL = lp - hit;
                float dist2 = dot(toL, toL);
                float dist = sqrt(dist2);
                vec3 wi = toL / dist;
                float cosS = max(0.0, dot(N, wi));
                float cosL = max(0.0, dot(ln, -wi));

                // Shadow ray
                float tBlock, uu, vv; int idx;
                bool blocked = false;
                vec3 shadowO = hit + N * 1e-3;
                Ray shadowRay; 
                shadowRay.o = shadowO; 
                shadowRay.d = wi; 
                shadowRay.tMin = 1e-4; 
                shadowRay.tMax = dist - 1e-3;
                if (intersectSceneBVH(shadowRay, tBlock, idx, uu, vv)) blocked = true;
                if (!blocked && cosS > 0.0 && cosL > 0.0) {
                    vec3 brdf = T.albedo / 3.14159265;
                    float G = (cosS * cosL) / dist2;
                    L += throughput * Le * brdf * G / pdfL;
                }
            }

            // Sample diffuse bounce
            vec3 local = cosineSampleHemisphere(rand(rng), rand(rng));
            mat3 onb = basisFromNormal(N);
            vec3 newDir = normalize(onb * local);

            // lambertian with MIS=off
            float cosI     = max(0.0, dot(N, newDir));
            float pdfBSDF  = cosI / 3.14159265;
            vec3  brdf     = T.albedo / 3.14159265;
            throughput *= brdf * cosI / pdfBSDF; // Óë throughput *= T.albedo µÈ¼Û

            // Russian roulette
            float p = max(throughput.r, max(throughput.g, throughput.b));
            p = clamp(p, 0.1, 0.95);
            if (rand(rng) > p) break;
            throughput /= p;

            ray.o = hit + N * 1e-3; 
            ray.d = newDir; 
            ray.tMin = 1e-4; 
            ray.tMax = 1e30;
        }
        col += L;
    }

    // Average over samples
    col /= float(uSpp);

    // Reinhard tone mapping + gamma correction
    col = col / (vec3(1.0) + col);
    col = pow(col, vec3(1.0/2.2));

    fragColor = vec4(col, 1.0);
}