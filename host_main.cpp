// Host harness: run the real Sayyaratak engine under Mesa llvmpipe, dump frames.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "game.h"


// audio stubs (audio.cpp excluded on host)
static AudioState g_ast;
AudioState& audioState() { return g_ast; }
void audioStart() {} void audioStop() {} void audioOneshot(int) {}

static void dumpFrame(const char* path, int w, int h) {
    std::vector<unsigned char> px(w * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    // flip vertically, write PPM (RGB)
    FILE* f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; y--)
        for (int x = 0; x < w; x++) {
            int i = (y * w + x) * 4;
            fputc(px[i], f); fputc(px[i+1], f); fputc(px[i+2], f);
        }
    fclose(f);
}

// ground texture approximation: sand + roads from world data (mimics Kotlin painter)
static void makeGroundTex(std::vector<unsigned>& px, int res) {
    std::vector<float> d = G.world.exportData();
    float size = d[0]; float seaX = d[1];
    for (int y = 0; y < res; y++) for (int x = 0; x < res; x++) {
        float wx = (x / (float)res) * size - size/2;
        unsigned c = 0xFFCFA96E; // sand
        if (wx > seaX) c = 0xFF2A6080; // sea
        px[y * res + x] = c;
    }
    size_t i = 2;
    int nR = (int)d[i++];
    auto X = [&](float wx){ return (wx + size/2) / size * res; };
    for (int r = 0; r < nR; r++) {
        float x1=d[i++],z1=d[i++],x2=d[i++],z2=d[i++],w=d[i++]; i++;
        // draw thick line
        int steps = (int)(fabsf(X(x2)-X(x1)) + fabsf(X(z2)-X(z1))) + 2;
        for (int s = 0; s <= steps; s++) {
            float t = s/(float)steps;
            int cx = (int)X(x1+(x2-x1)*t), cy = (int)X(z1+(z2-z1)*t);
            int hw = (int)(w/2/size*res);
            for (int dy=-hw;dy<=hw;dy++) for (int dx=-hw;dx<=hw;dx++) {
                int ax=cx+dx, ay=cy+dy;
                if (ax>=0&&ax<res&&ay>=0&&ay<res) px[ay*res+ax]=0xFF3A3C42;
            }
        }
    }
}

