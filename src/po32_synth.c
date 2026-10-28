#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define TWO_PI 6.28318530718f
#define SAMPLE_RATE 44100.0f

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
    float base_freq = 20.0f * powf(1000.0f, p->osc_freq);
    float dt = inv_sr;

    for (uint32_t i = 0; i < n_frames; i++) {
        float mod_pitch = powf(2.0f, (p->mod_amt - 0.5f) * 16.0f * v->mod_env);
        float freq = base_freq * mod_pitch;
        v->phase += freq * dt;
        if (v->phase >= 1.0f) v->phase -= 1.0f;

        float osc = 0.0f;
        if (p->osc_wave < 0.33f)      osc = sinf(TWO_PI * v->phase);
        else if (p->osc_wave < 0.66f) osc = 2.0f * fabsf(2.0f * v->phase - 1.0f) - 1.0f;
        else                          osc = 2.0f * v->phase - 1.0f;
        osc *= v->osc_env;

        float raw_n = white_noise(&v->rng_state);
        float filtered_n = (v->b0 * raw_n) + v->z1;
        v->z1 = (v->b1 * raw_n) - (v->a1 * filtered_n) + v->z2;
        v->z2 = (v->b2 * raw_n) - (v->a2 * filtered_n);
        filtered_n *= v->noise_env;

        float s = (osc * (1.0f - p->mix)) + (filtered_n * p->mix);
        if (p->dist_amt > 0.05f) {
            s = tanhf(s * (1.0f + p->dist_amt * 8.0f));
        }
        s *= (p->level * velocity);

        float pan_l = 0.5f * (1.0f - pan);
        float pan_r = 0.5f * (1.0f + pan);
        out_l[i] += s * pan_l;
        out_r[i] += s * pan_r;

        v->osc_env   *= expf(-dt / (0.001f + p->osc_dcy * 3.0f));
        v->noise_env *= expf(-dt / (0.001f + p->n_env_dcy * 3.0f));
        v->mod_env   *= expf(-dt / (0.001f + p->mod_rate * 2.0f));

        if (v->osc_env < 0.0001f && v->noise_env < 0.0001f) {
            v->active = false;
            break;
        }
    }
}
