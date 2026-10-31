/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * 9w9_perc909.h — circuit-informed TR-909 RIM SHOT and HAND CLAP.
 *
 * The hardware:
 *
 *  RIM SHOT — a trigger pulse shock-excites a high-Q resonant network. Very
 *  short, and its character is the *pair* of resonances (a low "tock" body and
 *  a high "tick" edge) ringing together, plus a little noise for the transient.
 *
 *  HAND CLAP — one noise source through a bandpass, gated by TWO envelopes
 *  summed: a burst of three fast pulses roughly 10 ms apart (the "hands"), and
 *  a longer decaying tail (the room). That two-part structure is what makes a
 *  909 clap sound like a clap rather than a noise blip.
 *
 * Hats, ride and crash stay sampled — the real 909's cymbals are 6-bit PCM, so
 * sampling them is accurate, not a shortcut.
 *
 * GPL-3.0.
 */

#ifndef W9_PERC909_H
#define W9_PERC909_H

#include "webaudio.h"
#include "9w9_circuit.h"   /* w9_shape() */

/* ===================== RIM SHOT ===================== */
typedef struct {
    float tune;        /* Hz, low resonance ("tock")  */
    float tune2;       /* Hz, high resonance ("tick") */
    float res;         /* Q of both resonators        */
    float decay;       /* ms                          */
    float noise_mix;   /* 0..1 transient noise        */
    float drive;
    float dist_type;
    float level;

    wa_biquad_t bp1, bp2, hp;
    wa_param_t  amp;
    float       crush_st[2];
    int         impulse;
    double      mute_countdown;
    float       sample_rate;
    float       accent_gain_;
} w9_rim909_t;

static inline void w9_rim909_init(w9_rim909_t *v, const float _sr)
{
    v->sample_rate = _sr;
    wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune  > 0 ? v->tune  : 220.0f,
                  v->res > 0 ? v->res : 12.0f, 0.0f, _sr);
    /* Both resonances measure Q ~10-11 on a real rim (ring times 15.5 ms at
     * 210 Hz and 7.5 ms at 480 Hz — the upper one dies sooner because it is
     * higher, not because it is damped harder). The 0.7 factor here made it
     * die too slowly relative to the body. */
    wa_biquad_set(&v->bp2, WA_BANDPASS, v->tune2 > 0 ? v->tune2 : 480.0f,
                  v->res > 0 ? v->res * 1.1f : 11.0f, 0.0f, _sr);
    wa_biquad_set(&v->hp, WA_HIGHPASS, 120.0f, 0.7071f, 0.0f, _sr);
    wa_param_init(&v->amp, 0.0f);
    v->impulse = 0;
    v->mute_countdown = 0.0;
    v->accent_gain_ = 1.0f;
}

static inline void w9_rim909_retune(w9_rim909_t *v)
{
    wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
    wa_biquad_set(&v->bp2, WA_BANDPASS, v->tune2, v->res * 1.1f, 0.0f, v->sample_rate);
}

static inline void w9_rim909_trigger(w9_rim909_t *v, const float _accent)
{
    const float ms = 0.001f * v->sample_rate;
    wa_set_value(&v->amp, 1.0f);
    wa_exp_ramp(&v->amp, 0.00001f, v->decay * ms);
    v->impulse = 2;                        /* the trigger pulse */
    v->accent_gain_ = _accent;
    v->mute_countdown = (v->decay + 20.0f) * ms;
}

static inline float w9_rim909_render(w9_rim909_t *v, const float _noise)
{
    if(v->mute_countdown <= 0.0) return 0.0f;
    v->mute_countdown -= 1.0;

    /* The trigger pulse shock-excites the network and then nothing drives it
     * — it rings down on its own. Noise belongs IN the pulse (a strike is
     * broadband for an instant), never after it: feeding noise continuously,
     * as this did, turns the voice into two filters ringing on hiss for the
     * whole envelope, which is audibly not a rim shot and shows up as HF the
     * real drum does not have (1180 Hz measures 3 against 56 at 210 Hz). */
    float exc = 0.0f;
    if(v->impulse > 0)
    {
        exc += 1.0f + _noise * v->noise_mix;
        v->impulse--;
    }

    const float a = wa_param_tick(&v->amp);
    /* Makeup: a high-Q bandpass fed a two-sample impulse puts out very little,
     * so the resonators need gain to sit level with the other voices. */
    /* Measured on a real 909 rim, the upper resonance sits at about half the
     * lower one (56 vs 29 at 2 ms). Equal-ish gains made the edge dominate. */
    /* Rebalanced once the noise stopped driving the pair: the upper resonator
     * had been living off that hiss, and lost most of its level when it went. */
    float y = wa_biquad_tick(&v->bp1, exc) * 60.0f
            + wa_biquad_tick(&v->bp2, exc) * 32.0f;
    y = w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);
    y = wa_biquad_tick(&v->hp, y);
    return y * v->level * v->accent_gain_;
}

