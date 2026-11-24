// Sayyaratak world: districts, roads, buildings, colliders, export for ground texture.
#pragma once
#include "mesh.h"
#include <vector>
#include <cstdlib>

struct RoadSeg { float x1, z1, x2, z2, w, type; };
struct TintRect { float x, z, w, h, style; };
struct Bldg { float x, z, w, d, h; int tex; };
struct Collider { float x, z, hw, hd; };
struct Palm { float x, z, s; };
struct TrafficCar { int seg, dir, colorIdx; float t, speed, cruise, x, z, heading, wheelSpin; };
struct Parked { float x, z, heading; int colorIdx; };
struct District { float cx, cz; int style; }; // style: 1 taiz, 2 sanaa, 3 ibb, 4 aden, 5 hadramawt

struct World {
    std::vector<RoadSeg> roads;
    std::vector<TintRect> tints;
    std::vector<Bldg> bldgs;
    std::vector<Collider> colliders;
    std::vector<Palm> palms;
    std::vector<TrafficCar> traffic;
    std::vector<Parked> parked;
    std::vector<V3> stars;
    std::vector<V3> intersections;
    District districts[6]; // 0 hub + 5 cities
    unsigned rng = 2026;
    static constexpr float SIZE = 2400.f;
    static constexpr float SEA_X = 950.f;

    float frand() { rng = rng * 1664525u + 1013904223u; return ((rng >> 8) & 0xFFFFFF) / 16777216.f; }
    float range(float a, float b) { return a + frand() * (b - a); }

