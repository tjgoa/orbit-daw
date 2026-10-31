/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * 9w9_pots.h — every continuous control is a 0..127 pot, like the hardware.
 *
 * Nobody dials a 909 in milliseconds. The panel has pots; you turn them and
 * listen. So the module's external parameter surface is 0..127 for everything
 * continuous, and this table maps each pot to its real engineering range with
 * a musically sensible curve.
 *
 * EXP curve: value = min * (max/min)^(pot/127). Gives fine control at the
 * bottom of time/frequency ranges, where the ear is most sensitive, and lets
 * one pot span e.g. 15..400 ms without the low end being a dead zone.
 *
 * GPL-3.0.
 */

#ifndef W9_POTS_H
#define W9_POTS_H

#include <math.h>

typedef enum { W9_LIN = 0, W9_EXP = 1 } w9_curve_t;

typedef struct {
    const char  *key;    /* same key the engine's raw setter uses */
    float        min;
    float        max;
    w9_curve_t curve;
} w9_pot_t;

/* Order is the storage order for pot values and for the state blob. */
static const w9_pot_t g_w9_pots[] = {
    /* --- Bass Drum --- */
    { "bd_c_tune",          6.0f,   32.0f, W9_LIN },
    /* From the board: ENV1 is C8 0.33 uF discharging through VR5 1M(A) plus
     * R58 47K — tau 15 ms with the pot closed, 345 ms wide open. This ramp's
     * tau is nominal/11.5, so 100..4000 nominal covers exactly that span; the
     * measured "Long" recording (tau ~120 ms, t60 760 ms) sits inside it.
     * The old 15-400 ms range topped out at a 35 ms tau and could not reach
     * a 909 kick at any setting. */
    { "bd_c_decay",       100.0f, 4000.0f, W9_EXP },
    { "bd_c_attack",        0.0f,    1.0f, W9_LIN },
    /* The two kick mod pots. P.Depth 0 = stock kick bit for bit, and Pitch is
     * inert until P.Depth opens it (RD-9 behaviour, per Gus). Pitch spans the
     * doc's 0.43x..4.7x of the usual frequency; unity sits at pot ~45. */
    { "bd_c_sweep_depth",   0.0f,    1.0f, W9_LIN },
    { "bd_c_pitch_mod",     0.43f,   4.7f, W9_EXP },
    { "bd_c_sweep_time",     8.0f,  136.0f, W9_EXP },
    /* Drive pots start at 0.85 — just under the shaping knee — instead of 0.2:
     * below unity every type is level-normalised transparent, so the old range
     * spent pots 0-52 doing nothing ("only kinda kicks in around 53"). Bite
     * now arrives by pot ~12; the top reaches half again further. */
    { "bd_c_drive",         0.85f,   12.0f, W9_EXP },
    /* Level tops out AT the output ceiling, not past it: 1.35 x accent (2.0)
     * x master volume (0.35) = 0.945 FS. Ranges that went to 2.0 (3.0 on the
     * rim and clap) put the last third of every pot into the limiter, which is
     * why turning Level up past a point stopped doing anything. */
    { "bd_c_level",         0.0f,    1.35f, W9_LIN },
    { "bd_c_click_tone",  500.0f, 8000.0f, W9_EXP },
    { "bd_c_tune2",        20.0f,  200.0f, W9_EXP },
    { "bd_c_osc2_mix",      0.0f,    1.0f, W9_LIN },

    /* --- Snare --- */
    { "sd_c_tune",         130.0f,  320.0f, W9_EXP },
    { "sd_c_decay",         60.0f,  700.0f, W9_EXP },
    { "sd_c_attack",        0.0f,    1.0f, W9_LIN },
    { "sd_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
    { "sd_c_sweep_time",    1.0f,   60.0f, W9_EXP },
    { "sd_c_drive",         0.85f,   12.0f, W9_EXP },
    { "sd_c_level",         0.0f,    1.35f, W9_LIN },
    { "sd_c_click_tone",  500.0f, 8000.0f, W9_EXP },
    { "sd_c_tune2",       150.0f,  800.0f, W9_EXP },
    { "sd_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
    { "sd_c_snappy",        0.0f,    1.0f, W9_LIN },
    { "sd_c_noise_decay",  300.0f, 4000.0f, W9_EXP },
    { "sd_c_noise_hp",    200.0f, 6000.0f, W9_EXP },

    /* --- Toms --- */
    /* Tom Tune pots are trims around each voice's own center (68.5 / 102 /
     * 131.5 Hz measured from a real 909), as the 10K VR on the board is: the
     * ranges only meet at the edges, so the toms can never trade places. */
    { "lt_c_tune",           50.0f,   90.0f, W9_EXP },
    { "lt_c_decay",          80.0f, 2600.0f, W9_EXP },
    { "lt_c_attack",        0.0f,    1.0f, W9_LIN },
    { "lt_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
    { "lt_c_sweep_time",    1.0f,  250.0f, W9_EXP },
    { "lt_c_drive",         0.85f,   12.0f, W9_EXP },
    { "lt_c_level",         0.0f,    1.35f, W9_LIN },
    { "lt_c_tune2",        20.0f,  300.0f, W9_EXP },
    { "lt_c_osc2_mix",      0.0f,    1.0f, W9_LIN },

    { "mt_c_tune",           80.0f,  125.0f, W9_EXP },
    { "mt_c_decay",          70.0f, 2200.0f, W9_EXP },
    { "mt_c_attack",        0.0f,    1.0f, W9_LIN },
    { "mt_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
    { "mt_c_sweep_time",    1.0f,  250.0f, W9_EXP },
    { "mt_c_drive",         0.85f,   12.0f, W9_EXP },
    { "mt_c_level",         0.0f,    1.35f, W9_LIN },
    { "mt_c_tune2",        30.0f,  400.0f, W9_EXP },
    { "mt_c_osc2_mix",      0.0f,    1.0f, W9_LIN },

    { "ht_c_tune",          110.0f,  170.0f, W9_EXP },
    { "ht_c_decay",          60.0f, 2000.0f, W9_EXP },
    { "ht_c_attack",        0.0f,    1.0f, W9_LIN },
    { "ht_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
    { "ht_c_sweep_time",    1.0f,  250.0f, W9_EXP },
    { "ht_c_drive",         0.85f,   12.0f, W9_EXP },
    { "ht_c_level",         0.0f,    1.35f, W9_LIN },
    { "ht_c_tune2",        50.0f,  600.0f, W9_EXP },
    { "ht_c_osc2_mix",      0.0f,    1.0f, W9_LIN },

    /* --- Rim (909 model) --- */
    { "rs_tune",          150.0f,  300.0f, W9_EXP },
    { "rs_tune2",         340.0f,  680.0f, W9_EXP },
    { "rs_res",             4.0f,   20.0f, W9_EXP },
    { "rs_decay",          40.0f,  700.0f, W9_EXP },
    { "rs_noise",           0.0f,    1.0f, W9_LIN },
    { "rs_saturation",      0.85f,   12.0f, W9_EXP },
    { "rs_volume",          0.0f,    1.35f, W9_LIN },

    /* --- Clap (909 model) --- */
    { "hc_tune",          650.0f, 1400.0f, W9_EXP },
    { "hc_spread",          2.0f,   30.0f, W9_EXP },
    { "hc_tone_decay",      2.0f,   40.0f, W9_EXP },   /* burst pulse decay */
    { "hc_decay",         120.0f, 1000.0f, W9_EXP },   /* room tail         */
    { "hc_tail",            0.0f,    1.0f, W9_LIN },
    { "hc_drive",           0.85f,   12.0f, W9_EXP },
    { "hc_volume",          0.0f,    1.35f, W9_LIN },

    /* --- Hats / cymbals --- */
    { "ohh_decay",         20.0f, 1200.0f, W9_EXP },
    { "ohh_pitch",          0.25f,   4.0f, W9_EXP },
    { "ohh_volume",         0.0f,    1.35f, W9_LIN },
    { "ohh_drive",          0.85f,   12.0f, W9_EXP },
    { "chh_decay",         15.0f,  300.0f, W9_EXP },
    { "chh_pitch",          0.25f,   4.0f, W9_EXP },
    { "chh_volume",         0.0f,    1.35f, W9_LIN },
    { "chh_drive",          0.85f,   12.0f, W9_EXP },
    { "rc_decay",         100.0f, 3000.0f, W9_EXP },
    { "rc_pitch",           0.25f,   4.0f, W9_EXP },
    { "rc_volume",          0.0f,    1.35f, W9_LIN },
    { "rc_drive",           0.85f,   12.0f, W9_EXP },
    { "cr_decay",         100.0f, 3000.0f, W9_EXP },
    { "cr_pitch",           0.25f,   4.0f, W9_EXP },
    { "cr_volume",          0.0f,    1.35f, W9_LIN },
    { "cr_drive",           0.85f,   12.0f, W9_EXP },

    /* --- Global --- */
    /* FX sends (kick has none, deliberately) and the two send FX. */
    { "sd_c_rev",           0.0f,    1.0f, W9_LIN },
    { "sd_c_dly",           0.0f,    1.0f, W9_LIN },
    { "lt_c_rev",           0.0f,    1.0f, W9_LIN },
    { "lt_c_dly",           0.0f,    1.0f, W9_LIN },
    { "mt_c_rev",           0.0f,    1.0f, W9_LIN },
    { "mt_c_dly",           0.0f,    1.0f, W9_LIN },
    { "ht_c_rev",           0.0f,    1.0f, W9_LIN },
    { "ht_c_dly",           0.0f,    1.0f, W9_LIN },
    { "rs_rev",             0.0f,    1.0f, W9_LIN },
    { "rs_dly",             0.0f,    1.0f, W9_LIN },
    { "hc_rev",             0.0f,    1.0f, W9_LIN },
    { "hc_dly",             0.0f,    1.0f, W9_LIN },
    { "ohh_rev",            0.0f,    1.0f, W9_LIN },
    { "ohh_dly",            0.0f,    1.0f, W9_LIN },
    { "chh_rev",            0.0f,    1.0f, W9_LIN },
    { "chh_dly",            0.0f,    1.0f, W9_LIN },
    { "rc_rev",             0.0f,    1.0f, W9_LIN },
    { "rc_dly",             0.0f,    1.0f, W9_LIN },
    { "cr_rev",             0.0f,    1.0f, W9_LIN },
    { "cr_dly",             0.0f,    1.0f, W9_LIN },
    { "rev_decay",          0.2f,    0.93f, W9_LIN },
    { "rev_tone",           0.0f,    1.0f, W9_LIN },
    { "rev_hpf",           30.0f,  800.0f, W9_EXP },
    { "rev_level",          0.0f,    1.2f, W9_LIN },
    { "dly_fdbk",           0.0f,    0.85f, W9_LIN },
    { "dly_tone",           0.0f,    1.0f, W9_LIN },
    { "dly_hpf",           30.0f,  800.0f, W9_EXP },
    { "dly_level",          0.0f,    1.2f, W9_LIN },
    { "master_drive",       0.85f,   12.0f, W9_EXP },
    { "master_comp",        0.0f,    1.0f, W9_LIN },
    { "accent",             1.0f,    4.0f, W9_LIN },
    { "vel_depth",          0.0f,    1.0f, W9_LIN },
    { "volume",             0.0f,    1.0f, W9_LIN },
};

#define W9_POT_COUNT ((int)(sizeof(g_w9_pots)/sizeof(g_w9_pots[0])))

/* pot (0..127) -> engineering value */
static inline float w9_pot_to_value(const w9_pot_t *p, int pot)
{
    if(pot < 0)   pot = 0;
    if(pot > 127) pot = 127;
    const float t = (float)pot / 127.0f;

    if(p->curve == W9_EXP && p->min > 0.0001f)
        return p->min * powf(p->max / p->min, t);
    return p->min + (p->max - p->min) * t;
}

/* engineering value -> nearest pot, used to seed pots from engine defaults */
static inline int w9_value_to_pot(const w9_pot_t *p, float v)
{
    if(v < p->min) v = p->min;
    if(v > p->max) v = p->max;
    float t;
    if(p->curve == W9_EXP && p->min > 0.0001f)
        t = logf(v / p->min) / logf(p->max / p->min);
    else
        t = (p->max - p->min) > 0.0f ? (v - p->min) / (p->max - p->min) : 0.0f;
    int pot = (int)(t * 127.0f + 0.5f);
    if(pot < 0)   pot = 0;
    if(pot > 127) pot = 127;
    return pot;
}

#endif /* W9_POTS_H */
