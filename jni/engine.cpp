// Sayyaratak: frame render + physics + JNI bridge.
#include <jni.h>
#include <cmath>
#include <cstring>
#include <android/log.h>
#include "game.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "sayya", __VA_ARGS__)

Game G;

GLuint* texIds() { static GLuint t[12] = {0}; return t; }
std::string& glErrorLog() { static std::string e; return e; }



// ---------- physics ----------
void spawnDust(int n, float r, float g, float b);
void spawnBurst(float x, float y, float z, float r, float g, float b);

static bool collideAABB(float x, float z, float hw, float hd, float& nx, float& nz) {
    float dx = G.pos.x - x, dz = G.pos.z - z;
    float px = hw + 1.0f - fabsf(dx), pz = hd + 1.0f - fabsf(dz);
    if (px <= 0 || pz <= 0) return false;
    if (px < pz) { nx = dx > 0 ? 1 : -1; nz = 0; G.pos.x += px * nx; }
    else { nx = 0; nz = dz > 0 ? 1 : -1; G.pos.z += pz * nz; }
    return true;
}

void stepPhysics(float dt) {
    float fwdX = sinf(G.heading), fwdZ = cosf(G.heading);
    float speed = G.vx * fwdX + G.vz * fwdZ;
    float latX = G.vx - fwdX * speed, latZ = G.vz - fwdZ * speed;
    float lat = sqrtf(latX * latX + latZ * latZ);

    // steering
    float maxSteer = 0.58f / (1.f + fabsf(speed) * 0.045f);
    float target = G.inSteer * maxSteer;
    // Steering smoothing derived from DAShoe1/Godot-Easy-Vehicle-Physics (MIT):
    // slower rate into the turn, faster return to center, target decays with speed.
    {
        float decay = 1.f / (1.f + fabsf(speed) * 0.055f); // speed decay (their 0.20 scale adapted to m/s)
        float tgt = target * decay;
        float rate = (fabsf(tgt) > fabsf(G.steer) ? 4.25f : 11.0f); // steer-in vs countersteer
        G.steer += (tgt - G.steer) * fminf(1.f, dt * rate);
    }

    // ---- phase B: surface + drivetrain model ----
    bool offroad = !G.world.onRoad(G.pos.x, G.pos.z, 0.5f);
    // surface table: asphalt vs sand/gravel (weather extends this later)
    float surfGrip = offroad ? 0.68f : 1.0f;
    float surfRoll = offroad ? 2.4f : 0.32f;
    // 6-speed automatic: per-gear top speeds (m/s) and accel multipliers
    static const float TOPS[7]  = {0, 12.f, 20.f, 29.f, 38.f, 46.f, 53.f};
    static const float GMUL[7]  = {0, 1.0f, 0.74f, 0.56f, 0.44f, 0.36f, 0.30f};
    bool low = G.driveMode == 2;
    float dmgP = 1.f - G.damage * 0.004f; if (dmgP < 0.45f) dmgP = 0.45f;
    float sp = fabsf(speed);
    // shift logic (automatic) derived from Dechode/Godot-Advanced-Vehicle (MIT):
    // compare wheel torque in adjacent gears; shift only when the other gear pulls
    // harder, with a 0.7 s cooldown between shifts.
    G.shiftTimer = fmaxf(0.f, G.shiftTimer - dt);
    // Shift (MIT): during a shift the box cuts torque for 0.15-0.25s (natural
    // variation), then the new ratio engages and rpm snaps to its new band.
    if (G.pendingGear > 0) {
        if (G.shiftCut <= 0.f) G.shiftCut = 0.15f + fmodf(G.time * 7.31f, 0.10f);
        G.shiftCut -= dt;
        if (G.shiftCut <= 0.f) { G.gear = G.pendingGear; G.pendingGear = 0; }
    }
    if (G.shiftTimer <= 0.f && G.inThrottle > 0.05f) {
        auto accelIn = [&](int g) {
            float loG = g > 1 ? TOPS[g - 1] * 0.55f : 0.f;
            float bandG = TOPS[g] - loG + 0.001f;
            float r01 = (sp - loG) / bandG; if (r01 < 0.f) r01 = 0.f; if (r01 > 1.f) r01 = 1.f;
            float tqG = 0.60f + 0.55f * sinf(fminf(1.f, r01) * 2.4f);
            return tqG * GMUL[g];
        };
        float cur = accelIn(G.gear);
        if (G.gear < 6 && sp > TOPS[G.gear] * 0.96f) {
            G.pendingGear = G.gear + 1; G.shiftTimer = 0.7f; // redline guard: always shift at the top
        } else if (G.gear < 6 && accelIn(G.gear + 1) > cur && sp > TOPS[G.gear] * 0.55f) {
            G.pendingGear = G.gear + 1; G.shiftTimer = 0.7f;
        } else if (G.gear > 1 && accelIn(G.gear - 1) > cur * 1.05f && sp < TOPS[G.gear - 1] * 0.85f) {
            G.pendingGear = G.gear - 1; G.shiftTimer = 0.7f;
        }
    }
    // rpm within the gear band + torque curve (peaks mid-band)
    float lo = G.gear > 1 ? TOPS[G.gear - 1] * 0.55f : 0.f;
    float band = TOPS[G.gear] - lo + 0.001f;
    float rpmT = fminf(1.f, fmaxf(0.f, (sp - lo) / band));
    float rpmTarget = G.inThrottle > 0.05f ? 0.22f + 0.78f * rpmT : fmaxf(0.12f, 0.22f + 0.78f * rpmT * 0.6f);
    G.rpm += (rpmTarget - G.rpm) * fminf(1.f, dt * 6.f);
    if (G.pendingGear > 0) G.rpm += (0.16f - G.rpm) * fminf(1.f, dt * 14.f); // clutch in: revs fall
    float tq = 0.60f + 0.55f * sinf(fminf(1.f, G.rpm) * 2.4f);
    // requested engine accel
    float accel = G.inThrottle * 12.5f * tq * GMUL[G.gear] * dmgP * (low ? 1.9f : 1.f);
    if (G.pendingGear > 0) accel = 0.f; // Shift: gearbox transmits no torque mid-shift
    // traction limit: RWD lower than AWD/4L; TC clamps and flags
    float tracLim = surfGrip * (G.driveMode >= 1 ? 12.0f : 8.6f);
    if (accel > tracLim) { G.slipT = fminf(1.f, (accel - tracLim) / (accel + 0.01f)); accel = tracLim; }
    else G.slipT = fmaxf(0.f, G.slipT - dt * 4.f);
    float drag = surfRoll + fabsf(speed) * 0.012f + (G.inThrottle < 0.05f ? 0.45f + (6 - G.gear) * 0.10f : 0.f); // engine braking
    if (G.inBrake > 0) {
        float want = G.inBrake * (speed > 0.6f ? 14.f : 6.5f);
        float brakeLim = surfGrip * 13.5f;
        if (want > brakeLim) { G.slipB = fminf(1.f, (want - brakeLim) / (want + 0.01f)); want = brakeLim; } // ABS
        else G.slipB = fmaxf(0.f, G.slipB - dt * 4.f);
        accel -= want;
    }
    speed += (accel - drag * (speed > 0 ? 1 : (speed < 0 ? -1 : 0))) * dt;
    float top = TOPS[6] * (low ? 0.55f : 1.f) * dmgP, rtop = -9.f;
    if (speed > top) speed = top;
    if (speed < rtop) speed = rtop;
    if (fabsf(speed) < 0.15f && G.inThrottle == 0 && G.inBrake == 0) speed = 0;
    // damaged steering pull
    if (G.damage > 60) G.steer += (G.damage - 60) * 0.0006f;
    // fuel + engine temperature
    G.fuel = fmaxf(0.f, G.fuel - dt * (0.05f + G.inThrottle * 0.22f + G.rpm * 0.08f));
    if (G.fuel <= 0.f) accel *= 0.2f; // limp home on empty
    float tempTarget = 62.f + G.rpm * 48.f;
    G.engineTemp += (tempTarget - G.engineTemp) * fminf(1.f, dt * 0.06f);
    if (G.engineTemp > 108.f) accel *= 0.75f; // heat soak

    // yaw
    float yaw = speed / 2.7f * tanf(G.steer);
    G.heading += yaw * dt;

    // lateral grip: exponential velocity-alignment decay (form ported from
    // stranmous/RallyCraft, MIT: rate via 1-exp(-r*dt)) with kinetic-grip falloff
    // once sliding (dicnunz/vehicle-physics, MIT: sliding grip = 0.84x past slip 0.35)
    float slipNow = fabsf(lat) / (fabsf(speed) + 2.f);
    float kin = slipNow > 0.35f ? 0.84f : 1.f;
    float grip = (G.inHandbrake ? 2.0f : (offroad ? 4.2f : 7.5f) * (G.driveMode >= 1 ? 1.08f : 1.f)) * kin;
    lat *= expf(-grip * dt);

    // recompose velocity in (possibly new) heading frame
    fwdX = sinf(G.heading); fwdZ = cosf(G.heading);
    float rX = fwdZ, rZ = -fwdX;
    float latSign = (latX * rX + latZ * rZ) >= 0 ? 1.f : -1.f;
    G.vx = fwdX * speed + rX * lat * latSign;
    G.vz = fwdZ * speed + rZ * lat * latSign;

    G.pos.x += G.vx * dt;
    G.pos.z += G.vz * dt;

    // collisions
    float nx = 0, nz = 0;
    for (auto& c : G.world.colliders) {
        if (fabsf(G.pos.x - c.x) > c.hw + 3 || fabsf(G.pos.z - c.z) > c.hd + 3) continue;
        if (collideAABB(c.x, c.z, c.hw, c.hd, nx, nz)) {
            float vn = G.vx * nx + G.vz * nz;
            if (vn < 0) {
                G.vx -= vn * 1.5f * nx; G.vz -= vn * 1.5f * nz;
                if (vn < -4) { G.shake = fminf(1.f, -vn * 0.08f); audioOneshot(2);
                    G.damage = fminf(100.f, G.damage + (-vn - 4.f) * 2.4f); }
            }
        }
    }
    // bounds
    float bx = 1130.f;
    if (G.pos.x > World::SEA_X - 25) G.pos.x = World::SEA_X - 25;
    if (G.pos.x < -bx) G.pos.x = -bx;
    if (G.pos.z > bx) G.pos.z = bx;
    if (G.pos.z < -bx) G.pos.z = -bx;

    G.wheelSpin += speed * dt / 0.34f;

    // traffic update + player collision
    G.world.updateTraffic(dt, G.pos.x, G.pos.z);
    for (auto& tc : G.world.traffic) {
        float dx = G.pos.x - tc.x, dz = G.pos.z - tc.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < 4.6f && d2 > 0.0001f) {
            float d = sqrtf(d2);
            float push = (2.15f - d) / d;
            G.pos.x += dx * push; G.pos.z += dz * push;
            float vn = G.vx * (dx / d) + G.vz * (dz / d);
            if (vn < 0) { G.vx -= vn * 1.4f * (dx / d); G.vz -= vn * 1.4f * (dz / d); }
            if (-vn > 3.5f) { G.shake = fminf(1.f, -vn * 0.1f); audioOneshot(2);
                G.damage = fminf(100.f, G.damage + (-vn - 3.5f) * 1.8f); }
        }
    }

    // visual weight transfer
    {
        float acc = dt > 0 ? (speed - G.prevSpeed) / dt : 0.f;
        G.prevSpeed = speed;
        float tp = -acc * 0.0045f; if (tp < -0.055f) tp = -0.055f; if (tp > 0.07f) tp = 0.07f;
        float tr = G.steer * fabsf(speed) * 0.0028f; if (tr < -0.09f) tr = -0.09f; if (tr > 0.09f) tr = 0.09f;
        float k = fminf(1.f, dt * 5.5f);
        G.vPitch += (tp - G.vPitch) * k;
        G.vRoll += (tr - G.vRoll) * k;
    }

    // drift score + particles
    float slip = atan2f(lat, fabsf(speed) + 0.5f);
    bool drifting = slip > 0.32f && fabsf(speed) > 8.f;
    G.driftActive = drifting ? 1.f : 0.f;
    if (drifting) {
        G.driftAccum += slip * fabsf(speed) * dt * 1.4f;
        if (G.mode == 5 && G.missionType == 3 && G.missionState == 1)
            G.missionDriftScore += slip * fabsf(speed) * dt * 1.4f;
        if (G.driftAccum > 25.f) { G.score += 25; G.driftAccum = 0; }
        spawnDust(2, 0.75f, 0.68f, 0.55f);
    }
    if (offroad && fabsf(speed) > 6) spawnDust(1, 0.82f, 0.72f, 0.5f);

    // stars
    for (size_t i = 0; i < G.world.stars.size() && i < 64; i++) {
        if (G.starsMask[i]) continue;
        V3 s = G.world.stars[i];
        float dx = G.pos.x - s.x, dz = G.pos.z - s.z;
        if (dx * dx + dz * dz < 9.f) {
            G.starsMask[i] = 1; G.stars++;
            // RacingGame (MIT) bonus flow: pickups give a resource reward
            G.fuel = fminf(100.f, G.fuel + 6.f);
            G.damage = fmaxf(0.f, G.damage - 4.f);
            audioOneshot(1);
            for (int k = 0; k < 14; k++) spawnBurst(s.x, 1.6f, s.z, 1.f, 0.85f, 0.3f);
        }
    }

    // challenge
    if (G.mode == 5 && G.missionState == 1) {
        G.missionTime -= dt;
        if (G.missionTime <= 0) { G.missionTime = 0; G.missionState = 3; }
        if (G.missionType == 1 && G.ringIdx >= 0 && G.ringIdx < (int)G.world.intersections.size()) {
            V3 r = G.world.intersections[G.ringIdx];
            float dx = G.pos.x - r.x, dz = G.pos.z - r.z;
            if (dx*dx + dz*dz < 30.f) {
                G.missionProgress++;
                audioOneshot(1);
                G.ringIdx = (G.ringIdx * 7 + 3) % (int)G.world.intersections.size();
                if (G.missionProgress >= G.missionGoal) G.missionState = 2;
            }
        } else if (G.missionType == 2) {
            float dx = G.pos.x - G.deliveryTarget.x, dz = G.pos.z - G.deliveryTarget.z;
            if (dx*dx + dz*dz < 90.f) { G.missionState = 2; audioOneshot(1); }
        } else if (G.missionType == 3) {
            G.missionProgress = (int)G.missionDriftScore;
            if (G.missionProgress >= G.missionGoal) G.missionState = 2;
        }
        if (G.missionState == 2) {
            int bonus = G.missionType == 3 ? G.missionProgress / 10 : (int)(G.missionTime * 2);
            G.missionReward += bonus;
            G.earnedTotal += G.missionReward;
        }
    }
    if (G.mode == 3) {
        G.challengeTime -= dt;
        if (G.challengeTime <= 0) G.challengeTime = 0;
        if (G.ringIdx >= 0 && G.ringIdx < (int)G.world.intersections.size()) {
            V3 r = G.world.intersections[G.ringIdx];
            float dx = G.pos.x - r.x, dz = G.pos.z - r.z;
            if (dx * dx + dz * dz < 40.f) {
                G.challengeHits++;
                audioOneshot(1);
                for (int k = 0; k < 16; k++) spawnBurst(r.x, 2.f, r.z, 1.f, 0.8f, 0.25f);
                G.ringIdx = (G.ringIdx * 7 + 3) % (int)G.world.intersections.size();
            }
        }
    }

    // audio
    float rpm = fminf(1.f, 0.14f + fabsf(speed) / top * 0.62f + G.inThrottle * 0.28f);
    if (G.previewRev > 0) {
        G.previewRev -= dt;
        float t = 1.4f - G.previewRev;
        rpm = 0.3f + sinf(fminf(t / 1.4f, 1.f) * 3.14159f) * 0.7f;
        G.previewRev = fmaxf(0.f, G.previewRev);
    }
    audioState().rpm = rpm;
    audioState().throttle = G.mode >= 2 || G.previewRev > 0 ? fmaxf(G.inThrottle, G.previewRev > 0 ? 0.6f : 0.f) : 0.f;
    audioState().speed01 = fabsf(speed) / top;
    audioState().slip = drifting ? fminf(1.f, slip) : 0.f;
}

