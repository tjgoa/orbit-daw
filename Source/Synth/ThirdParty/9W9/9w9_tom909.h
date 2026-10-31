/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * 9w9_tom909.h — the TR-909's tom voice, from the voicing-board schematic.
 *
 * The 909's toms are not one oscillator with a body. Each tom is THREE
 * oscillators, a noise path, and separate envelopes — that structure is why a
 * real 909 tom has weight and a bark, and why a single swept oscillator sounds
 * thin next to it.
 *
 * From the low tom on the service notes' voicing board (IC16, a 4069 UBP
 * hex inverter, supplies all three):
 *
 *   VCO1  timing cap C18  0.022 uF        R73  47K
 *   VCO2  timing cap C19  0.033 uF        R78  47K
 *   VCO3  timing cap C20  0.012 uF        R75  47K
 *
 * Same resistor on each, so the frequencies go as 1/C and the three sit at
 * fixed ratios: taking VCO2 (the lowest) as the fundamental,
 *
 *   VCO2 : VCO1 : VCO3  =  1 : 1.50 : 2.75
 *
 * Inharmonic on purpose — a drum head's modes are not a harmonic series, and
 * this stack is what gives the voice its pitched-but-not-musical character.
 * The hi tom uses 0.022 / 0.015 / 0.0082 uF, the same ratios an octave up, so
 * one set of ratios serves all three toms and the Tune control moves them
 * together.
 *
 * Each oscillator has its own VCA transistor and envelope (ENV1/ENV2/ENV3),
 * so the partials do NOT decay together. The envelope capacitors give the
 * rates: C23 0.68 uF through the Decay pot (VR11 500K) plus R111 56K is the
 * long one, 38 ms to 380 ms; C22 0.22 uF through R110 470K is fixed at about
 * 100 ms. So the upper partials die away while the fundamental is still
 * sounding, which is exactly what a struck drum does.
 *
 * Each oscillator's triangle passes through back-to-back diodes (D20/21,
 * D22/23, D28/27) that round it toward a sine, and the NOISE bus (R114, 22K)
 * adds the stick attack — visible as the spike marked "noise" at the head of
 * the low tom's scope trace in the service notes.
 *
 * Not modelled from component values: the exact CV-to-frequency law of the
 * starved-inverter oscillator, and the diodes' precise knee. Those are fitted.
 *
 * GPL-3.0.
 */
#ifndef W9_TOM909_H
#define W9_TOM909_H

#include "webaudio.h"
#include "9w9_circuit.h"

/* VCO2 : VCO1 : VCO3 from the timing capacitors above. */
#define W9_TOM_R1  1.00f
#define W9_TOM_R2  1.50f
#define W9_TOM_R3  2.75f

typedef struct {
    wa_osc_t    osc[3];
    wa_param_t  env[3];        /* one VCA envelope per oscillator */
    wa_param_t  pitch;         /* shared sweep, as ENV4 does on the board */
    wa_param_t  noise_env;
    wa_biquad_t noise_bp;      /* the stick, not a hiss                  */
    wa_biquad_t dc_block;
    float       out_gain;
    float       noise_level;
    float       crush_st[2];
    double      mute_countdown;
    float       sample_rate;
} w9_tom_t;

static inline void w9_tom909_init(w9_tom_t *t, const float _sr)
{
    t->sample_rate = _sr;
    for(int i = 0; i < 3; ++i)
    {
        wa_osc_init(&t->osc[i], _sr);
        wa_param_init(&t->env[i], 0.0f);
    }
    wa_param_init(&t->pitch, 100.0f);
    wa_param_init(&t->noise_env, 0.0f);
    wa_biquad_set(&t->noise_bp, WA_BANDPASS, 1200.0f, 1.2f, 0.0f, _sr);
    wa_biquad_set(&t->dc_block, WA_HIGHPASS, 20.0f, 0.7071f, 0.0f, _sr);
    /* wa_biquad_set writes coefficients only. The engine happens to memset its
     * whole struct, but this init must stand alone: undefined delay-line state
     * is a NaN generator (found the hard way in a stack-allocated probe). */
    wa_biquad_reset(&t->noise_bp);
    wa_biquad_reset(&t->dc_block);
    t->out_gain = 0.0f;
    t->noise_level = 0.0f;
    t->mute_countdown = 0.0;
}

