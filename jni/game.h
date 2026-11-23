// Sayyaratak shared game state.
#pragma once
#include "render.h"
#include "world.h"
#include "carmodel.h"
#include "car.h"

struct Particle { V3 pos, vel; float life, maxLife, size; float r, g, b, a; };

struct Game {
    World world;
    // GL
    Program prog;
    GLMesh ground, farRing, sky, mountains, canyon, roadMesh;
    GLMesh bldgMesh[4];
    GLMesh palms;
    GLMesh starsMesh;
    GLMesh ringMesh, beaconMesh, cloudMesh, sunMesh, shadowMesh;
    GLMesh carBody, carGlass, carTrim, carLights, carTire, carRim, carSpoiler, carNeon;
    GLMesh parkedBody[6], parkedGlass;
    CarModel carModel, parkedModel;
    GLMesh particles;
    std::vector<float> pbuf;
    std::vector<Particle> ps;
    bool sceneBuilt = false;
    bool carDirty = true;
    // screen
    int scrW = 1280, scrH = 720;
    int quality = 1; // 0 low 1 med 2 high
    // mode
    int mode = 0; // 0 menu 1 workshop 2 free 3 challenge
    float garageAngle = 0.f;
    // car state
    V3 pos = {0, 0, 30};
    float heading = 0.f;
    float vx = 0, vz = 0;
    float steer = 0;
    float wheelSpin = 0;
    float vPitch = 0, vRoll = 0, prevSpeed = 0;
    float paint[3] = {0.62f, 0.05f, 0.06f};
    int wheels = 0;
    bool spoiler = true, neon = false, tint = true;
    int engineProfile = 1;
    // input
    float inThrottle = 0, inBrake = 0, inSteer = 0;
    bool inHandbrake = false;
    // camera
    int camMode = 0;
    V3 camPos = {0, 3, -8};
    float shake = 0;
    // gameplay
    float score = 0;      // drift points (session)
    int stars = 0;
    int starsMask[64] = {0};
    float challengeTime = 0;
    int challengeHits = 0;
    int ringIdx = -1;
    // missions (mode 5): 1 rings, 2 delivery, 3 drift
    int missionType = 0, missionCity = 0, missionGoal = 0, missionProgress = 0;
    int missionReward = 0, missionState = 0; // 0 none 1 running 2 done 3 failed
    float missionTime = 0, missionDriftScore = 0;
    V3 deliveryTarget = {0,0,0};
    float earnedTotal = 0; // session earnings for HUD
    float driftActive = 0;
    float driftAccum = 0;
    float fps = 60;
    bool audioOn = true;
    float previewRev = 0;
    float time = 0;
};

extern Game G;

void buildScene();
void buildCarGL();
void drawFrame(float dt);
void stepPhysics(float dt);
#include "audio_state.h"
