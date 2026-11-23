// Sayyaratak car model: loads packed SYCR binary (from tools/glb2car.py) into GL meshes.
// Parts: body(paint), trim, wing, wing_trim, lights, glass, wheel_FL/FR/RL/RR, steer_L/R.
#pragma once
#include "mesh.h"
#include <cstring>

struct CarPart {
    std::string name;
    int flags = 0;      // 1=glass 2=lights 4=paint 8=wing
    V3 pivot = {0,0,0};
    MeshData cpu;       // kept until uploaded
    std::vector<unsigned> idx;
    GLMesh gpu;
    bool uploaded = false;
};

struct CarModel {
    CarPart parts[12];
    int partCount = 0;
    std::vector<unsigned char> pending; // raw bytes if load called before GL
    bool loaded = false;   // CPU parse ok
    bool glReady = false;  // uploaded

    const CarPart* find(const char* n) const {
        for (int i = 0; i < partCount; i++) if (parts[i].name == n) return &parts[i];
        return nullptr;
    }

    bool load(const unsigned char* d, size_t n) {
        if (!d || n < 12) return false;
        unsigned magic, ver, count;
        memcpy(&magic, d, 4); memcpy(&ver, d+4, 4); memcpy(&count, d+8, 4);
        if (magic != 0x52435953 || ver != 1 || count == 0 || count > 12) return false;
        size_t o = 12;
        partCount = 0;
        for (unsigned p = 0; p < count; p++) {
            if (o + 4 > n) return false;
            unsigned nl; memcpy(&nl, d+o, 4); o += 4;
            if (o + nl + 28 > n) return false;
            CarPart& P = parts[partCount++];
            P.name.assign((const char*)d+o, nl); o += nl;
            unsigned flags, vc, ic; float pv[3];
            memcpy(&flags, d+o, 4); memcpy(pv, d+o+4, 12); memcpy(&vc, d+o+16, 4); memcpy(&ic, d+o+20, 4);
            o += 24;
            if (vc == 0 || vc > 200000 || ic > 600000 || o + vc*40 + ic*4 > n) { partCount--; return false; }
            P.flags = flags; P.pivot = v3(pv[0], pv[1], pv[2]);
            // expand indexed to triangle soup (GLMesh draws non-indexed)
            std::vector<unsigned> idx(ic);
            const float* src0 = (const float*)(d + o);
            memcpy(idx.data(), d + o + vc*40, ic*4);
            P.cpu.v.resize((size_t)ic * 12);
            for (unsigned k = 0; k < ic; k++) {
                unsigned vi = idx[k];
                if (vi >= vc) vi = 0;
                const float* src = src0 + vi * 10;
                float* dst = &P.cpu.v[(size_t)k*12];
                dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2];      // pos
                dst[3]=src[3]; dst[4]=src[4]; dst[5]=src[5];      // nrm
                dst[6]=0; dst[7]=0;                               // uv (unused)
                dst[8]=src[6]; dst[9]=src[7]; dst[10]=src[8]; dst[11]=src[9]; // col
            }
            o += vc * 40 + ic * 4;
        }
        loaded = true;
        return true;
    }

    // upload to GL (call with a current GL context)
    void upload() {
        if (!loaded || glReady) return;
        for (int i = 0; i < partCount; i++) {
            parts[i].gpu.upload(parts[i].cpu);
            parts[i].uploaded = true;
            parts[i].cpu.v.clear(); parts[i].cpu.v.shrink_to_fit();
        }
        glReady = true;
    }
};
