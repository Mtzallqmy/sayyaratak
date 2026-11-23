// Sayyaratak GL layer: shader program, GL mesh, texture store.
#pragma once
#include <cstring>
#include <GLES3/gl3.h>
#include <vector>
#include <string>
#include "mesh.h"

static const char* VS = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aCol;
uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNrm;
out vec3 vN; out vec3 vW; out vec2 vUV; out vec4 vC;
void main(){
    vec4 w = uModel * vec4(aPos,1.0);
    vW = w.xyz;
    vN = uNrm * aNrm;
    vUV = aUV; vC = aCol;
    gl_Position = uMVP * vec4(aPos,1.0);
})";

static const char* FS = R"(#version 300 es
precision highp float;
precision highp int;
in vec3 vN; in vec3 vW; in vec2 vUV; in vec4 vC;
uniform vec4 uColor;
uniform vec3 uSunDir; uniform vec3 uSunCol; uniform vec3 uAmbSky; uniform vec3 uAmbGnd;
uniform vec3 uCamPos; uniform vec3 uFogCol; uniform vec2 uFog;
uniform sampler2D uTex; uniform float uUseTex; uniform float uEmissive;
uniform float uSpec; uniform float uShin; uniform float uAlphaTest; uniform float uNoFog;
uniform float uEnvRef; uniform vec2 uRes;
out vec4 frag;
void main(){
    vec4 base = uColor * vC;
    if (uUseTex > 0.5) base *= texture(uTex, vUV);
    if (uAlphaTest > 0.5 && base.a < 0.45) discard;
    vec3 n = normalize(vN);
    vec3 amb = mix(uAmbGnd, uAmbSky, n.y * 0.5 + 0.5);
    float diff = max(dot(n, uSunDir), 0.0) * 0.9 + max(dot(n, uSunDir), 0.0) * max(dot(n, uSunDir), 0.0) * 0.35;
    vec3 col = base.rgb * (amb + uSunCol * diff);
    vec3 vd = normalize(uCamPos - vW);
    vec3 h = normalize(uSunDir + vd);
    float spec = pow(max(dot(n, h), 0.0), uShin) * uSpec;
    float coat = pow(max(dot(n, h), 0.0), 256.0) * uSpec * 0.5;
    col += uSunCol * (spec + coat);
    if (uEnvRef > 0.001) {
        vec3 r = reflect(-vd, n);
        vec3 env = mix(vec3(0.95,0.75,0.5), vec3(0.25,0.45,0.75), clamp(r.y*1.4+0.35, 0.0, 1.0));
        float fres = pow(1.0 - max(dot(n, vd), 0.0), 3.0);
        col += env * fres * uEnvRef;
    }
    col += base.rgb * uEmissive * 1.6;
    float d = distance(uCamPos, vW);
    float fog = clamp((d - uFog.x) / (uFog.y - uFog.x), 0.0, 1.0) * (1.0 - uNoFog);
    col = mix(col, uFogCol, fog);
    col = col / (col + 0.55) * 1.5;
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, 1.12);
    vec2 pq = (gl_FragCoord.xy / uRes) * 2.0 - 1.0;
    col *= 1.0 - 0.30 * smoothstep(0.6, 1.9, length(pq));
    frag = vec4(col, base.a);
})";

GLuint* texIds();   // single shared texture store
std::string& glErrorLog();