int main(int argc, char** argv) {
    const int W = 1280, H = 640;
    EGLDisplay dpy = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    if (dpy == EGL_NO_DISPLAY) dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(dpy, nullptr, nullptr)) { printf("eglInit fail\n"); return 1; }
    EGLint cfgAttrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24, EGL_NONE};
    EGLConfig cfg; EGLint ncfg = 0;
    if (!eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &ncfg) || ncfg == 0) { printf("cfg fail\n"); return 1; }
    EGLint ctxAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttrs);
    if (ctx == EGL_NO_CONTEXT) { printf("ctx fail\n"); return 1; }
    EGLint surfAttrs[] = {EGL_WIDTH, W, EGL_HEIGHT, H, EGL_NONE};
    EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, surfAttrs);
    if (surf == EGL_NO_SURFACE) { printf("surf fail\n"); return 1; }
    if (!eglMakeCurrent(dpy, surf, surf, ctx)) { printf("current fail\n"); return 1; }
    printf("GL: %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    G.world.gen();
    G.scrW = W; G.scrH = H;
    if (!G.prog.build()) { printf("SHADER FAIL: %s\n", glErrorLog().c_str()); return 1; }
    {
        FILE* f = fopen("assets/models/car.bin", "rb");
        if (f) { fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
            std::vector<unsigned char> b(n); fread(b.data(),1,n,f); fclose(f);
            printf("car model load: %d (%ld bytes)\n", G.carModel.load(b.data(), b.size()) ? 1:0, n); }
        f = fopen("assets/models/car_lod.bin", "rb");
        if (f) { fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
            std::vector<unsigned char> b(n); fread(b.data(),1,n,f); fclose(f);
            printf("lod model load: %d (%ld bytes)\n", G.parkedModel.load(b.data(), b.size()) ? 1:0, n); }
    }
    buildScene();
    buildCarGL();

    // textures
    {
        int gr = 1024;
        std::vector<unsigned> gpx(gr*gr);
        makeGroundTex(gpx, gr);
        uploadTex(0, (int*)gpx.data(), gr, gr, true);
        for (int s = 0; s < 4; s++) {
            int r2 = 64; std::vector<unsigned> b(r2*r2);
            for (int y=0;y<r2;y++) for (int x=0;x<r2;x++) {
                bool win = (x%16>4&&x%16<11)&&(y%16>4&&y%16<11);
                unsigned base = s==1?0xFF92684A:(s==2?0xFF9E764A:(s==3?0xFFE4DCCC:0xFFBAAA94));
                b[y*r2+x] = win?0xFF304050:base;
            }
            uploadTex(1+s, (int*)b.data(), r2, r2, true);
        }
        int pr=64; std::vector<unsigned> p(pr*pr, 0x00000000);
        for (int y=0;y<pr;y++) for (int x=0;x<pr;x++) if ((x-32)*(x-32)+(y-32)*(y-32)<400) p[y*pr+x]=0xFF228B22;
        uploadTex(5, (int*)p.data(), pr, pr, true);
        int sr=32; std::vector<unsigned> sp(sr*sr);
        for (int y=0;y<sr;y++) for (int x=0;x<sr;x++) {
            float dx=(x-16)/16.f, dy=(y-16)/16.f; float dd=sqrtf(dx*dx+dy*dy);
            unsigned a=(unsigned)(dd<1?(1-dd)*255:0);
            sp[y*sr+x]=(a<<24)|0xFFFFFF;
        }
        uploadTex(6, (int*)sp.data(), sr, sr, false);
    }
    // asphalt tile with white edges + dashed yellow center (mimics WorldPainter.roadPixels)
    {
        int ar=128; std::vector<unsigned> ap(ar*ar);
        unsigned seed=4;
        for (int y=0;y<ar;y++) for (int x=0;x<ar;x++) {
            seed = seed*1664525u+1013904223u;
            int n = 66 + ((seed>>13)&31);
            unsigned c = 0xFF000000u | (unsigned)(n) | ((unsigned)(n-2)<<8) | ((unsigned)(n-4)<<16);
            float u = x/(float)ar, v = y/(float)ar;
            if (u < 0.045f || u > 0.955f) c = 0xFFE1DED2u; // ARGB whitish
            if (u > 0.487f && u < 0.513f && v < 0.55f) c = 0xFFE8BE3Cu; // ARGB yellow
            ap[y*ar+x] = c;
        }
        uploadTex(7, (int*)ap.data(), ar, ar, true);
    }

    // real equirect sky from assets (raw RGBA dumped by PIL)
    {
        FILE* f = fopen("/tmp/sky.rgba", "rb");
        if (f) {
            std::vector<unsigned> sk(1024*512);
            size_t n = fread(sk.data(), 4, sk.size(), f);
            fclose(f);
            for (size_t i = 0; i < n; i++) { unsigned char* b=(unsigned char*)&sk[i]; sk[i]=((unsigned)b[3]<<24)|((unsigned)b[2]<<16)|((unsigned)b[1]<<8)|b[0]; }
            uploadTex(8, (int*)sk.data(), 1024, 512, true);
            printf("sky loaded %zu px\n", n);
        } else printf("sky.rgba missing\n");
    }
    printf("texids: %u %u %u err=%d\n", texIds()[0], texIds()[1], texIds()[6], glGetError());
    int shot = 0;
    auto snap = [&](const char* name) {
        char p[128]; snprintf(p, sizeof(p), "/tmp/shot_%s.ppm", name);
        dumpFrame(p, W, H);
        printf("saved %s\n", p);
    };

    // menu/garage view
    G.mode = 0;
    for (int k = 0; k < 30; k++) drawFrame(0.016f);
    snap("menu");

    // free drive at hub
    G.mode = 2; V3 sp = G.world.spawnFor(0); G.pos = sp; G.heading = 0; G.vx=G.vz=0;
    for (int k = 0; k < 30; k++) drawFrame(0.016f);
    snap("hub");

    // drive forward 4 seconds with throttle
    G.inThrottle = 1;
    for (int k = 0; k < 40; k++) drawFrame(0.016f);
    { float sp = sqrtf(G.vx*G.vx+G.vz*G.vz)*3.6f; printf("drive probe: %.1f kmh rpm=%.2f gear=%d fuel=%.2f\n", sp, G.rpm, G.gear, G.fuel); }
    for (int k = 0; k < 200; k++) drawFrame(0.016f);
    { float sp = sqrtf(G.vx*G.vx+G.vz*G.vz)*3.6f; printf("drive probe2: %.1f kmh rpm=%.2f gear=%d\n", sp, G.rpm, G.gear); }
    snap("drive");

    // teleport to each district
    const char* names[] = {"", "taiz", "sanaa", "ibb", "aden", "hadramawt"};
    for (int di = getenv("FAST") ? 5 : 1; di <= 5; di++) {
        G.pos = G.world.spawnFor(di); G.vx = G.vz = 0; G.heading = di==5?1.5708f:0.f; G.inThrottle = 0;
        for (int k = 0; k < 45; k++) drawFrame(0.016f);
        if (di == 5) {
            printf("hadr dbg: pos=(%.1f,%.1f) cam=(%.1f,%.1f,%.1f)\n", G.pos.x, G.pos.z, G.camPos.x, G.camPos.y, G.camPos.z);
            int nb = 0;
            for (auto& b : G.world.bldgs) {
                float dx = b.x - G.pos.x, dz = b.z - G.pos.z;
                if (dx*dx + dz*dz < 90*90 && nb < 8) { printf("  bldg x=%.0f z=%.0f w=%.0f d=%.0f h=%.0f tex=%d\n", b.x, b.z, b.w, b.d, b.h, b.tex); nb++; }
            }
        }
        snap(names[di]);
    }
    if (!getenv("FAST")) {
// mission mode: rings mission in Taiz
    G.mode = 5; G.missionType = 1; G.missionCity = 1; G.missionGoal = 5; G.missionProgress = 1;
    G.missionState = 1; G.missionTime = 58; G.ringIdx = 1;
    // park the camera 25m behind ring #1, facing it
    V3 ring = G.world.intersections[1];
    G.pos = v3(ring.x - 25.f, ring.y, ring.z); G.vx = G.vz = 0;
    G.heading = 1.5708f; // fwd=(sin h,cos h)=(1,0): face +x toward ring
    G.inThrottle = 0;
    // snap the chase camera behind the car (lerp would need many frames)
    G.camPos = v3(G.pos.x - 7.6f, 3.1f, G.pos.z);
    printf("mission dbg: mode=%d state=%d type=%d ringIdx=%d nintersections=%d ringmesh_v=%d\n",
           G.mode, G.missionState, G.missionType, G.ringIdx,
           (int)G.world.intersections.size(), G.ringMesh.count);
    for (int k = 0; k < 5; k++) drawFrame(0.016f);
    printf("after frames: mode=%d state=%d ring=(%.1f,%.1f,%.1f) cam=(%.1f,%.1f)\n",
           G.mode, G.missionState, ring.x, ring.y, ring.z, G.pos.x, G.pos.z);
    snap("mission");
}

    G.inThrottle = 0; G.mode = 2; G.missionState = 0;
    printf("done. speed=%.1f kmh pos=%.1f,%.1f\n", fabsf(G.vx)*3.6f, G.pos.x, G.pos.z);
    return 0;
}
