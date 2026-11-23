// Sayyaratak scene building: static world meshes.
#include "game.h"

static const float SAND[4] = {0.80f, 0.66f, 0.45f, 1.f};
static const float SEA[4] = {0.06f, 0.22f, 0.38f, 1.f};

void buildScene() {
    if (G.world.roads.empty()) G.world.gen();
    // ground quad (textured)
    {
        MeshData m;
        float S = World::SIZE / 2;
        float W[4] = {1,1,1,1};
        quadN(m, v3(-S,0,-S), v3(S,0,-S), v3(S,0,S), v3(-S,0,S), v3(0,1,0), W, 1.f, 1.f);
        G.ground.upload(m);
    }
    // far ring (vertex colored sand->sea) radius 1200..3200
    {
        MeshData m;
        int seg = 48;
        for (int i = 0; i < seg; i++) {
            float a0 = i/(float)seg*6.2831853f, a1 = (i+1)/(float)seg*6.2831853f;
            float r0 = 1180, r1 = 3200;
            V3 p00 = v3(cosf(a0)*r0, -0.5f, sinf(a0)*r0), p01 = v3(cosf(a1)*r0, -0.5f, sinf(a1)*r0);
            V3 p10 = v3(cosf(a0)*r1, -0.5f, sinf(a0)*r1), p11 = v3(cosf(a1)*r1, -0.5f, sinf(a1)*r1);
            auto col = [&](V3 p, float c[4]) {
                if (p.x > World::SEA_X - 40) { c[0]=SEA[0]; c[1]=SEA[1]; c[2]=SEA[2]; c[3]=1; }
                else { c[0]=SAND[0]; c[1]=SAND[1]; c[2]=SAND[2]; c[3]=1; }
            };
            float c00[4], c01[4], c10[4], c11[4];
            col(p00,c00); col(p01,c01); col(p10,c10); col(p11,c11);
            V3 n = v3(0,1,0);
            vert(m, p00, n, 0,0, c00[0],c00[1],c00[2],1);
            vert(m, p01, n, 0,0, c01[0],c01[1],c01[2],1);
            vert(m, p11, n, 0,0, c11[0],c11[1],c11[2],1);
            vert(m, p00, n, 0,0, c00[0],c00[1],c00[2],1);
            vert(m, p11, n, 0,0, c11[0],c11[1],c11[2],1);
            vert(m, p10, n, 0,0, c10[0],c10[1],c10[2],1);
        }
        G.farRing.upload(m);
    }
    // sky dome (hemisphere, vertex colored)
    {
        MeshData m;
        float R = 2900;
        int seg = 24, rings = 6;
        for (int ri = 0; ri < rings; ri++) {
            float t0 = ri/(float)rings, t1 = (ri+1)/(float)rings;
            float e0 = t0 * 1.45f, e1 = t1 * 1.45f; // elevation angle
            for (int i = 0; i < seg; i++) {
                float a0 = i/(float)seg*6.2831853f, a1 = (i+1)/(float)seg*6.2831853f;
                auto pt = [&](float e, float a) {
                    return v3(cosf(a)*cosf(e)*R, sinf(e)*R, sinf(a)*cosf(e)*R);
                };
                auto col = [&](float e, float c[4]) {
                    float t = e / 1.45f;
                    c[0] = 0.94f*(1-t) + 0.20f*t;
                    c[1] = 0.78f*(1-t) + 0.40f*t;
                    c[2] = 0.60f*(1-t) + 0.72f*t;
                    c[3] = 1;
                };
                V3 p00=pt(e0,a0), p01=pt(e0,a1), p10=pt(e1,a0), p11=pt(e1,a1);
                float c0[4], c1[4]; col(e0,c0); col(e1,c1);
                V3 n = vnorm(vscale(pt((e0+e1)/2,(a0+a1)/2), -1.f));
                vert(m, p00, n, 0,0, c0[0],c0[1],c0[2],1);
                vert(m, p10, n, 0,0, c1[0],c1[1],c1[2],1);
                vert(m, p11, n, 0,0, c1[0],c1[1],c1[2],1);
                vert(m, p00, n, 0,0, c0[0],c0[1],c0[2],1);
                vert(m, p11, n, 0,0, c1[0],c1[1],c1[2],1);
                vert(m, p01, n, 0,0, c0[0],c0[1],c0[2],1);
            }
        }
        G.sky.upload(m);
    }
    // sun billboard + glow
    {
        MeshData m;
        float S = 260;
        float c1[4] = {1.f, 0.92f, 0.70f, 0.95f};
        quadN(m, v3(-S,-S,0), v3(S,-S,0), v3(S,S,0), v3(-S,S,0), v3(0,0,1), c1);
        G.sunMesh.upload(m);
    }
    // clouds (billboards merged, drawn with particle tex)
    {
        MeshData m;
        unsigned r = 99;
        for (int i = 0; i < 10; i++) {
            r = r*1664525u+1013904223u; float x = ((r>>8)&0xFFFF)/65536.f*2400-1200;
            r = r*1664525u+1013904223u; float z = ((r>>8)&0xFFFF)/65536.f*2400-1200;
            r = r*1664525u+1013904223u; float y = 180 + ((r>>8)&0xFFFF)/65536.f*120;
            float s = 90 + (i%4)*35;
            float c[4] = {1,1,1,0.34f};
            quadN(m, v3(x-s,y,z-s), v3(x+s,y,z-s), v3(x+s,y,z+s), v3(x-s,y,z+s), v3(0,1,0), c);
        }
        G.cloudMesh.upload(m);
    }
    // mountains ring + Sabir behind Taiz
    {
        MeshData m;
        unsigned r = 55;
        for (int i = 0; i < 16; i++) {
            float a = i/16.f*6.2831853f;
            r = r*1664525u+1013904223u;
            float rad = 1250 + ((r>>8)&0xFFFF)/65536.f*350;
            float x = cosf(a)*rad, z = sinf(a)*rad;
            if (x > World::SEA_X - 100) continue; // not in the sea
            float h = 140 + ((r>>4)&0xFFFF)/65536.f*180;
            float base = 180 + (i%3)*70;
            float rc[4] = {0.52f+0.05f*(i%3), 0.42f, 0.34f, 1.f};
            cone(m, x, -2, z, base, h, 8, rc);
        }
        float sb[4] = {0.45f, 0.38f, 0.33f, 1.f};
        cone(m, -1250, -2, -900, 420, 330, 9, sb); // Jabal Sabir
        G.mountains.upload(m);
    }
    // Hadramawt canyon walls
    {
        MeshData m;
        float rc[4] = {0.62f, 0.44f, 0.26f, 1.f};
        float rd[4] = {0.55f, 0.38f, 0.22f, 1.f};
        for (int k = -2; k <= 2; k++) {
            box(m, 0 + k*95, 26, -800 - 175, 100, 52, 60, k%2 ? rc : rd);
            box(m, 0 + k*95, 22, -800 + 175, 100, 44, 60, k%2 ? rd : rc);
        }
        G.canyon.upload(m);
        for (int k = -2; k <= 2; k++) {
            G.world.colliders.push_back({k*95.f, -975.f, 50, 30});
            G.world.colliders.push_back({k*95.f, -625.f, 50, 30});
        }
    }
    // buildings per texture
    {
        MeshData mm[4];
        for (auto& b : G.world.bldgs) {
            MeshData& m = mm[b.tex];
            float tintv = 0.9f + 0.1f * ((int)(b.x*7+b.z*13) & 3) / 3.f;
            float col[4] = {tintv, tintv, tintv, 1};
            // base plinth darker
            float dk[4] = {tintv*0.72f, tintv*0.72f, tintv*0.72f, 1};
            box(m, b.x, b.h/2, b.z, b.w, b.h, b.d, col, b.w/6.f, b.h/6.f);
            box(m, b.x, 0.5f, b.z, b.w+0.6f, 1.f, b.d+0.6f, dk, 2, 1);
            // roof slab
            box(m, b.x, b.h + 0.3f, b.z, b.w + 0.4f, 0.6f, b.d + 0.4f, dk, 1, 1);
        }
        for (int i = 0; i < 4; i++) G.bldgMesh[i].upload(mm[i]);
    }
    // palms: trunk cylinder + 2 crossed frond quads (textured, alpha test)
    {
        MeshData m;
        float TC[4] = {0.42f, 0.30f, 0.16f, 1.f};
        float W[4] = {1,1,1,1};
        for (auto& p : G.world.palms) {
            float h = 5.5f * p.s;
            cylY(m, p.x, h/2, p.z, 0.16f*p.s, h, 5, TC);
            float w = 2.6f * p.s;
            float wl = w * 0.55f;
            quadN(m, v3(p.x-wl, h, p.z-wl), v3(p.x+wl, h, p.z-wl), v3(p.x+wl, h, p.z+wl), v3(p.x-wl, h, p.z+wl), v3(0,1,0), W);
            quadN(m, v3(p.x-w, h*0.55f, p.z), v3(p.x, h, p.z), v3(p.x+w, h*0.55f, p.z), v3(p.x-w, h*0.55f, p.z), v3(0,0,1), W);
            quadN(m, v3(p.x-w, h*0.55f, p.z), v3(p.x+w, h*0.55f, p.z), v3(p.x, h, p.z), v3(p.x-w, h*0.55f, p.z), v3(0,0,1), W);
            quadN(m, v3(p.x, h*0.55f, p.z-w), v3(p.x, h, p.z), v3(p.x, h*0.55f, p.z+w), v3(p.x, h*0.55f, p.z-w), v3(1,0,0), W);
            quadN(m, v3(p.x, h*0.55f, p.z-w), v3(p.x, h*0.55f, p.z+w), v3(p.x, h, p.z+w), v3(p.x, h*0.55f, p.z-w), v3(1,0,0), W);
        }
        G.palms.upload(m);
    }
    // stars (one star mesh, drawn many times)
    {
        MeshData m;
        float Gc[4] = {1.0f, 0.80f, 0.25f, 1.f};
        star(m, 1.1f, 0.48f, 0.3f, Gc);
        G.starsMesh.upload(m);
    }
    // checkpoint ring + beacon
    {
        MeshData m;
        float Gc[4] = {1.0f, 0.75f, 0.2f, 0.95f};
        torus(m, 4.2f, 0.4f, 20, 8, Gc);
        G.ringMesh.upload(m);
        MeshData b;
        float BC[4] = {1.0f, 0.8f, 0.3f, 0.16f};
        cylY(b, 0, 60, 0, 1.4f, 120, 8, BC, false);
        G.beaconMesh.upload(b);
    }
    // blob shadow quad
    {
        MeshData m;
        float c[4] = {0,0,0,0.38f};
        quadN(m, v3(-1,0,-1), v3(1,0,-1), v3(1,0,1), v3(-1,0,1), v3(0,1,0), c);
        G.shadowMesh.upload(m);
    }
    // parked cars: 6 color bodies + shared glass
    {
        for (int ci = 0; ci < 6; ci++) {
            CarParts cp = buildCar(PAINTS[ci], 0, false, false, true);
            G.parkedBody[ci].upload(cp.body);
            // append trim+lights+spoiler-less parts into body draw: keep simple, draw trim via own mesh merged
            if (ci == 0) {
                G.parkedGlass.upload(cp.glass);
            }
        }
    }
    // particles dynamic
    G.particles.uploadDynamic(256 * 6);
    G.sceneBuilt = true;
}

void buildCarGL() {
    CarParts cp = buildCar(G.paint, G.wheels, G.spoiler, G.neon, G.tint);
    G.carBody.upload(cp.body);
    G.carGlass.upload(cp.glass);
    G.carTrim.upload(cp.trim);
    G.carLights.upload(cp.lights);
    G.carTire.upload(cp.tire);
    G.carRim.upload(cp.rim);
    G.carSpoiler.upload(cp.spoiler);
    G.carNeon.upload(cp.neon);
    G.carDirty = false;
}