struct Program {
    GLuint id = 0;
    GLint uMVP, uModel, uNrm, uColor, uSunDir, uSunCol, uAmbSky, uAmbGnd, uCamPos, uFogCol, uFog,
          uTex, uUseTex, uEmissive, uSpec, uShin, uAlphaTest, uNoFog, uEnvRef, uRes;
    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[1024]; GLsizei n = 0;
            glGetShaderInfoLog(s, sizeof(buf), &n, buf);
            glErrorLog().append("shader: ").append(buf, n).append("\n");
            glDeleteShader(s); return 0;
        }
        return s;
    }
    bool build() {
        GLuint vs = compile(GL_VERTEX_SHADER, VS);
        GLuint fs = compile(GL_FRAGMENT_SHADER, FS);
        if (!vs || !fs) return false;
        id = glCreateProgram();
        glAttachShader(id, vs); glAttachShader(id, fs);
        glLinkProgram(id);
        glDeleteShader(vs); glDeleteShader(fs);
        GLint ok = 0; glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[1024]; GLsizei n = 0;
            glGetProgramInfoLog(id, sizeof(buf), &n, buf);
            glErrorLog().append("link: ").append(buf, n).append("\n");
            return false;
        }
        uMVP = glGetUniformLocation(id, "uMVP"); uModel = glGetUniformLocation(id, "uModel");
        uNrm = glGetUniformLocation(id, "uNrm"); uColor = glGetUniformLocation(id, "uColor");
        uSunDir = glGetUniformLocation(id, "uSunDir"); uSunCol = glGetUniformLocation(id, "uSunCol");
        uAmbSky = glGetUniformLocation(id, "uAmbSky"); uAmbGnd = glGetUniformLocation(id, "uAmbGnd");
        uCamPos = glGetUniformLocation(id, "uCamPos"); uFogCol = glGetUniformLocation(id, "uFogCol");
        uFog = glGetUniformLocation(id, "uFog"); uTex = glGetUniformLocation(id, "uTex");
        uUseTex = glGetUniformLocation(id, "uUseTex"); uEmissive = glGetUniformLocation(id, "uEmissive");
        uSpec = glGetUniformLocation(id, "uSpec"); uShin = glGetUniformLocation(id, "uShin");
        uAlphaTest = glGetUniformLocation(id, "uAlphaTest"); uNoFog = glGetUniformLocation(id, "uNoFog");
        uEnvRef = glGetUniformLocation(id, "uEnvRef"); uRes = glGetUniformLocation(id, "uRes");
        return true;
    }
    void use() { glUseProgram(id); }
};

struct GLMesh {
    GLuint vbo = 0, vao = 0;
    int count = 0;
    void upload(const MeshData& d) {
        if (!vao) glGenVertexArrays(1, &vao);
        if (!vbo) glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, d.v.size() * sizeof(float), d.v.data(), GL_STATIC_DRAW);
        int stride = 12 * sizeof(float);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (void*)24);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 4, GL_FLOAT, false, stride, (void*)32);
        glBindVertexArray(0);
        count = (int)(d.v.size() / 12);
    }
    void uploadDynamic(int maxVerts) {
        if (!vao) glGenVertexArrays(1, &vao);
        if (!vbo) glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, maxVerts * 12 * sizeof(float), nullptr, GL_STREAM_DRAW);
        int stride = 12 * sizeof(float);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (void*)24);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 4, GL_FLOAT, false, stride, (void*)32);
        glBindVertexArray(0);
    }
    void stream(const std::vector<float>& d) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, d.size() * sizeof(float), d.data());
        count = (int)(d.size() / 12);
    }
    void draw() {
        if (!vao || count == 0) return;
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
    }
};

static void uploadTex(int slot, const int* px, int w, int h, bool mip) {
    GLuint* g_tex = texIds();
    if (!g_tex[slot]) glGenTextures(1, &g_tex[slot]);
    glBindTexture(GL_TEXTURE_2D, g_tex[slot]);
    // Android Bitmap ints are ARGB in memory (BGRA byte order on LE); swap to RGBA
    std::vector<unsigned> tmp(w * h);
    for (int i = 0; i < w * h; i++) {
        unsigned p = (unsigned)px[i];
        tmp[i] = (p & 0xFF00FF00u) | ((p >> 16) & 0xFFu) | ((p & 0xFFu) << 16);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (mip) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        static int aniso = -1;
        if (aniso < 0) {
            const char* ext = (const char*)glGetString(GL_EXTENSIONS);
            aniso = (ext && strstr(ext, "texture_filter_anisotropic")) ? 1 : 0;
        }
        if (aniso) {
            float mx = 4.f;
            glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &mx);
            if (mx > 8.f) mx = 8.f;
            if (mx >= 2.f) glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/, mx);
        }
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
static void bindTex(int slot) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texIds()[slot]);
}
