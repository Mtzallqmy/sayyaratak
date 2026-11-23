// Sayyaratak: math + mesh builders (header-only). Vertex layout: pos3 nrm3 uv2 col4 (12 floats).
#pragma once
#include <cmath>
#include <vector>
#include <cstdint>

struct V3 { float x, y, z; };
static inline V3 v3(float x, float y, float z) { return {x, y, z}; }
static inline V3 vadd(V3 a, V3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline V3 vsub(V3 a, V3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline V3 vscale(V3 a, float s) { return {a.x*s, a.y*s, a.z*s}; }
static inline V3 vcross(V3 a, V3 b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
static inline float vdot(V3 a, V3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline V3 vnorm(V3 a) { float l = sqrtf(vdot(a,a)); return l > 1e-6f ? vscale(a, 1.f/l) : v3(0,1,0); }

// column-major 4x4
struct M4 {
    float m[16];
    static M4 ident() { M4 r; for (int i=0;i<16;i++) r.m[i]=0; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.f; return r; }
    static M4 mul(const M4& a, const M4& b) {
        M4 r;
        for (int c=0;c<4;c++) for (int row=0;row<4;row++) {
            float s=0; for (int k=0;k<4;k++) s += a.m[k*4+row]*b.m[c*4+k];
            r.m[c*4+row]=s;
        }
        return r;
    }
    static M4 translate(float x, float y, float z) { M4 r=ident(); r.m[12]=x; r.m[13]=y; r.m[14]=z; return r; }
    static M4 scale(float x, float y, float z) { M4 r=ident(); r.m[0]=x; r.m[5]=y; r.m[10]=z; return r; }
    static M4 rotY(float a) { M4 r=ident(); float c=cosf(a),s=sinf(a); r.m[0]=c; r.m[2]=-s; r.m[8]=s; r.m[10]=c; return r; }
    static M4 rotX(float a) { M4 r=ident(); float c=cosf(a),s=sinf(a); r.m[5]=c; r.m[6]=s; r.m[9]=-s; r.m[10]=c; return r; }
    static M4 rotZ(float a) { M4 r=ident(); float c=cosf(a),s=sinf(a); r.m[0]=c; r.m[1]=s; r.m[4]=-s; r.m[5]=c; return r; }
    static M4 perspective(float fovY, float aspect, float zn, float zf) {
        M4 r; for (int i=0;i<16;i++) r.m[i]=0;
        float t = 1.f/tanf(fovY*0.5f);
        r.m[0]=t/aspect; r.m[5]=t; r.m[10]=(zf+zn)/(zn-zf); r.m[11]=-1.f; r.m[14]=2.f*zf*zn/(zn-zf);
        return r;
    }
    static M4 lookAt(V3 eye, V3 at, V3 up) {
        V3 f = vnorm(vsub(at, eye));
        V3 s = vnorm(vcross(f, up));
        V3 u = vcross(s, f);
        M4 r = ident();
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;
        r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12]=-vdot(s,eye); r.m[13]=-vdot(u,eye); r.m[14]=vdot(f,eye);
        return r;
    }
    V3 xform(V3 p) const {
        return { m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12], m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13], m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14] };
    }
};

struct MeshData { std::vector<float> v; };

static inline void vert(MeshData& m, V3 p, V3 n, float u, float vv, float r, float g, float b, float a) {
    m.v.insert(m.v.end(), {p.x,p.y,p.z, n.x,n.y,n.z, u,vv, r,g,b,a});
}
static inline void tri(MeshData& m, V3 a, V3 b, V3 c, V3 n, float col[4], float uv[6]) {
    vert(m, a, n, uv[0], uv[1], col[0],col[1],col[2],col[3]);
    vert(m, b, n, uv[2], uv[3], col[0],col[1],col[2],col[3]);
    vert(m, c, n, uv[4], uv[5], col[0],col[1],col[2],col[3]);
}
static inline void quadN(MeshData& m, V3 a, V3 b, V3 c, V3 d, V3 n, const float col[4], float us=1.f, float vs=1.f) {
    float uv[12] = {0,0, us,0, us,vs, 0,0, us,vs, 0,vs};
    tri(m, a,b,c, n, (float*)col, uv); tri(m, a,c,d, n, (float*)col, uv+6);
}
// auto-normal quad
static inline void quad(MeshData& m, V3 a, V3 b, V3 c, V3 d, const float col[4], float us=1.f, float vs=1.f) {
    V3 n = vnorm(vcross(vsub(b,a), vsub(d,a)));
    quadN(m, a,b,c,d, n, col, us, vs);
}