    void gen() {
        districts[0] = {0, 0, 0};
        districts[1] = {-700, -420, 1};
        districts[2] = {700, -420, 2};
        districts[3] = {-700, 420, 3};
        districts[4] = {700, 420, 4};
        districts[5] = {0, -800, 5};
        // district tints
        for (int i = 1; i <= 5; i++) {
            int ts = districts[i].style == 3 ? 1 : (districts[i].style == 5 ? 2 : (districts[i].style == 4 ? 3 : (districts[i].style == 2 ? 4 : 5)));
            tints.push_back({districts[i].cx, districts[i].cz, 460, 460, (float)ts});
        }
        // hub plaza
        tints.push_back({0, 0, 260, 260, 3});
        // streets per district: 4x4 grid over 420m
        for (int i = 1; i <= 5; i++) {
            float cx = districts[i].cx, cz = districts[i].cz;
            if (districts[i].style == 5) { // Hadramawt: canyon, single main street + cross
                roads.push_back({cx - 200, cz, cx + 200, cz, 14, 1});
                roads.push_back({cx, cz - 150, cx, cz + 150, 12, 0});
                intersections.push_back(v3(cx, 0, cz));
            } else {
                for (int k = -1; k <= 1; k++) {
                    roads.push_back({cx - 210, cz + k * 140.f, cx + 210, cz + k * 140.f, 12, 0});
                    roads.push_back({cx + k * 140.f, cz - 210, cx + k * 140.f, cz + 210, 12, 0});
                }
                for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++)
                    intersections.push_back(v3(cx + a * 140.f, 0, cz + b * 140.f));
            }
        }
        // highways hub <-> districts
        for (int i = 1; i <= 5; i++) {
            float dx = districts[i].cx, dz = districts[i].cz;
            float len = sqrtf(dx*dx + dz*dz);
            float ux = dx / len, uz = dz / len;
            roads.push_back({ux * 130, uz * 130, dx - ux * 215, dz - uz * 215, 18, 1});
        }
        // hub ring road (octagon)
        for (int i = 0; i < 8; i++) {
            float a0 = i / 8.f * 6.2831853f, a1 = (i + 1) / 8.f * 6.2831853f;
            roads.push_back({cosf(a0) * 120, sinf(a0) * 120, cosf(a1) * 120, sinf(a1) * 120, 14, 1});
        }
        // corniche (Aden coastal road)
        roads.push_back({880, 80, 880, 900, 14, 1});
        roads.push_back({700, 630, 880, 630, 12, 0});
        // buildings
        for (int i = 1; i <= 5; i++) genDistrictBuildings(i);
        for (int i = 1; i <= 5; i++) genStreetWalls(i);
        // palms along streets + Ibb jungle
        for (int i = 1; i <= 5; i++) {
            float cx = districts[i].cx, cz = districts[i].cz;
            int n = districts[i].style == 3 ? 120 : 36;
            for (int k = 0; k < n; k++)
                palms.push_back({cx + range(-215, 215), cz + range(-215, 215), range(0.8f, 1.3f)});
        }
        for (int k = 0; k < 26; k++) palms.push_back({range(-110, 110), range(-110, 110), range(0.9f, 1.3f)});
        // remove palms on roads
        std::vector<Palm> kept;
        for (auto& p : palms) if (!onRoad(p.x, p.z, 3.f)) kept.push_back(p);
        palms = kept;
        // parked cars
        for (int i = 1; i <= 5; i++) {
            for (int k = 0; k < 4; k++) {
                float cx = districts[i].cx + range(-140, 140);
                float cz = districts[i].cz + (k % 2 ? 140 : -140) + range(-4, 4);
                parked.push_back({cx, cz + 4.5f, 0.f, (int)(frand() * 6)});
                colliders.push_back({cx, cz + 4.5f, 1.3f, 2.4f});
            }
        }
        // stars on intersections
        for (auto& it : intersections) if (frand() < 0.65f) stars.push_back(v3(it.x, 1.6f, it.z));
        // moving traffic
        for (int k = 0; k < 20 && !roads.empty(); k++) {
            int seg = (int)(frand() * roads.size()) % (int)roads.size();
            RoadSeg& r = roads[seg];
            float dx = r.x2 - r.x1, dz = r.z2 - r.z1;
            float len = sqrtf(dx * dx + dz * dz);
            if (len < 60) { k--; if (k < -40) break; continue; }
            TrafficCar tc;
            tc.seg = seg; tc.dir = frand() < 0.5f ? 1 : -1;
            tc.t = frand(); tc.cruise = 7.f + frand() * 7.f; tc.speed = tc.cruise;
            tc.colorIdx = (int)(frand() * 6);
            tc.x = tc.z = tc.heading = tc.wheelSpin = 0;
            traffic.push_back(tc);
        }
    }

    // advance traffic one step; px/pz = player position for braking
    void updateTraffic(float dt, float px, float pz) {
        for (auto& tc : traffic) {
            RoadSeg& r = roads[tc.seg];
            float dx = r.x2 - r.x1, dz = r.z2 - r.z1;
            float len = sqrtf(dx * dx + dz * dz);
            if (len < 1) continue;
            float ux = dx / len, uz = dz / len;
            // brake when the player is close ahead
            float fx = ux * tc.dir, fz = uz * tc.dir;
            float pdx = px - tc.x, pdz = pz - tc.z;
            float pd = sqrtf(pdx * pdx + pdz * pdz);
            float target = tc.cruise;
            if (pd < 14.f && (pdx * fx + pdz * fz) > 0) target = pd < 7.f ? 0.f : tc.cruise * 0.3f;
            tc.speed += (target - tc.speed) * fminf(1.f, dt * 2.2f);
            tc.t += tc.dir * tc.speed * dt / len;
            if (tc.t > 1.f || tc.t < 0.f) {
                float ex = tc.t > 1.f ? r.x2 : r.x1, ez = tc.t > 1.f ? r.z2 : r.z1;
                int best = -1; float bestScore = 1e9f;
                for (int i = 0; i < (int)roads.size(); i++) {
                    if (i == tc.seg) continue;
                    RoadSeg& o = roads[i];
                    float ddx = o.x2 - o.x1, ddz = o.z2 - o.z1;
                    float ol = sqrtf(ddx * ddx + ddz * ddz);
                    if (ol < 30) continue;
                    for (int e = 0; e < 2; e++) {
                        float ox = e ? o.x2 : o.x1, oz = e ? o.z2 : o.z1;
                        float dd = (ox - ex) * (ox - ex) + (oz - ez) * (oz - ez);
                        if (dd < 9.f) {
                            // prefer continuing roughly straight
                            float dot = (ddx / ol) * fx * (e ? 1 : -1) + (ddz / ol) * fz * (e ? 1 : -1);
                            float score = -dot + frand() * 0.35f;
                            if (score < bestScore) { bestScore = score; best = i * 2 + e; }
                        }
                    }
                }
                if (best >= 0) {
                    tc.seg = best / 2; tc.t = (best % 2) ? 1.f : 0.f; tc.dir = 1;
                } else {
                    tc.dir = -tc.dir;
                    tc.t = tc.t > 1.f ? 1.f : 0.f;
                }
            } else {
                // lane offset: right-hand side relative to travel direction
                float lane = 3.1f * tc.dir;
                tc.x = r.x1 + dx * tc.t - uz * lane;
                tc.z = r.z1 + dz * tc.t + ux * lane;
                tc.heading = atan2f(fx, fz);
                tc.wheelSpin += tc.speed * dt / 0.34f;
            }
        }
    }

    void genDistrictBuildings(int di) {
        float cx = districts[di].cx, cz = districts[di].cz;
        int style = districts[di].style;
        if (style == 5) { // Shibam-style mud tower cluster
            for (int k = 0; k < 40; k++) {
                float x = cx + range(-190, 190), z = cz + range(-140, 140);
                if (onRoad(x, z, 10.f)) continue;
                float w = range(10, 16), d = range(10, 16), h = range(22, 42);
                bldgs.push_back({x, z, w, d, h, 2});
                colliders.push_back({x, z, w / 2, d / 2});
            }
            return;
        }
        // opencity (MIT) ideas: downtown height falloff toward the district
        // center, and the occasional empty lot.
        for (int gx = -1; gx <= 1; gx++) for (int gz = -1; gz <= 1; gz++) {
            // block center between streets
            float bx = cx + gx * 140.f + 70.f * (gx == 1 ? 1 : (gx == -1 ? -1 : 0)) * 0.0f;
            float bz = cz + gz * 140.f;
            if (frand() < 0.10f) continue; // empty lot
            float ddx = bx - cx, ddz = bz - cz;
            float downtown = 1.f - fminf(1.f, sqrtf(ddx*ddx + ddz*ddz) / 230.f);
            int n = 1 + (int)(frand() * 2.2f);
            for (int k = 0; k < n; k++) {
                float x = bx + range(-44, 44), z = bz + range(-44, 44);
                if (onRoad(x, z, 16.f)) continue;
                float w, d, h; int tex;
                if (style == 2) { w = range(10, 15); d = range(10, 15); h = range(18, 36) + downtown * range(6, 20); tex = 1; }
                else if (style == 4) { w = range(12, 22); d = range(12, 22); h = range(8, 18) + downtown * range(3, 8); tex = 3; }
                else if (style == 3) { w = range(10, 18); d = range(10, 18); h = range(6, 12) + downtown * range(2, 5); tex = 0; }
                else { w = range(12, 24); d = range(12, 24); h = range(10, 26) + downtown * range(5, 14); tex = 0; }
                bldgs.push_back({x, z, w, d, h, tex});
                colliders.push_back({x, z, w / 2, d / 2});
            }
        }
        // outskirts ring: lower skyline around each city
        for (int k = 0; k < 18; k++) {
            float a = k / 18.f * 6.2831853f + range(-0.1f, 0.1f);
            float rr = range(275.f, 380.f);
            float x = cx + cosf(a) * rr, z = cz + sinf(a) * rr;
            if (onRoad(x, z, 14.f)) continue;
            float w = range(10, 20), d = range(10, 20), h = range(7, style == 2 ? 24 : 14);
            int tex = style == 2 ? 1 : (style == 4 ? 3 : 0);
            bldgs.push_back({x, z, w, d, h, tex});
            colliders.push_back({x, z, w / 2, d / 2});
        }
    }

    void genStreetWalls(int di) {
        float cx = districts[di].cx, cz = districts[di].cz;
        int style = districts[di].style;
        if (style == 5) return;
        for (auto& r : roads) {
            float mx = (r.x1 + r.x2) / 2 - cx, mz = (r.z1 + r.z2) / 2 - cz;
            if (mx * mx + mz * mz > 240 * 240) continue;
            float dx = r.x2 - r.x1, dz = r.z2 - r.z1;
            float len = sqrtf(dx * dx + dz * dz);
            if (len < 30) continue;
            float ux = dx / len, uz = dz / len;
            float nx = -uz, nz = ux;
            for (float t = 18; t < len - 18; t += 27) {
                for (int side = -1; side <= 1; side += 2) {
                    if (frand() < 0.22f) continue;
                    float off = r.w / 2 + 9 + range(0, 7);
                    float x = r.x1 + ux * t + nx * off * side;
                    float z = r.z1 + uz * t + nz * off * side;
                    float ddx = x - cx, ddz = z - cz;
                    if (ddx * ddx + ddz * ddz > 250 * 250) continue;
                    if (onRoad(x, z, 11.f)) continue;
                    float w = range(10, 18), d = range(10, 18), h; int tex;
                    if (style == 2) { h = range(16, 34); tex = 1; }
                    else if (style == 4) { h = range(8, 16); tex = 3; }
                    else if (style == 3) { h = range(6, 12); tex = 0; }
                    else { h = range(10, 24); tex = 0; }
                    bldgs.push_back({x, z, w, d, h, tex});
                    colliders.push_back({x, z, w / 2, d / 2});
                }
            }
        }
    }

    bool onRoad(float x, float z, float margin) const {
        for (auto& r : roads) {
            float dx = r.x2 - r.x1, dz = r.z2 - r.z1;
            float l2 = dx * dx + dz * dz;
            float t = l2 > 0 ? ((x - r.x1) * dx + (z - r.z1) * dz) / l2 : 0;
            t = t < 0 ? 0 : (t > 1 ? 1 : t);
            float px = r.x1 + t * dx - x, pz = r.z1 + t * dz - z;
            float hw = r.w / 2 + margin;
            if (px * px + pz * pz < hw * hw) return true;
        }
        // hub plaza
        if (x * x + z * z < 125 * 125) return true;
        return false;
    }

    int districtAt(float x, float z) const {
        if (x * x + z * z < 200 * 200) return 0;
        for (int i = 1; i <= 5; i++) {
            float dx = x - districts[i].cx, dz = z - districts[i].cz;
            if (dx * dx + dz * dz < 260 * 260) return i;
        }
        return 6;
    }

    V3 spawnFor(int d) const {
        if (d >= 1 && d <= 5) return v3(districts[d].cx, 0, districts[d].cz + 140.f);
        return v3(0, 0, 30);
    }

    std::vector<float> exportData() const {
        std::vector<float> o;
        o.push_back(SIZE); o.push_back(SEA_X);
        o.push_back((float)roads.size());
        for (auto& r : roads) o.insert(o.end(), {r.x1, r.z1, r.x2, r.z2, r.w, r.type});
        o.push_back((float)tints.size());
        for (auto& t : tints) o.insert(o.end(), {t.x, t.z, t.w, t.h, t.style});
        o.push_back((float)bldgs.size());
        for (auto& b : bldgs) o.insert(o.end(), {b.x, b.z, b.w, b.d});
        o.push_back((float)palms.size());
        for (auto& p : palms) o.insert(o.end(), {p.x, p.z, p.s});
        return o;
    }
};
