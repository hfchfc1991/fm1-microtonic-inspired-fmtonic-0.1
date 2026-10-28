#include <stdint.h>
#include <stdbool.h>

#define TWO_PI 6.28318530718f
#define SAMPLE_RATE 44100.0f

// --- Embedded fast math routines (no math.h required) ---
static inline float fast_abs(float x) {
    return (x < 0.0f) ? -x : x;
}

static inline float fast_sin(float x) {
    while (x > 3.14159265f) x -= 6.2831853f;
    while (x < -3.14159265f) x += 6.2831853f;
    float a = x * (3.14159265f - fast_abs(x));
    return (16.0f * a) / (49.348022f - 4.0f * fast_abs(a));
}

static inline float fast_exp(float x) {
    if (x < -8.0f) return 0.0f;
    float p = 1.0f + x * 0.125f;
    p *= p; p *= p; p *= p;
    return (p > 0.0f) ? p : 0.0f;
}

static inline float fast_pow2(float x) {
    return fast_exp(x * 0.69314718f);
}

static inline float fast_pow1000(float x) {
    return fast_pow2(x * 9.965784f);
}

static inline float fast_tanh(float x) {
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

// --- Synth Engine ---
typedef struct {
    float osc_wave, osc_freq, osc_atk, osc_dcy;
    float mod_mode, mod_rate, mod_amt;
    float n_fil_mod, n_fil_frq, n_fil_q;
    float n_env_mod, n_env_atk, n_env_dcy;
    float mix, dist_amt, eq_freq, eq_gain, level;
    float osc_vel, n_vel, mod_vel;
} DrumPatch;

typedef struct {
    float phase;
    float osc_env;
    float noise_env;
    float mod_env;
    float b0, b1, b2, a1, a2;
    float z1, z2;
    uint32_t rng_state;
    bool active;
} VoiceState;

static inline float white_noise(uint32_t *state) {
    *state = (*state * 1103515245 + 12345) & 0x7FFFFFFF;
    return ((float)*state / 1073741824.0f) - 1.0f;
}

void render_drum_voice(const DrumPatch *p, VoiceState *v, float *out_l, float *out_r, uint32_t n_frames, float velocity, float pan) {
    if (!v->active) return;
    
    float inv_sr = 1.0f / SAMPLE_RATE;
    float base_freq = 20.0f * fast_pow1000(p->osc_freq);
    float dt = inv_sr;

    for (uint32_t i = 0; i < n_frames; i++) {
        float mod_pitch = fast_pow2((p->mod_amt - 0.5f) * 16.0f * v->mod_env);
        float freq = base_freq * mod_pitch;
        v->phase += freq * dt;
        if (v->phase >= 1.0f) v->phase -= 1.0f;

        float osc = 0.0f;
        if (p->osc_wave < 0.33f)      osc = fast_sin(TWO_PI * v->phase);
        else if (p->osc_wave < 0.66f) osc = 2.0f * fast_abs(2.0f * v->phase - 1.0f) - 1.0f;
        else                          osc = 2.0f * v->phase - 1.0f;
        osc *= v->osc_env;

        float raw_n = white_noise(&v->rng_state);
        float filtered_n = (v->b0 * raw_n) + v->z1;
        v->z1 = (v->b1 * raw_n) - (v->a1 * filtered_n) + v->z2;
        v->z2 = (v->b2 * raw_n) - (v->a2 * filtered_n);
        filtered_n *= v->noise_env;

        float s = (osc * (1.0f - p->mix)) + (filtered_n * p->mix);
        if (p->dist_amt > 0.05f) {
            s = fast_tanh(s * (1.0f + p->dist_amt * 8.0f));
        }
        s *= (p->level * velocity);

        float pan_l = 0.5f * (1.0f - pan);
        float pan_r = 0.5f * (1.0f + pan);
        out_l[i] += s * pan_l;
        out_r[i] += s * pan_r;

        v->osc_env   *= fast_exp(-dt / (0.001f + p->osc_dcy * 3.0f));
        v->noise_env *= fast_exp(-dt / (0.001f + p->n_env_dcy * 3.0f));
        v->mod_env   *= fast_exp(-dt / (0.001f + p->mod_rate * 2.0f));

        if (v->osc_env < 0.0001f && v->noise_env < 0.0001f) {
            v->active = false;
            break;
        }
    }
}

// Firmware entry point for the linker
void _start(void) {
    while (1) { }
}
