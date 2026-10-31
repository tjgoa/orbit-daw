/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * 9w9_engine.h — 9W9 drum engine.
 *
 * GPL-3.0.
 */

#ifndef W9_ENGINE_H
#define W9_ENGINE_H

#include <stdint.h>
#include "webaudio.h"
#include "9w9_circuit.h"
#include "9w9_tom909.h"
#include "9w9_pots.h"
#include "9w9_perc909.h"

/* Trigger IDs. 'chh' is not a separate voice — it retriggers the hi-hat with
 * the closed decay time. */
typedef enum {
    W9_BD = 0, W9_SD, W9_LT, W9_MT, W9_HT,
    W9_RS, W9_HC,
    W9_OHH, W9_CHH, W9_RC, W9_CR,
    W9_NUM_TRIGGERS
} w9_trigger_t;

#define W9_NUM_INSTRUMENTS 5   /* bd sd lt mt ht */
/* Open hat, ride, crash, closed hat. The closed hat is its own sampler rather
 * than a second decay on the open one, so it can have its own tuning, level and
 * drive; it plays the same buffer, and the two choke each other (see
 * w9_trigger) because they are one physical hat. */
#define W9_NUM_SAMPLERS    4   /* ohh ride crash chh */
#define W9_SAMP_OHH        0
#define W9_SAMP_RC         1
#define W9_SAMP_CR         2
#define W9_SAMP_CHH        3

/* ---- Oscillator+noise voices: BD, SD, LT, MT, HT ---- */


/* ---- Rim shot ---- */


/* ---- Hand clap ---- */


/* ---- Samplers: hi-hat, ride, crash ---- */
typedef struct {
    float  decay;         /* ms (open)     */
    float  decay_closed;  /* ms, hh only   */
    float  volume;
    float  pitch;         /* playbackRate  */
    float  rate_ratio;    /* Orbit modification: source-rate / host-rate. */
    float  drive;         /* per-sampler distortion (w9_shape) */
    float  dist_type;

    float  crush_st[2];

    const float *buffer;
    uint32_t     length;
    double       pos;
    wa_param_t   out;
    double       mute_countdown;
} w9_sampler_t;

/* ---- Send FX: one reverb, one delay, fed by per-voice sends ----
 *
 * Old-school on purpose. The reverb is a small early-digital topology (four
 * combs, two allpasses) with 12-BIT QUANTISATION IN THE FEEDBACK PATH — that
 * is where the character of the early rack units lives — and damping in the
 * loop. The delay is a 90s rack digital: mono, feedback through a darkening
 * one-pole, 12-bit write, and the time knob SLEWS so it warps instead of
 * clicking. Both take a highpass on the send input so low end stays out.
 * Sends default to zero: with them down the engine is bit-identical to one
 * that has no FX at all. The kick has no sends, deliberately. */
#define W9_DLY_MAX 88200            /* 2 s at 44.1k: a dotted half at 90 BPM */
#define W9_DLY_DIVS 13
typedef struct {
    float time_ms, fdbk, tone, hpf_hz, level;
    float divi;                       /* note division index, 0..12 */
    float bpm;                        /* fed by the host each block */
    wa_biquad_t hp;
    float buf[W9_DLY_MAX];
    int   w;
    float dcur;                       /* slewed delay, samples */
    float lp;                         /* loop damping state    */
} w9_dly_t;

typedef struct {
    float decay, tone, hpf_hz, level;
    wa_biquad_t hp;
    float comb[4][1356];
    int   cpos[4];
    float cdmp[4];                    /* per-comb damping state */
    float apb[2][556];
    int   apos[2];
} w9_verb_t;