void spawnDust(int n, float r, float g, float b) {
    float back = G.heading + 3.14159f;
    for (int i = 0; i < n; i++) {
        if (G.ps.size() > 230) return;
        float jx = (rand() % 100 / 100.f - 0.5f) * 1.6f;
        Particle p;
        p.pos = v3(G.pos.x + sinf(back) * 1.5f + jx, 0.25f, G.pos.z + cosf(back) * 1.5f + jx);
        p.vel = v3(jx * 1.2f, 1.2f + rand() % 100 / 100.f, (rand() % 100 / 100.f - 0.5f) * 1.6f);
        p.life = p.maxLife = 0.7f + rand() % 60 / 100.f;
        p.size = 0.8f; p.r = r; p.g = g; p.b = b; p.a = 0.42f;
        G.ps.push_back(p);
    }
}

void spawnBurst(float x, float y, float z, float r, float g, float b) {
    if (G.ps.size() > 230) return;
    Particle p;
    float a = (rand() % 628) / 100.f;
    p.pos = v3(x, y, z);
    p.vel = v3(cosf(a) * 3.5f, 2.5f + rand() % 30 / 10.f, sinf(a) * 3.5f);
    p.life = p.maxLife = 0.8f;
    p.size = 0.5f; p.r = r; p.g = g; p.b = b; p.a = 0.9f;
    G.ps.push_back(p);
}

