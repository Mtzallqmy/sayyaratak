// Sayyaratak native audio: engine synth + wind + skid + one-shots via AAudio.
#include <aaudio/AAudio.h>
#include <cmath>
#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>
#include <algorithm>

#include "audio_state.h"

class AudioEngine {
public:
    AudioState st;
    void start() {
        if (stream) { AAudioStream_requestStart(stream); st.running = true; return; }
        AAudioStreamBuilder* b = nullptr;
        if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) return;
        AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setSharingMode(b, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setChannelCount(b, 1);
        AAudioStreamBuilder_setSampleRate(b, 44100);
        AAudioStreamBuilder_setDataCallback(b, &AudioEngine::cb, this);
        AAudioStream* s = nullptr;
        if (AAudioStreamBuilder_openStream(b, &s) == AAUDIO_OK) {
            stream = s;
            AAudioStream_requestStart(stream);
            st.running = true;
        }
        AAudioStreamBuilder_delete(b);
    }
    void stop() {
        st.running = false;
        if (stream) AAudioStream_requestStop(stream);
    }
    void shutdown() {
        st.running = false;
        if (stream) { AAudioStream_close(stream); stream = nullptr; }
    }
    void oneshot(int type) { // 0 click, 1 chime, 2 thud
        std::lock_guard<std::mutex> lk(mu);
        if (shots.size() < 8) shots.push_back({type, 0.f});
    }
private:
    AAudioStream* stream = nullptr;
    struct Shot { int type; float t; };
    std::vector<Shot> shots;
    std::mutex mu;
    float ph1 = 0.f, ph2 = 0.f, ph3 = 0.f;   // engine phases
    float lp = 0.f, lp2 = 0.f;               // lowpass state
    float lpw = 0.f;                          // wind lowpass
    unsigned rng = 12345u;

    float noise() {
        rng = rng * 1664525u + 1013904223u;
        return ((rng >> 9) & 0xFFFF) / 32768.f - 1.f;
    }

    static aaudio_data_callback_result_t cb(AAudioStream*, void* user, void* data, int32_t frames) {
        return ((AudioEngine*)user)->render((int16_t*)data, frames);
    }

    aaudio_data_callback_result_t render(int16_t* out, int32_t frames) {
        const float sr = 44100.f;
        bool on = st.enabled.load() && st.running.load();
        float rpm = st.rpm.load();
        float thr = st.throttle.load();
        float spd = st.speed01.load();
        float slip = st.slip.load();
        int prof = st.profile.load();
        float base = prof == 0 ? 42.f : (prof == 1 ? 55.f : 34.f);
        float bright = prof == 0 ? 0.05f : (prof == 1 ? 0.16f : 0.10f);
        float drive = prof == 2 ? 1.8f : 1.0f;
        float f = base * (0.7f + rpm * 2.4f);
        float engGain = 0.10f + thr * 0.30f + rpm * 0.10f;
        float windGain = spd * spd * 0.16f;
        float skidGain = slip > 0.35f ? (slip - 0.35f) * 1.0f * (spd > 0.15f ? 1.f : 0.f) : 0.f;
        for (int i = 0; i < frames; i++) {
            float s = 0.f;
            if (on) {
                ph1 += f / sr; if (ph1 > 1.f) ph1 -= 1.f;
                ph2 += f * 1.494f / sr; if (ph2 > 1.f) ph2 -= 1.f;
                ph3 += f * 0.5f / sr; if (ph3 > 1.f) ph3 -= 1.f;
                float saw1 = ph1 * 2.f - 1.f;
                float saw2 = ph2 * 2.f - 1.f;
                float sub = sinf(ph3 * 6.2831853f);
                float eng = saw1 * 0.45f + saw2 * 0.25f + sub * 0.45f;
                lp += bright * (eng - lp);
                lp2 += bright * 3.f * (lp - lp2);
                float e = lp2 * drive;
                e = tanhf(e) * engGain;
                // wind
                lpw += 0.03f * (noise() - lpw);
                float w = lpw * 6.f * windGain;
                // skid: brighter noise
                float sk = noise() * 0.5f + (noise() - lpw) * 0.5f;
                s = e + w + sk * skidGain * 0.35f;
                // one-shots
                mu.lock();
                for (size_t k = 0; k < shots.size(); k++) {
                    Shot& sh = shots[k];
                    float x = 0.f;
                    if (sh.type == 0) x = sinf(sh.t * 1400.f * 6.2831853f) * expf(-sh.t * 40.f) * 0.3f;
                    else if (sh.type == 1) {
                        float fr = sh.t < 0.14f ? 880.f : 1318.f;
                        x = sinf(sh.t * fr * 6.2831853f) * expf(-sh.t * 8.f) * 0.35f;
                        if (sh.t > 0.35f) x = 0.f;
                    } else if (sh.type == 3) {
                        // car horn: dual-tone 370 + 460 Hz, held 0.45 s
                        float env = sh.t < 0.45f ? (1.f - sh.t * 0.4f) : 0.f;
                        float v = sinf(sh.t * 370.f * 6.2831853f) + sinf(sh.t * 462.f * 6.2831853f)
                                + 0.5f * sinf(sh.t * 740.f * 6.2831853f);
                        x = tanhf(v * 1.4f) * 0.30f * env;
                    } else {
                        float n = noise();
                        lp += 0.f; // keep lp state stable
                        x = n * expf(-sh.t * 18.f) * 0.5f * (sh.t < 0.18f ? 1.f : 0.f);
                    }
                    s += x;
                    sh.t += 1.f / sr;
                }
                shots.erase(std::remove_if(shots.begin(), shots.end(),
                    [](const Shot& sh){ return sh.t > 0.6f; }), shots.end());
                mu.unlock();
            }
            if (s > 1.f) s = 1.f; else if (s < -1.f) s = -1.f;
            out[i] = (int16_t)(s * 30000.f);
        }
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
};

static AudioEngine g_audio;
AudioState& audioState() { return g_audio.st; }
void audioStart() { g_audio.start(); }
void audioStop() { g_audio.stop(); }
void audioOneshot(int t) { g_audio.oneshot(t); }