// box centered at (cx,cy,cz) with sizes; per-face normals
static inline void box(MeshData& m, float cx, float cy, float cz, float sx, float sy, float sz, const float col[4], float us=1.f, float vs=1.f) {
    float x=sx/2, y=sy/2, z=sz/2;
    V3 p[8] = {
        {cx-x,cy-y,cz-z},{cx+x,cy-y,cz-z},{cx+x,cy+y,cz-z},{cx-x,cy+y,cz-z},
        {cx-x,cy-y,cz+z},{cx+x,cy-y,cz+z},{cx+x,cy+y,cz+z},{cx-x,cy+y,cz+z}};
    quadN(m, p[4],p[5],p[6],p[7], v3(0,0,1), col, us, vs);
    quadN(m, p[1],p[0],p[3],p[2], v3(0,0,-1), col, us, vs);
    quadN(m, p[5],p[1],p[2],p[6], v3(1,0,0), col, us, vs);
    quadN(m, p[0],p[4],p[7],p[3], v3(-1,0,0), col, us, vs);
    quadN(m, p[7],p[6],p[2],p[3], v3(0,1,0), col, us, vs);
    quadN(m, p[0],p[1],p[5],p[4], v3(0,-1,0), col, us, vs);
}

// hexahedron from 8 corners (bottom 4 ccw seen from +y: b0..b3, top 4: t0..t3)
static inline void hex8(MeshData& m, V3 b0, V3 b1, V3 b2, V3 b3, V3 t0, V3 t1, V3 t2, V3 t3, const float col[4], float us=1.f, float vs=1.f) {
    quad(m, b3,b2,t2,t3, col, us, vs); // back (-z side depends on winding)
    quad(m, b1,b0,t0,t1, col, us, vs); // front
    quad(m, b0,b3,t3,t0, col, us, vs); // left
    quad(m, b2,b1,t1,t2, col, us, vs); // right
    quad(m, t0,t3,t2,t1, col, us, vs); // top
    quad(m, b0,b1,b2,b3, col, us, vs); // bottom
}