/* ===================== HAND CLAP ===================== */
typedef struct {
    float tune;         /* Hz, bandpass centre       */
    float res;          /* Q                         */
    float spread;       /* ms between the 3 pulses   */
    float burst_decay;  /* ms, each pulse            */
    float tail_decay;   /* ms, the room tail         */
    float tail_level;   /* 0..1                      */
    float drive;
    float dist_type;
    float level;

    wa_biquad_t bp, hp;
    wa_param_t  burst, tail;
    float       crush_st[2];
    int         pulse_index;
    double      next_pulse;
    double      mute_countdown;
    float       sample_rate;
    float       accent_gain_;
} w9_clap909_t;

static inline void w9_clap909_init(w9_clap909_t *v, const float _sr)
{
    v->sample_rate = _sr;
    wa_biquad_set(&v->bp, WA_BANDPASS, v->tune > 0 ? v->tune : 1000.0f,
                  v->res > 0 ? v->res : 2.5f, 0.0f, _sr);
    wa_biquad_set(&v->hp, WA_HIGHPASS, 400.0f, 0.7071f, 0.0f, _sr);
    wa_param_init(&v->burst, 0.0f);
    wa_param_init(&v->tail, 0.0f);
    v->pulse_index = 4;
    v->mute_countdown = 0.0;
    v->accent_gain_ = 1.0f;
}

static inline void w9_clap909_retune(w9_clap909_t *v)
{
    wa_biquad_set(&v->bp, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
}

static inline void w9_clap909_trigger(w9_clap909_t *v, const float _accent)
{
    const float ms = 0.001f * v->sample_rate;
    v->pulse_index = 0;
    v->next_pulse = 0.0;
    /* The tail is armed by the FOURTH event (see render), not here: on the
     * measured clap the three pre-echoes RISE into the main hit at ~3 spreads
     * and the room tail hangs off that hit — a tail from t=0 puts energy
     * under the echoes and smears the gaps that make it read as hands. */
    wa_set_value(&v->tail, 0.0f);
    v->accent_gain_ = _accent;
    v->mute_countdown = (v->tail_decay + v->spread * 4.0f + 40.0f) * ms;
}

static inline float w9_clap909_render(w9_clap909_t *v, const float _noise)
{
    if(v->mute_countdown <= 0.0) return 0.0f;
    v->mute_countdown -= 1.0;

    const float ms = 0.001f * v->sample_rate;

    /* Four events, measured off the real drum: three pre-echoes RISING in
     * level (0.62 / 0.70 / 0.86, ~12 ms apart), then the main clap at full —
     * and the room tail starts THERE. */
    if(v->pulse_index < 4)
    {
        if(v->next_pulse <= 0.0)
        {
            /* Echo levels compensate for the tail that rides under the main
             * hit's analysis window — raw 0.62/0.70/0.86 relative to (main +
             * tail) once rendered. */
            static const float amp[4] = { 1.22f, 1.35f, 1.68f, 1.0f };
            wa_set_value(&v->burst, amp[v->pulse_index]);
            wa_exp_ramp(&v->burst, 0.00001f, v->burst_decay * ms);
            if(v->pulse_index == 3)
            {
                wa_set_value(&v->tail, v->tail_level);
                wa_exp_ramp(&v->tail, 0.00001f, v->tail_decay * ms);
            }
            v->pulse_index++;
            v->next_pulse = v->spread * ms;
        }
        v->next_pulse -= 1.0;
    }

    const float env = wa_param_tick(&v->burst) + wa_param_tick(&v->tail);
    float y = wa_biquad_tick(&v->bp, _noise) * env;
    y = w9_shape_st(y, v->drive, v->dist_type, v->crush_st);
    y = wa_biquad_tick(&v->hp, y);
    return y * v->level * v->accent_gain_;
}

#endif /* W9_PERC909_H */