/* ---- Master bus ---- */
typedef struct {
    /* No compressor and no limiter: a TR-909 has nothing of the kind — its voices sum through a mixer straight into the output amp. The
     * headroom comes from the gain structure instead (voice Level tops out at
     * the ceiling, master volume 0.35), and the only thing left on the bus is
     * the distortion, which is ours and optional.
     *
     * Master distortion: whole-kit drive.
     * dist_mode: 0=Off, 1=Diode, 2=Hard Clip, 3=Wavefolder, 4=Bitcrush
     * (modes 1..4 map to w9_shape types 0..3). */
    float drive;
    float dist_mode;
    float volume;

    /* Gain a FULL-velocity hit reaches — the 909's accent bus, and the level
     * this kit has always played at from a sequencer (Move sends 100+, which
     * the old accent switch put here). Velocity scales down from it, never
     * above it, so this stays the kit's reference level. */
    float accent;

    /* Velocity sensitivity, 0..1 — how far below Accent a soft hit falls.
     * 0 ignores velocity entirely and every hit plays at Accent, which is
     * exactly how the kit sounded before velocity existed. It only ever
     * attenuates: nothing gets louder than it plays at 0. */
    float vel_depth;

    /* One-knob bus glue — OURS, requested as a feature, not 909 circuitry,
     * and unlike the compressor this module once inherited it is honest
     * about it: at zero the stage is bypassed entirely (bit-identical), and
     * the knob blends threshold, ratio and auto-makeup together. */
    float crush_st[2];  /* master bitcrush decimator     */
    float comp;         /* 0..1 amount; 0 = hard bypass  */
    float comp_env_db;  /* smoothed gain reduction state */
    float comp_det;     /* rectified level follower      */
} w9_master_t;

typedef struct {
    float sample_rate;

    /* Circuit-informed BD/SD/tom models (9w9_circuit.h). */
    w9_bt_t         bt[W9_NUM_INSTRUMENTS];
    w9_tom_t        tom909[3]; /* LT/MT/HT, three oscillators each */
    w9_rim909_t     rim909;   /* circuit models (default)      */
    w9_clap909_t    clap909;
    w9_sampler_t    sampler[W9_NUM_SAMPLERS];

    w9_verb_t   verb;
    w9_dly_t    dly;
    float         send_rev[W9_NUM_TRIGGERS];
    float         send_dly[W9_NUM_TRIGGERS];

    w9_master_t master;
    wa_noise_t    noise;


    /* Pot positions (0..127) — the external parameter surface. Real
     * engineering values are derived from these via g_w9_pots. */
    unsigned char pots[W9_POT_COUNT];

    /* decoded sample data, owned by the engine */
    float   *sample_data[W9_NUM_SAMPLERS];
    uint32_t sample_len[W9_NUM_SAMPLERS];
} w9_engine_t;

/* Initialise with default parameters. module_dir may be NULL, in which
 * case the samplers stay silent (engine still runs). */
void w9_engine_init(w9_engine_t *e, float sample_rate, const char *module_dir);
void w9_engine_free(w9_engine_t *e);

/* velocity 0..127; scales the voice, reaching the Accent gain at 127. */
void w9_engine_trigger(w9_engine_t *e, w9_trigger_t which, int velocity);

/* Render mono into out (engine is mono; the plugin duplicates to stereo). */
void w9_engine_render(w9_engine_t *e, float *out, int frames);

/* Raw access in engineering units (ms, Hz, ...). Internal. */
int w9_engine_set_raw(w9_engine_t *e, const char *key, float value);
int w9_engine_get_raw(const w9_engine_t *e, const char *key, float *out);

/* Public parameter surface: pot positions 0..127, like the hardware panel.
 * Enum-style keys (dist_type, master_dist) pass through as
 * their own small integer values rather than being scaled. */
void w9_engine_seed_pots(w9_engine_t *e);
int w9_engine_set_param(w9_engine_t *e, const char *key, float pot);
int w9_engine_get_param(const w9_engine_t *e, const char *key, float *pot);

/* Self-contained state blob — required for slot autosave and User Presets. */
/* State blob tag. Blobs saved by v2.7.0 and earlier carry an older v3 tag
 * and still load (see w9_state_tag_ok), so existing patches and slot
 * autosaves keep their sound. */
#define W9_STATE_TAG     "9w9v3;"
int w9_engine_get_state(const w9_engine_t *e, char *buf, int len);
int w9_engine_set_state(w9_engine_t *e, const char *blob);

extern const char *const w9_trigger_names[W9_NUM_TRIGGERS];

#endif /* W9_ENGINE_H */