// cylinder around Y axis
static inline void cylY(MeshData& m, float cx, float cy, float cz, float r, float h, int seg, const float col[4], bool caps=true) {
    float y0 = cy - h/2, y1 = cy + h/2;
    for (int i=0;i<seg;i++) {
        float a0 = i/(float)seg*6.2831853f, a1 = (i+1)/(float)seg*6.2831853f;
        V3 n0 = v3(cosf(a0),0,sinf(a0)), n1 = v3(cosf(a1),0,sinf(a1));
        V3 p00 = v3(cx+n0.x*r, y0, cz+n0.z*r), p01 = v3(cx+n1.x*r, y0, cz+n1.z*r);
        V3 p10 = v3(cx+n0.x*r, y1, cz+n0.z*r), p11 = v3(cx+n1.x*r, y1, cz+n1.z*r);
        vert(m, p00, n0, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p01, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p11, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p00, n0, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p11, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p10, n0, 0,0, col[0],col[1],col[2],col[3]);
        if (caps) {
            vert(m, v3(cx,y1,cz), v3(0,1,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p10, v3(0,1,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p11, v3(0,1,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, v3(cx,y0,cz), v3(0,-1,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p01, v3(0,-1,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p00, v3(0,-1,0), 0,0, col[0],col[1],col[2],col[3]);
        }
    }
}

// cylinder around X axis (wheels)
static inline void cylX(MeshData& m, float cx, float cy, float cz, float r, float w, int seg, const float col[4], bool caps=true) {
    float x0 = cx - w/2, x1 = cx + w/2;
    for (int i=0;i<seg;i++) {
        float a0 = i/(float)seg*6.2831853f, a1 = (i+1)/(float)seg*6.2831853f;
        V3 n0 = v3(0,cosf(a0),sinf(a0)), n1 = v3(0,cosf(a1),sinf(a1));
        V3 p00 = v3(x0, cy+n0.y*r, cz+n0.z*r), p01 = v3(x0, cy+n1.y*r, cz+n1.z*r);
        V3 p10 = v3(x1, cy+n0.y*r, cz+n0.z*r), p11 = v3(x1, cy+n1.y*r, cz+n1.z*r);
        vert(m, p00, n0, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p01, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p11, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p00, n0, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p11, n1, 0,0, col[0],col[1],col[2],col[3]);
        vert(m, p10, n0, 0,0, col[0],col[1],col[2],col[3]);
        if (caps) {
            vert(m, v3(x1,cy,cz), v3(1,0,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p10, v3(1,0,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p11, v3(1,0,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, v3(x0,cy,cz), v3(-1,0,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p01, v3(-1,0,0), 0,0, col[0],col[1],col[2],col[3]);
            vert(m, p00, v3(-1,0,0), 0,0, col[0],col[1],col[2],col[3]);
        }
    }
}

static inline void cone(MeshData& m, float cx, float cy, float cz, float r, float h, int seg, const float col[4]) {
    float uvx[6] = {0,0,1,0,0.5f,1};
    float y0 = cy, y1 = cy + h;
    V3 apex = v3(cx, y1, cz);
    for (int i=0;i<seg;i++) {
        float a0 = i/(float)seg*6.2831853f, a1 = (i+1)/(float)seg*6.2831853f;
        V3 p0 = v3(cx+cosf(a0)*r, y0, cz+sinf(a0)*r);
        V3 p1 = v3(cx+cosf(a1)*r, y0, cz+sinf(a1)*r);
        V3 n = vnorm(vcross(vsub(p1,p0), vsub(apex,p0)));
        tri(m, p0, p1, apex, n, (float*)col, uvx);
        tri(m, v3(cx,y0,cz), p1, p0, v3(0,-1,0), (float*)col, uvx);
    }
}

// torus in XZ plane (ring standing up: rotate after)
static inline void torus(MeshData& m, float R, float r, int seg, int tseg, const float col[4]) {
    for (int i=0;i<seg;i++) {
        float u0 = i/(float)seg*6.2831853f, u1 = (i+1)/(float)seg*6.2831853f;
        for (int j=0;j<tseg;j++) {
            float v0 = j/(float)tseg*6.2831853f, v1 = (j+1)/(float)tseg*6.2831853f;
            auto pt = [&](float u, float v) {
                return v3((R + r*cosf(v))*cosf(u), r*sinf(v), (R + r*cosf(v))*sinf(u));
            };
            V3 p00=pt(u0,v0), p01=pt(u1,v0), p10=pt(u0,v1), p11=pt(u1,v1);
            V3 n = vnorm(vcross(vsub(p01,p00), vsub(p10,p00)));
            quadN(m, p00,p01,p11,p10, n, col);
        }
    }
}

// 5-point star extruded, facing +z
static inline void star(MeshData& m, float R, float r, float depth, const float col[4]) {
    float uvx[6] = {0,0,1,0,0.5f,1};
    V3 out[5], in[5];
    for (int i=0;i<5;i++) {
        float a = i/5.f*6.2831853f - 3.14159265f/2;
        float b = a + 6.2831853f/10;
        out[i] = v3(cosf(a)*R, sinf(a)*R, depth/2);
        in[i]  = v3(cosf(b)*r, sinf(b)*r, depth/2);
    }
    V3 nf = v3(0,0,1), nb = v3(0,0,-1);
    for (int i=0;i<5;i++) {
        int n = (i+1)%5;
        tri(m, v3(0,0,depth/2), out[i], in[i], nf, (float*)col, uvx);
        tri(m, v3(0,0,depth/2), in[i], out[n], nf, (float*)col, uvx);
        V3 of = out[i], of2 = out[n];
        tri(m, v3(0,0,-depth/2), in[i], out[i], nb, (float*)col, uvx);
        tri(m, v3(0,0,-depth/2), out[n], in[i], nb, (float*)col, uvx);
        // side
        V3 a0 = v3(of.x, of.y, -depth/2), a1 = v3(of.x, of.y, depth/2);
        V3 b0 = v3(in[i].x, in[i].y, -depth/2), b1 = v3(in[i].x, in[i].y, depth/2);
        quad(m, a0, b0, b1, a1, col);
        V3 c0 = v3(in[i].x, in[i].y, -depth/2), c1 = v3(in[i].x, in[i].y, depth/2);
        V3 d0 = v3(of2.x, of2.y, -depth/2), d1 = v3(of2.x, of2.y, depth/2);
        quad(m, c0, d0, d1, c1, col);
    }
}