// ---------- render ----------
static void setCommon(const M4& proj, const M4& view, V3 cam) {
    Program& p = G.prog;
    p.use();
    glUniform3f(p.uSunDir, SUN.x, SUN.y, SUN.z);
    glUniform3f(p.uSunCol, 1.15f, 1.02f, 0.82f);
    glUniform3f(p.uAmbSky, 0.34f, 0.40f, 0.52f);
    glUniform3f(p.uAmbGnd, 0.36f, 0.28f, 0.20f);
    glUniform3f(p.uCamPos, cam.x, cam.y, cam.z);
    glUniform3f(p.uFogCol, 0.91f, 0.78f, 0.62f);
    float fe = G.quality == 0 ? 750 : (G.quality == 1 ? 1200 : 1700);
    glUniform2f(p.uFog, 240, fe);
    glUniform1i(p.uTex, 0);
    glUniform2f(p.uRes, (float)G.scrW, (float)G.scrH);
}

static void drawMesh(GLMesh& m, const M4& model, const M4& mvp,
                     float r, float g, float b, float a,
                     int tex, float useTex, float emissive, float spec, float shin,
                     float alphaTest, float noFog, float envRef) {
    Program& p = G.prog;
    glUniformMatrix4fv(p.uMVP, 1, false, mvp.m);
    glUniformMatrix4fv(p.uModel, 1, false, model.m);
    float n3[9] = {model.m[0],model.m[1],model.m[2], model.m[4],model.m[5],model.m[6], model.m[8],model.m[9],model.m[10]};
    glUniformMatrix3fv(p.uNrm, 1, false, n3);
    glUniform4f(p.uColor, r, g, b, a);
    glUniform1f(p.uUseTex, useTex);
    glUniform1f(p.uEmissive, emissive);
    glUniform1f(p.uSpec, spec);
    glUniform1f(p.uShin, shin);
    glUniform1f(p.uAlphaTest, alphaTest);
    glUniform1f(p.uNoFog, noFog);
    glUniform1f(p.uEnvRef, envRef);
    if (useTex > 0.5f) bindTex(tex);
    m.draw();
}

