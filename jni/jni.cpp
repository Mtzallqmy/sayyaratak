// Sayyaratak JNI bridge.
#include <jni.h>
#include <cstring>
#include "game.h"

static long lastNs = 0;
static long nowNs() {
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

extern "C" {

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeInit(JNIEnv*, jclass) {
    G.world.gen();
    V3 s = G.world.spawnFor(0);
    G.pos = s;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSurfaceCreated(JNIEnv*, jclass) {
    if (!G.prog.id) {
        G.prog.build();
        buildScene();
        G.carDirty = true;
    }
    audioStart();
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSurfaceChanged(JNIEnv*, jclass, jint w, jint h) {
    G.scrW = w; G.scrH = h;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeDrawFrame(JNIEnv*, jclass) {
    long now = nowNs();
    float dt = lastNs ? (now - lastNs) / 1e9f : 0.016f;
    lastNs = now;
    if (dt > 0.05f) dt = 0.05f;
    if (dt <= 0) dt = 0.016f;
    G.fps = G.fps * 0.95f + (1.f / dt) * 0.05f;
    drawFrame(dt);
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeGetState(JNIEnv* env, jclass, jfloatArray out) {
    float s[16] = {0};
    float fwdX = sinf(G.heading), fwdZ = cosf(G.heading);
    s[0] = fabsf(G.vx * fwdX + G.vz * fwdZ) * 3.6f;   // speed km/h
    s[1] = G.score;
    s[2] = G.driftActive;
    s[3] = G.mode == 3 ? G.challengeTime : 0;
    s[4] = (float)G.challengeHits;
    s[5] = (float)G.mode;
    s[6] = G.fps;
    s[7] = (float)G.camMode;
    s[8] = (float)G.world.districtAt(G.pos.x, G.pos.z);
    s[9] = (float)G.stars;
    s[10] = 0;
    s[11] = G.world.onRoad(G.pos.x, G.pos.z, 0.5f) ? 0.f : 1.f;
    env->SetFloatArrayRegion(out, 0, 16, s);
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetInput(JNIEnv*, jclass, jfloat t, jfloat b, jfloat s, jboolean hb) {
    G.inThrottle = t; G.inBrake = b; G.inSteer = s; G.inHandbrake = hb;
}

JNIEXPORT jint JNICALL Java_com_sayyaratak_NativeBridge_nativeCycleCamera(JNIEnv*, jclass) {
    G.camMode = (G.camMode + 1) % 3;
    return G.camMode;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetMode(JNIEnv*, jclass, jint m) {
    if (m == 3) { // challenge reset
        G.challengeTime = 90.f;
        G.challengeHits = 0;
        G.score = 0;
        if (!G.world.intersections.empty()) G.ringIdx = 0;
    }
    if (m == 2 && G.mode == 3) { /* challenge ended, keep driving */ }
    if (m >= 2 && G.mode < 2) {
        // entering drive: place car heading along +z
        G.vx = 0; G.vz = 0; G.steer = 0;
    }
    if (m == 0 || m == 1) { G.vx = 0; G.vz = 0; }
    G.mode = m;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetPaint(JNIEnv*, jclass, jfloat r, jfloat g, jfloat b) {
    G.paint[0] = r; G.paint[1] = g; G.paint[2] = b; G.carDirty = true;
}
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetWheels(JNIEnv*, jclass, jint s) { G.wheels = s; G.carDirty = true; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetSpoiler(JNIEnv*, jclass, jboolean v) { G.spoiler = v; G.carDirty = true; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetNeon(JNIEnv*, jclass, jboolean v) { G.neon = v; G.carDirty = true; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetTint(JNIEnv*, jclass, jboolean v) { G.tint = v; G.carDirty = true; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetEngineProfile(JNIEnv*, jclass, jint p) { G.engineProfile = p; audioState().profile = p; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetQuality(JNIEnv*, jclass, jint q) { G.quality = q; }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeSetAudioEnabled(JNIEnv*, jclass, jboolean v) { G.audioOn = v; audioState().enabled = v; }

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeTeleport(JNIEnv*, jclass, jint d) {
    V3 s = G.world.spawnFor(d);
    G.pos = s; G.vx = 0; G.vz = 0;
    G.heading = d == 5 ? 1.5708f : 0.f;
}

JNIEXPORT jfloatArray JNICALL Java_com_sayyaratak_NativeBridge_nativeGetWorldData(JNIEnv* env, jclass) {
    std::vector<float> d = G.world.exportData();
    jfloatArray arr = env->NewFloatArray((jsize)d.size());
    env->SetFloatArrayRegion(arr, 0, (jsize)d.size(), d.data());
    return arr;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeUploadTexture(JNIEnv* env, jclass, jint slot, jintArray px, jint w, jint h, jboolean mip) {
    if (slot < 0 || slot >= 8) return;
    jint* data = env->GetIntArrayElements(px, nullptr);
    uploadTex(slot, (const int*)data, w, h, mip);
    env->ReleaseIntArrayElements(px, data, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativePreviewRev(JNIEnv*, jclass) {
    G.previewRev = 1.4f;
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeGarageRotate(JNIEnv*, jclass, jfloat d) {
    G.garageAngle += d;
}

JNIEXPORT jstring JNICALL Java_com_sayyaratak_NativeBridge_nativeGetGlError(JNIEnv* env, jclass) {
    return env->NewStringUTF(glErrorLog().c_str());
}

JNIEXPORT jstring JNICALL Java_com_sayyaratak_NativeBridge_nativeGetGlInfo(JNIEnv* env, jclass) {
    const char* v = (const char*)glGetString(GL_VERSION);
    const char* r = (const char*)glGetString(GL_RENDERER);
    std::string s = std::string(v ? v : "?") + " | " + (r ? r : "?");
    return env->NewStringUTF(s.c_str());
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeLoadModel(JNIEnv* env, jclass, jbyteArray hero, jbyteArray lod) {
    if (hero) {
        jsize n = env->GetArrayLength(hero);
        std::vector<unsigned char> buf(n);
        env->GetByteArrayRegion(hero, 0, n, (jbyte*)buf.data());
        G.carModel.load(buf.data(), buf.size());
    }
    if (lod) {
        jsize n = env->GetArrayLength(lod);
        std::vector<unsigned char> buf(n);
        env->GetByteArrayRegion(lod, 0, n, (jbyte*)buf.data());
        G.parkedModel.load(buf.data(), buf.size());
    }
}

JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativePauseAudio(JNIEnv*, jclass) { audioStop(); }
JNIEXPORT void JNICALL Java_com_sayyaratak_NativeBridge_nativeResumeAudio(JNIEnv*, jclass) { audioStart(); }

}
