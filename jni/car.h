// Sayyaratak car builder: detailed procedural sports sedan.
#pragma once
#include "mesh.h"

struct CarParts {
    MeshData body;    // painted
    MeshData glass;   // windows
    MeshData trim;    // dark bumpers, grille, skirts
    MeshData lights;  // emissive front/rear
    MeshData tire;    // one wheel tire (drawn 4x)
    MeshData rim;     // one wheel rim (drawn 4x)
    MeshData spoiler; // painted
    MeshData neon;    // underglow quad
    float wheelX = 0.86f, wheelY = 0.34f, wheelZF = 1.38f, wheelZR = -1.38f, wheelR = 0.34f;
};

static const float PAINTS[8][3] = {
    {0.62f,0.05f,0.06f},{0.08f,0.14f,0.38f},{0.92f,0.90f,0.86f},{0.06f,0.06f,0.08f},
    {0.80f,0.62f,0.22f},{0.05f,0.38f,0.20f},{0.90f,0.42f,0.08f},{0.35f,0.14f,0.50f}
};

inline CarParts buildCar(const float paint[3], int wheelStyle, bool spoilerOn, bool neonOn, bool tintOn) {
    CarParts c;
    float P[4] = {paint[0], paint[1], paint[2], 1.f};
    float DK[4] = {0.05f, 0.05f, 0.06f, 1.f};
    float GL[4] = {0.10f, 0.13f, 0.18f, tintOn ? 0.92f : 0.55f};
    float HL[4] = {1.0f, 0.95f, 0.8f, 1.f};
    float TL[4] = {0.9f, 0.08f, 0.08f, 1.f};
    float SL[4] = {0.75f, 0.75f, 0.78f, 1.f};

    // main body: lower box
    box(c.body, 0, 0.52f, 0, 1.84f, 0.44f, 4.5f, P, 1.f, 1.f);
    // hood: sloped front (forward is +z)
    hex8(c.body,
        v3(-0.86f, 0.55f, 2.28f), v3(0.86f, 0.55f, 2.28f), v3(0.80f, 0.55f, 1.05f), v3(-0.80f, 0.55f, 1.05f),
        v3(-0.86f, 0.62f, 2.28f), v3(0.86f, 0.62f, 2.28f), v3(0.80f, 0.80f, 1.05f), v3(-0.80f, 0.80f, 1.05f), P);
    // trunk: sloped rear
    hex8(c.body,
        v3(-0.80f, 0.55f, -1.05f), v3(0.80f, 0.55f, -1.05f), v3(0.88f, 0.55f, -2.28f), v3(-0.88f, 0.55f, -2.28f),
        v3(-0.80f, 0.80f, -1.05f), v3(0.80f, 0.80f, -1.05f), v3(0.88f, 0.78f, -2.28f), v3(-0.88f, 0.78f, -2.28f), P);
    // cabin (greenhouse), tapered
    hex8(c.body,
        v3(-0.78f, 0.78f, 1.00f), v3(0.78f, 0.78f, 1.00f), v3(0.78f, 0.78f, -1.10f), v3(-0.78f, 0.78f, -1.10f),
        v3(-0.60f, 1.22f, 0.45f), v3(0.60f, 1.22f, 0.45f), v3(0.60f, 1.22f, -0.75f), v3(-0.60f, 1.22f, -0.75f), P);
    // windshield / backlight / side glass (slightly proud of cabin)
    hex8(c.glass,
        v3(-0.79f, 0.80f, 1.01f), v3(0.79f, 0.80f, 1.01f), v3(0.79f, 0.80f, -1.11f), v3(-0.79f, 0.80f, -1.11f),
        v3(-0.61f, 1.23f, 0.46f), v3(0.61f, 1.23f, 0.46f), v3(0.61f, 1.23f, -0.76f), v3(-0.61f, 1.23f, -0.76f), GL);
    // roof cap (painted)
    box(c.body, 0, 1.235f, -0.15f, 1.22f, 0.03f, 1.22f, P);
    // bumpers + skirts + grille
    box(c.trim, 0, 0.30f, 2.30f, 1.88f, 0.22f, 0.18f, DK);
    box(c.trim, 0, 0.30f, -2.30f, 1.88f, 0.22f, 0.18f, DK);
    box(c.trim, 0.95f, 0.32f, 0, 0.08f, 0.16f, 3.6f, DK);
    box(c.trim, -0.95f, 0.32f, 0, 0.08f, 0.16f, 3.6f, DK);
    box(c.trim, 0, 0.52f, 2.33f, 0.9f, 0.16f, 0.06f, DK);   // grille
    box(c.trim, 0, 0.72f, -2.31f, 0.9f, 0.05f, 0.05f, SL); // rear chrome strip
    // mirrors
    box(c.body, 0.98f, 0.95f, 0.85f, 0.14f, 0.08f, 0.10f, P);
    box(c.body, -0.98f, 0.95f, 0.85f, 0.14f, 0.08f, 0.10f, P);
    // headlights + DRL + taillights
    box(c.lights, 0.62f, 0.62f, 2.31f, 0.42f, 0.12f, 0.06f, HL);
    box(c.lights, -0.62f, 0.62f, 2.31f, 0.42f, 0.12f, 0.06f, HL);
    box(c.lights, 0.62f, 0.70f, -2.31f, 0.5f, 0.08f, 0.06f, TL);
    box(c.lights, -0.62f, 0.70f, -2.31f, 0.5f, 0.08f, 0.06f, TL);
    // plate
    float PL[4] = {0.95f, 0.95f, 0.9f, 1.f};
    box(c.lights, 0, 0.45f, -2.36f, 0.5f, 0.16f, 0.02f, PL);

    // wheels
    float TR[4] = {0.06f, 0.06f, 0.07f, 1.f};
    cylX(c.tire, 0, 0, 0, c.wheelR, 0.26f, 14, TR);
    float rimCol[4];
    if (wheelStyle == 0) { rimCol[0]=0.82f; rimCol[1]=0.83f; rimCol[2]=0.86f; rimCol[3]=1; }
    else if (wheelStyle == 1) { rimCol[0]=0.12f; rimCol[1]=0.12f; rimCol[2]=0.14f; rimCol[3]=1; }
    else { rimCol[0]=0.85f; rimCol[1]=0.68f; rimCol[2]=0.28f; rimCol[3]=1; }
    cylX(c.rim, 0, 0, 0, c.wheelR * 0.62f, 0.28f, 12, rimCol);
    int spokes = wheelStyle == 0 ? 5 : (wheelStyle == 1 ? 7 : 10);
    for (int i = 0; i < spokes; i++) {
        float a = i / (float)spokes * 6.2831853f;
        MeshData sp;
        box(sp, 0, 0, 0, 0.30f, 0.05f, c.wheelR * 0.55f, rimCol);
        // rotate+merge into rim
        M4 rot = M4::rotX(a);
        for (size_t k = 0; k < sp.v.size(); k += 12) {
            V3 p = rot.xform(v3(sp.v[k], sp.v[k+1], sp.v[k+2]));
            V3 n = rot.xform(v3(sp.v[k+3], sp.v[k+4], sp.v[k+5]));
            c.rim.v.insert(c.rim.v.end(), {p.x,p.y,p.z, n.x,n.y,n.z, 0,0, rimCol[0],rimCol[1],rimCol[2],1.f});
        }
    }

    if (spoilerOn) {
        box(c.spoiler, 0.55f, 0.98f, -2.05f, 0.06f, 0.22f, 0.06f, P);
        box(c.spoiler, -0.55f, 0.98f, -2.05f, 0.06f, 0.22f, 0.06f, P);
        box(c.spoiler, 0, 1.12f, -2.08f, 1.5f, 0.05f, 0.30f, P);
    }
    if (neonOn) {
        float NC[4] = {0.2f, 0.9f, 1.0f, 0.55f};
        quadN(c.neon, v3(-1.1f, 0.06f, -2.4f), v3(1.1f, 0.06f, -2.4f), v3(1.1f, 0.06f, 2.4f), v3(-1.1f, 0.06f, 2.4f), v3(0,1,0), NC);
    }
    return c;
}