/*
 * Triggered from the voice's existing panel values (`p`), so the tom reads the
 * same Tune / Decay / Attack / Drive / Dist / Level the UI already has and no
 * new parameter has to exist for it.
 */
static inline void w9_tom909_trigger(w9_tom_t *t, const w9_bt_t *p,
                                       const float _accent)
{
    const float sr = t->sample_rate;
    const float ms = 0.001f * sr;

    /* Every hit starts from the same place — see the note in 9w9_circuit.h. */
    for(int i = 0; i < 3; ++i) wa_osc_set_phase(&t->osc[i], 0.25);
    wa_biquad_reset(&t->dc_block);

    /* Pitch envelope (ENV4 on the board): a short drop into the fundamental. */
    wa_set_value(&t->pitch, p->tune * p->sweep_depth);
    wa_exp_ramp(&t->pitch, p->tune, p->sweep_time * ms);

    /* Measured against a real 909 (Tom Lo/Mid/Hi Clean): after ~60 ms the
     * ring is very nearly a pure sine — the fundamental reads ~1150 on the
     * Goertzel where every other partial is under ~170. The two upper VCOs
     * are an ATTACK feature: they bark and get out of the way. So the Decay
     * pot drives the fundamental alone, and the partials run from fixed short
     * envelopes (their C22/R110-class RCs), which also means Decay no longer
     * changes the timbre — it lengthens the body, like the hardware pot. */
    const float d1 = p->decay;
    wa_set_value(&t->env[0], 1.0f);
    wa_exp_ramp(&t->env[0], 0.00001f, d1 * ms);
    wa_set_value(&t->env[1], 0.55f);
    wa_exp_ramp(&t->env[1], 0.00001f, 110.0f * ms);
    wa_set_value(&t->env[2], 0.45f);
    wa_exp_ramp(&t->env[2], 0.00001f, 70.0f * ms);

    /* Stick attack off the noise bus. Short: it is a transient, not a layer. */
    t->noise_level = p->attack;
    wa_set_value(&t->noise_env, 1.0f);
    wa_exp_ramp(&t->noise_env, 0.00001f, 8.0f * ms);

    t->out_gain = p->level * _accent;
    t->mute_countdown = (d1 + 60.0f) * ms;
}

static inline float w9_tom909_render(w9_tom_t *t, const w9_bt_t *p,
                                       const float _noise)
{
    if(t->mute_countdown <= 0.0) return 0.0f;
    t->mute_countdown -= 1.0;

    const float f = wa_param_tick(&t->pitch);

    /* Three oscillators at the board's ratios, each rounded by its own diode
     * pair and gated by its own envelope. */
    float o = 0.0f;
    o += w9_diode_round(wa_osc_triangle(&t->osc[0], f * W9_TOM_R1), 2.0f)
       * wa_param_tick(&t->env[0]);
    o += w9_diode_round(wa_osc_triangle(&t->osc[1], f * W9_TOM_R2), 2.0f)
       * wa_param_tick(&t->env[1]);
    o += w9_diode_round(wa_osc_triangle(&t->osc[2], f * W9_TOM_R3), 2.0f)
       * wa_param_tick(&t->env[2]);
    o *= 0.55f;                       /* three partials summed, keep headroom */

    const float stick = wa_biquad_tick(&t->noise_bp, _noise)
                      * wa_param_tick(&t->noise_env) * t->noise_level * 1.5f;

    float y = w9_shape_st(o + stick, p->drive, p->dist_type, t->crush_st);
    y = wa_biquad_tick(&t->dc_block, y);
    return y * t->out_gain;
}

#endif /* W9_TOM909_H */