void drawFrame(float dt) {
    G.time += dt;
    if (!G.sceneBuilt) return;
    if (G.carDirty) buildCarGL();
    if (G.carModel.loaded && !G.carModel.glReady) G.carModel.upload();
    if (G.parkedModel.loaded && !G.parkedModel.glReady) G.parkedModel.upload();

    bool driving = G.mode >= 2;
    if (driving) stepPhysics(dt);
    else {
        G.garageAngle += dt * (G.mode == 1 ? 0.25f : 0.10f);
        audioState().throttle = G.previewRev > 0 ? 0.6f : 0.f;
        if (G.previewRev > 0) {
            G.previewRev -= dt;
            float t = fmaxf(0.f, G.previewRev);
            audioState().rpm = 0.3f + sinf(fminf((1.4f - t) / 1.4f, 1.f) * 3.14159f) * 0.7f;
        } else audioState().rpm = 0.12f;
        audioState().speed01 = 0; audioState().slip = 0;
    }

    // camera
    float fwdX = sinf(G.heading), fwdZ = cosf(G.heading);
    V3 look;
    if (driving) {
        V3 want;
        if (G.camMode == 0) want = v3(G.pos.x - fwdX * 7.6f, 3.1f, G.pos.z - fwdZ * 7.6f);
        else if (G.camMode == 1) want = v3(G.pos.x + fwdX * 0.1f, 1.24f, G.pos.z + fwdZ * 0.1f);
        else want = v3(G.pos.x - fwdX * 4.f, 42.f, G.pos.z - fwdZ * 4.f);
        float k = G.camMode == 1 ? 1.f : 1.f - expf(-dt * (G.camMode == 2 ? 3.2f : 5.5f));
        G.camPos = vadd(G.camPos, vscale(vsub(want, G.camPos), k));
        look = G.camMode == 2 ? v3(G.pos.x, 0, G.pos.z)
             : v3(G.pos.x + fwdX * 5.f, 1.0f, G.pos.z + fwdZ * 5.f);
    } else {
        float a = G.garageAngle;
        G.camPos = v3(G.pos.x + sinf(a) * 7.2f, 2.3f, G.pos.z + cosf(a) * 7.2f);
        look = v3(G.pos.x, 0.8f, G.pos.z);
    }
    G.shake = fmaxf(0.f, G.shake - dt * 2.2f);
    V3 cam = G.camPos;
    if (G.shake > 0) {
        cam.x += (rand() % 100 / 100.f - 0.5f) * G.shake * 0.5f;
        cam.y += (rand() % 100 / 100.f - 0.5f) * G.shake * 0.4f;
    }
    if (cam.y < 0.4f) cam.y = 0.4f;

    M4 proj = M4::perspective(1.05f, (float)G.scrW / (float)G.scrH, 0.3f, 4200.f);
    M4 view = M4::lookAt(cam, look, v3(0, 1, 0));
    M4 vp = M4::mul(proj, view);

    glViewport(0, 0, G.scrW, G.scrH);
    glClearColor(0.91f, 0.78f, 0.62f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    setCommon(proj, view, cam);

    M4 I = M4::ident();
    // sky (no fog, emissive, no depth write)
    glDepthMask(false);
    M4 skyM = M4::translate(cam.x, 0, cam.z);
    drawMesh(G.sky, skyM, M4::mul(vp, skyM), 1,1,1,1, 8, texIds()[8] ? 1.f : 0.f, 0.9f, 0, 1, 0, 1, 0);
    // sun billboard
    V3 sd = vnorm(SUN);
    M4 sunM = M4::mul(M4::translate(cam.x + sd.x * 2600, cam.y + sd.y * 2600, cam.z + sd.z * 2600),
                      M4::mul(M4::rotY(atan2f(-sd.x, -sd.z)), M4::rotX(-asinf(sd.y))));
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawMesh(G.sunMesh, sunM, M4::mul(vp, sunM), 1,1,1,1, 6, 1, 1.2f, 0, 1, 0, 1, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // clouds
    drawMesh(G.cloudMesh, I, vp, 1,1,1,1, 6, 1, 0.85f, 0, 1, 0, 0, 0);
    glDepthMask(true);
    glDisable(GL_BLEND);

    // ground + far ring
    drawMesh(G.ground, I, vp, 1,1,1,1, 0, 1, 0, 0.04f, 12, 0, 0, 0);
    drawMesh(G.roadMesh, I, vp, 1,1,1,1, 7, 1, 0, 0.05f, 12, 0, 0, 0);
    drawMesh(G.farRing, I, vp, 1,1,1,1, 0, 0, 0, 0, 1, 0, 0, 0);
    // mountains + canyon
    drawMesh(G.mountains, I, vp, 1,1,1,1, 0, 0, 0, 0.05f, 8, 0, 0, 0);
    drawMesh(G.canyon, I, vp, 1,1,1,1, 0, 0, 0, 0.1f, 10, 0, 0, 0);
    { glEnable(GL_BLEND); glDepthMask(false);
    drawMesh(G.bldgShadow, I, vp, 1,1,1,1, 6, 1, 0, 0, 1, 0, 0, 0);
    glDepthMask(true); glDisable(GL_BLEND); }
    drawMesh(G.props, I, vp, 1,1,1,1, 0, 0, 0, 0.3f, 24, 0, 0, 0);
    // buildings
    for (int t = 0; t < 4; t++)
        drawMesh(G.bldgMesh[t], I, vp, 1,1,1,1, 1 + t, 1, 0, 0.10f, 14, 0, 0, 0);
    // palms (alpha test)
    drawMesh(G.palms, I, vp, 1,1,1,1, 5, 1, 0, 0.05f, 8, 1, 0, 0);
    // stars
    for (size_t i = 0; i < G.world.stars.size() && i < 64; i++) {
        if (G.starsMask[i]) continue;
        V3 s = G.world.stars[i];
        M4 m = M4::mul(M4::translate(s.x, s.y + sinf(G.time * 2 + i) * 0.25f, s.z),
                       M4::rotY(G.time * 1.5f + i));
        drawMesh(G.starsMesh, m, M4::mul(vp, m), 1,1,1,1, 0, 0, 0.55f, 0.5f, 32, 0, 0, 0);
    }
    // mission ring / delivery beacon
    if (G.mode == 5 && G.missionState == 1) {
        V3 r = G.missionType == 2 ? G.deliveryTarget
             : (G.ringIdx >= 0 && G.ringIdx < (int)G.world.intersections.size() ? G.world.intersections[G.ringIdx] : v3(0,-99,0));
        if (r.y > -50) {
            M4 rm = M4::mul(M4::translate(r.x, 3.4f + sinf(G.time * 2.5f) * 0.3f, r.z),
                            M4::mul(M4::rotY(G.time * 0.5f), M4::rotX(1.5708f)));
            drawMesh(G.ringMesh, rm, M4::mul(vp, rm), 0.4f,0.9f,1.f,1, 0, 0, 1.0f, 0.6f, 48, 0, 0, 0);
            M4 bm = M4::translate(r.x, 0, r.z);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glDepthMask(false);
            drawMesh(G.beaconMesh, bm, M4::mul(vp, bm), 0.4f,0.9f,1.f,1, 0, 0, 0.9f, 0, 1, 0, 0, 0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(true); glDisable(GL_BLEND);
        }
    }
    // challenge ring + beacon
    if (G.mode == 3 && G.ringIdx >= 0 && G.ringIdx < (int)G.world.intersections.size()) {
        V3 r = G.world.intersections[G.ringIdx];
        M4 rm = M4::mul(M4::translate(r.x, 3.4f + sinf(G.time * 2.5f) * 0.3f, r.z),
                        M4::mul(M4::rotY(G.time * 0.5f), M4::rotX(1.5708f)));
        drawMesh(G.ringMesh, rm, M4::mul(vp, rm), 1,1,1,1, 0, 0, 1.0f, 0.6f, 48, 0, 0, 0);
        M4 bm = M4::translate(r.x, 0, r.z);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glDepthMask(false);
        drawMesh(G.beaconMesh, bm, M4::mul(vp, bm), 1,1,1,1, 0, 0, 0.9f, 0, 1, 0, 0, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(true); glDisable(GL_BLEND);
    }
    // parked cars
    for (auto& pc : G.world.parked) {
        M4 m = M4::mul(M4::translate(pc.x, 0, pc.z), M4::rotY(pc.heading));
        M4 pmvp = M4::mul(vp, m);
        if (G.parkedModel.glReady) {
            const float* pc3 = PAINTS[pc.colorIdx];
            for (int pi = 0; pi < G.parkedModel.partCount; pi++) {
                CarPart& P = G.parkedModel.parts[pi];
                if (P.flags & 4) drawMesh(P.gpu, m, pmvp, pc3[0],pc3[1],pc3[2],1, 0,0,0, 0.6f, 48, 0, 0, 0.4f);
                else if (P.flags & 2) drawMesh(P.gpu, m, pmvp, 1,1,1,1, 0,0,0.4f, 0.5f, 48, 0, 0, 0.2f);
                else if (P.flags & 1) { glEnable(GL_BLEND); glDepthMask(false);
                    drawMesh(P.gpu, m, pmvp, 1,1,1,0.85f, 0,0,0, 0.9f, 90, 0, 0, 0.5f);
                    glDepthMask(true); glDisable(GL_BLEND); }
                else drawMesh(P.gpu, m, pmvp, 1,1,1,1, 0,0,0, 0.5f, 48, 0, 0, 0.35f);
            }
        } else {
            drawMesh(G.parkedBody[pc.colorIdx], m, pmvp, 1,1,1,1, 0, 0, 0, 0.5f, 48, 0, 0, 0.35f);
            glEnable(GL_BLEND); glDepthMask(false);
            drawMesh(G.parkedGlass, m, pmvp, 1,1,1,0.9f, 0, 0, 0, 0.9f, 90, 0, 0, 0.5f);
            glDepthMask(true); glDisable(GL_BLEND);
        }
    }

    // moving traffic
    if (G.parkedModel.glReady) {
        for (auto& tc : G.world.traffic) {
            M4 m = M4::mul(M4::translate(tc.x, 0, tc.z), M4::rotY(tc.heading));
            M4 pmvp = M4::mul(vp, m);
            const float* pc3 = PAINTS[tc.colorIdx];
            for (int pi = 0; pi < G.parkedModel.partCount; pi++) {
                CarPart& P = G.parkedModel.parts[pi];
                M4 lm = m;
                if (P.name.rfind("wheel_", 0) == 0) {
                    lm = M4::mul(m, M4::mul(M4::translate(P.pivot.x, P.pivot.y, P.pivot.z),
                         M4::mul(M4::rotX(tc.wheelSpin), M4::translate(-P.pivot.x, -P.pivot.y, -P.pivot.z))));
                    pmvp = M4::mul(vp, lm);
                }
                if (P.flags & 4) drawMesh(P.gpu, lm, pmvp, pc3[0],pc3[1],pc3[2],1, 0,0,0, 0.6f, 48, 0, 0, 0.4f);
                else if (P.flags & 2) drawMesh(P.gpu, lm, pmvp, 1,1,1,1, 0,0,0.4f, 0.5f, 48, 0, 0, 0.2f);
                else if (P.flags & 1) { glEnable(GL_BLEND); glDepthMask(false);
                    drawMesh(P.gpu, lm, pmvp, 1,1,1,0.85f, 0,0,0, 0.9f, 90, 0, 0, 0.5f);
                    glDepthMask(true); glDisable(GL_BLEND); }
                else drawMesh(P.gpu, lm, pmvp, 1,1,1,1, 0,0,0, 0.5f, 48, 0, 0, 0.35f);
            }
        }
    }

    // player car
    float carYaw = driving ? G.heading : -G.garageAngle * 1.2f;
    V3 carPos = driving ? G.pos : G.pos;
    M4 carM = M4::mul(M4::translate(carPos.x, carPos.y, carPos.z),
                M4::mul(M4::rotY(carYaw), M4::mul(M4::rotZ(G.vRoll), M4::rotX(G.vPitch))));
    // blob shadow
    glEnable(GL_BLEND); glDepthMask(false);
    M4 sh = M4::mul(M4::translate(carPos.x, 0.03f, carPos.z), M4::mul(M4::rotY(carYaw), M4::scale(1.5f, 1, 2.7f)));
    drawMesh(G.shadowMesh, sh, M4::mul(vp, sh), 1,1,1,1, 6, 1, 0, 0, 1, 0, 0, 0);
    glDepthMask(true); glDisable(GL_BLEND);

    if (G.carModel.glReady) {
        M4 cmvp = M4::mul(vp, carM);
        float steerA = G.steer * 1.1f;
        for (int pi = 0; pi < G.carModel.partCount; pi++) {
            CarPart& P = G.carModel.parts[pi];
            if ((P.flags & 8) && !G.spoiler) continue;   // wing hidden when spoiler off
            if (P.flags & 1) continue;                    // glass drawn later
            M4 lm = carM;
            bool isWheel = P.name.rfind("wheel_", 0) == 0;
            bool isSteer = P.name.rfind("steer_", 0) == 0;
            if (isWheel || isSteer) {
                float sa = (P.name == "wheel_FL" || P.name == "wheel_FR" || P.name == "steer_L" || P.name == "steer_R") ? steerA : 0.f;
                lm = M4::mul(carM, M4::mul(M4::translate(P.pivot.x, P.pivot.y, P.pivot.z),
                     M4::mul(M4::rotY(sa),
                     M4::mul(isWheel ? M4::rotX(G.wheelSpin) : M4::ident(),
                             M4::translate(-P.pivot.x, -P.pivot.y, -P.pivot.z)))));
            }
            M4 pm = M4::mul(vp, lm);
            if (P.flags & 4) drawMesh(P.gpu, lm, pm, G.paint[0], G.paint[1], G.paint[2], 1, 0, 0, 0, 0.9f, 72, 0, 0, 0.6f);
            else if (P.flags & 2) drawMesh(P.gpu, lm, pm, 1,1,1,1, 0, 0, 0.9f, 0.5f, 60, 0, 0, 0.2f);
            else if (isWheel) drawMesh(P.gpu, lm, pm, 1,1,1,1, 0, 0, 0, 0.55f, 40, 0, 0, 0.3f);
            else drawMesh(P.gpu, lm, pm, 1,1,1,1, 0, 0, 0, 0.6f, 40, 0, 0, 0.35f);
        }
        // glass
        CarPart* gl = const_cast<CarPart*>(G.carModel.find("glass"));
        if (gl) {
            glEnable(GL_BLEND); glDepthMask(false);
            drawMesh(gl->gpu, carM, cmvp, 1,1,1, G.tint ? 0.9f : 0.55f, 0, 0, 0, 0.95f, 110, 0, 0, 0.6f);
            glDepthMask(true); glDisable(GL_BLEND);
        }
        if (G.neon) {
            glEnable(GL_BLEND); glDepthMask(false); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            drawMesh(G.carNeon, carM, cmvp, 1,1,1,1, 6, 1, 1.1f, 0, 1, 0, 0, 0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(true); glDisable(GL_BLEND);
        }
        if (G.lightsOn) {
            // soft headlight pools on the road ahead (+z is the car's front)
            glEnable(GL_BLEND); glDepthMask(false); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            for (int sd = -1; sd <= 1; sd += 2) {
                M4 lm = M4::mul(carM, M4::mul(M4::translate(sd * 0.75f, 0.07f, 6.0f),
                        M4::scale(1.5f, 1, 3.4f)));
                drawMesh(G.shadowMesh, lm, M4::mul(vp, lm), 1.f, 0.95f, 0.78f, 0.30f, 6, 1, 0.6f, 0, 1, 0, 0, 0);
            }
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(true); glDisable(GL_BLEND);
        }
    } else {
    drawMesh(G.carTrim, carM, M4::mul(vp, carM), 1,1,1,1, 0, 0, 0, 0.3f, 24, 0, 0, 0.15f);
    drawMesh(G.carBody, carM, M4::mul(vp, carM), 1,1,1,1, 0, 0, 0, 0.85f, 64, 0, 0, 0.55f);
    if (G.spoiler) drawMesh(G.carSpoiler, carM, M4::mul(vp, carM), 1,1,1,1, 0, 0, 0, 0.85f, 64, 0, 0, 0.55f);
    drawMesh(G.carLights, carM, M4::mul(vp, carM), 1,1,1,1, 0, 0, 0.5f, 0.6f, 80, 0, 0, 0.2f);
    // wheels
    CarParts dims;
    for (int w = 0; w < 4; w++) {
        float lx = (w % 2 == 0) ? -dims.wheelX : dims.wheelX;
        float lz = (w < 2) ? dims.wheelZF : dims.wheelZR;
        float steerA = (w < 2) ? G.steer * 1.1f : 0.f;
        M4 wm = M4::mul(carM, M4::mul(M4::translate(lx, dims.wheelY, lz),
                        M4::mul(M4::rotY(steerA), M4::rotX(G.wheelSpin))));
        drawMesh(G.carTire, wm, M4::mul(vp, wm), 1,1,1,1, 0, 0, 0, 0.15f, 12, 0, 0, 0.05f);
        drawMesh(G.carRim, wm, M4::mul(vp, wm), 1,1,1,1, 0, 0, 0, 0.8f, 90, 0, 0, 0.4f);
    }
    // glass + neon
    glEnable(GL_BLEND); glDepthMask(false);
    drawMesh(G.carGlass, carM, M4::mul(vp, carM), 1,1,1,1, 0, 0, 0, 0.95f, 110, 0, 0, 0.6f);
    if (G.neon) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        drawMesh(G.carNeon, carM, M4::mul(vp, carM), 1,1,1,1, 6, 1, 1.1f, 0, 1, 0, 0, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glDepthMask(true);
    }

    // particles (billboard)
    V3 cf2 = vnorm(vsub(look, cam));
    V3 cr2 = vnorm(vcross(cf2, v3(0,1,0)));
    V3 cu2 = vcross(cr2, cf2);
    for (size_t i = 0; i < G.ps.size(); i++) {
        Particle& p = G.ps[i];
        p.life -= dt;
        if (p.life <= 0) { G.ps[i] = G.ps.back(); G.ps.pop_back(); i--; continue; }
        p.pos = vadd(p.pos, vscale(p.vel, dt));
        p.vel.y -= 1.2f * dt;
    }
    if (!G.ps.empty()) {
        G.pbuf.clear();
        for (auto& p : G.ps) {
            float t = p.life / p.maxLife;
            float s = p.size * (1.6f - t);
            float a = p.a * t;
            V3 rx = vscale(cr2, s), ry = vscale(cu2, s);
            V3 p0 = vsub(vsub(p.pos, rx), ry), p1 = vsub(vadd(p.pos, rx), ry);
            V3 p2 = vadd(vadd(p.pos, rx), ry), p3 = vadd(vsub(p.pos, rx), ry);
            V3 n = vscale(cf2, -1.f);
            auto q = [&](V3 a0, V3 b0, V3 c0, float u0, float v0, float u1, float v1, float u2, float v2) {
                G.pbuf.insert(G.pbuf.end(), {a0.x,a0.y,a0.z, n.x,n.y,n.z, u0,v0, p.r,p.g,p.b,a});
                G.pbuf.insert(G.pbuf.end(), {b0.x,b0.y,b0.z, n.x,n.y,n.z, u1,v1, p.r,p.g,p.b,a});
                G.pbuf.insert(G.pbuf.end(), {c0.x,c0.y,c0.z, n.x,n.y,n.z, u2,v2, p.r,p.g,p.b,a});
            };
            q(p0, p1, p2, 0,0, 1,0, 1,1);
            q(p0, p2, p3, 0,0, 1,1, 0,1);
        }
        if (!G.pbuf.empty()) {
            G.particles.stream(G.pbuf);
            glDepthMask(false);
            drawMesh(G.particles, I, vp, 1,1,1,1, 6, 1, 0.25f, 0, 1, 0, 0.4f, 0);
            glDepthMask(true);
        }
    }
    glDisable(GL_BLEND);
}
