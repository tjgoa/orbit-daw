/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * 9w9_engine.c — 9W9 drum voices, sample playback, sends and master bus.
 *
 * GPL-3.0.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9w9_engine.h"

const char *const w9_trigger_names[W9_NUM_TRIGGERS] = {
    "bd", "sd", "lt", "mt", "ht", "rs", "hc", "ohh", "chh", "rc", "cr"
};

static inline float ms_to_samples(const float _ms, const float _sr)
{
    return _ms * 0.001f * _sr;
}

/* ===================================================================== */
/* WAV loading (mono 16/24-bit PCM)                                    */
/* ===================================================================== */

static float *load_wav_mono(const char *_path, uint32_t *_out_len)
{
    *_out_len = 0;
    FILE *f = fopen(_path, "rb");
    if(!f) return NULL;

    unsigned char hdr[12];
    if(fread(hdr, 1, 12, f) != 12 ||
       memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
    { fclose(f); return NULL; }

    uint16_t channels = 1, bits = 16;
    float *out = NULL;

    for(;;)
    {
        unsigned char ch[8];
        if(fread(ch, 1, 8, f) != 8) break;
        const uint32_t size = (uint32_t)ch[4] | ((uint32_t)ch[5] << 8) |
                              ((uint32_t)ch[6] << 16) | ((uint32_t)ch[7] << 24);

        if(!memcmp(ch, "fmt ", 4))
        {
            unsigned char fmt[16];
            const uint32_t want = size < 16 ? size : 16;
            if(fread(fmt, 1, want, f) != want) break;
            channels = (uint16_t)(fmt[2] | (fmt[3] << 8));
            bits     = (uint16_t)(fmt[14] | (fmt[15] << 8));
            if(size > want) fseek(f, (long)(size - want), SEEK_CUR);
        }
        else if(!memcmp(ch, "data", 4))
        {
            const uint32_t bytes_per = (uint32_t)(bits / 8) * channels;
            if(bytes_per == 0) break;
            const uint32_t frames = size / bytes_per;
            unsigned char *raw = (unsigned char*)malloc(size);
            if(!raw) break;
            if(fread(raw, 1, size, f) != size) { free(raw); break; }

            out = (float*)calloc(frames ? frames : 1, sizeof(float));
            if(!out) { free(raw); break; }

            for(uint32_t i=0; i<frames; ++i)
            {
                const unsigned char *p = raw + (size_t)i * bytes_per; /* ch 0 */
                int32_t v = 0;
                if(bits == 16)
                    v = (int16_t)(p[0] | (p[1] << 8));
                else if(bits == 24)
                    v = (int32_t)((uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 |
                                  (uint32_t)p[2] << 24) >> 8;
                else if(bits == 32)
                    v = (int32_t)((uint32_t)p[0] | (uint32_t)p[1] << 8 |
                                  (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24);
                const float scale = bits == 16 ? 32768.0f : 8388608.0f;
                out[i] = (float)v / (bits == 32 ? 2147483648.0f : scale);
            }
            free(raw);
            *_out_len = frames;
            break;
        }
        else
        {
            fseek(f, (long)(size + (size & 1)), SEEK_CUR);
        }
    }

    fclose(f);
    return out;
}

/* ===================================================================== */
/* Init                                                                   */
/* ===================================================================== */

static void w9_dly_retime(w9_dly_t *d, float _sr);

void w9_engine_init(w9_engine_t *e, const float _sr, const char *_module_dir)
{
    memset(e, 0, sizeof(*e));
    e->sample_rate = _sr;
    wa_noise_init(&e->noise, 0xC0FFEEu);

    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
    {
        w9_bt_t *b = &e->bt[i];
        memset(b, 0, sizeof(*b));
        b->drive = 2.0f;  b->level = 0.8f;  b->click_tone = 3000.0f;
        switch(i)
        {
        case 0: /* Bass Drum */
            /* From BD 909 Clean Long A and the mods doc: settles at ~50 Hz,
             * starts 1.9x sharp — the STOCK sweep amount, fitted against the
             * recording's own pitch track (the smeared 35 ms analysis window
             * under-reads the true start; 1.9/34 ms reproduces the measured
             * 66/55/50/50 Hz curve within 2 Hz). P.Depth 0 disables the sweep
             * entirely, at which point Tune audibly does nothing, exactly
             * like the hardware — the pitch envelope decays in ~34 ms
             * and the amplitude holds ~40 ms then falls with a ~120 ms time
             * constant (this ramp reaches -100 dB over `decay`, so tau is
             * decay/11.5). Click on the clean recording is nearly absent
             * (HF/body 0.002), so Attack defaults low. Drive at its floor is
             * the stock diode pair, now applied unconditionally in the render;
             * sub/tube/drift were our inventions and are pinned off at
             * trigger — a 909 has none of them. */
            b->tune = 13.0f;  /* sweep tau ms: the C recording's position */
            b->sweep_depth = 0.0f;   /* P.Depth: mods disengaged = stock */
            b->pitch_mod   = 1.0f;   /* Pitch: unity = stock */
            b->decay = 1380.0f; b->amp_hold = 40.0f;
            b->attack = 0.10f; b->click_tone = 2500.0f;
            b->drive = 0.2f;  b->level = 1.0f;
            break;
        /* Snare fitted to real 909 recordings (SD Clean E 01/03/10 — the same
         * drum at three Snappy settings): shells at 205 and 325 Hz (ratio
         * 1.585, NOT an octave), the second at ~28% of the first, shell t60
         * about 0.19 s, pitch glide only ~2.5% and fast, and the NOISE rings
         * longer than the shells (t60 0.34 s with Snappy up) — the shells are
         * the front of the sound, the noise is the tail. */
        case 1: /* Snare Drum: two tuned shells + snappy noise */
            b->tune = 205.0f; b->tune2 = 325.0f; b->osc2_mix = 0.40f;
            b->sweep_depth = 1.045f; b->sweep_time = 30.0f;
            b->decay = 320.0f; b->attack = 0.15f;
            b->snappy = 0.5f; b->noise_decay = 1200.0f; b->noise_hp = 1000.0f;
            b->drive = 1.8f;  b->level = 0.62f;   /* accented peak 0.85, not 0.95 */
            break;
        /* Tom tuning, glide and decay are fitted to measurements of a real
         * 909 (Tom Lo/Mid/Hi Clean): f0 68.5 / 102 / 131.5 Hz, pitch gliding
         * down ~10% over 200 ms (71 -> 64.5 on the low tom), t60 about
         * 1.0 / 0.64 / 0.66 s. The old 2.2x sweep was a synth swoop the
         * hardware does not make. */
        case 2: /* Low Tom */
            b->tune = 68.0f;
            b->sweep_depth = 1.12f; b->sweep_time = 200.0f;
            b->decay = 1700.0f; b->attack = 0.18f;
            b->level = 0.55f;  /* toms rang at 3/4 of the kick's RMS: overpowering */
            break;
        case 3: /* Med Tom */
            b->tune = 102.0f;
            b->sweep_depth = 1.12f; b->sweep_time = 180.0f;
            b->decay = 1050.0f; b->attack = 0.18f;
            b->level = 0.55f;  /* toms rang at 3/4 of the kick's RMS: overpowering */
            break;
        case 4: /* Hi Tom */
            b->tune = 132.0f;
            b->sweep_depth = 1.12f; b->sweep_time = 160.0f;
            b->decay = 1100.0f; b->attack = 0.18f;
            b->level = 0.55f;  /* toms rang at 3/4 of the kick's RMS: overpowering */
            break;
        default: break;
        }
        w9_bt_init(b, _sr);
    }
    for(int i=0; i<3; ++i) w9_tom909_init(&e->tom909[i], _sr);

    /* ---- Rim shot and hand clap (circuit models) ---- */
    {
        w9_rim909_t *r9 = &e->rim909;
        memset(r9, 0, sizeof(*r9));
        /* Measured on a real 909 rim: the two resonances are 210 and 480 Hz —
         * a ratio of 2.3, not the near-decade this had. The 1350 Hz "tick" was
         * invented; the real edge lives at 480 with its own harmonics above.
         * Second resonance sits at about half the first (56 vs 29 at 2 ms),
         * and the whole thing is down 30 dB by 30 ms — the shortest voice in
         * the machine. */
        r9->tune = 210.0f; r9->tune2 = 480.0f; r9->res = 10.0f;
        /* decay is the envelope's time to -100 dB, so -30 dB (where the real
         * rim lands at 30 ms) arrives at roughly an eighth of it. Drive down
         * from 2.4: the shaper was distorting the pair hard enough to invert
         * their balance — 480 Hz came out 2.5x the 210 where the hardware has
         * it at half. Distortion stays available on the knob. */
        r9->decay = 200.0f; r9->noise_mix = 0.35f;
        r9->drive = 1.4f;  r9->dist_type = 0.0f; r9->level = 1.1f;
        w9_rim909_init(r9, _sr);

        w9_clap909_t *c9 = &e->clap909;
        memset(c9, 0, sizeof(*c9));
        /* Fitted to Clap 909 Clean: bandpass centred ~950 Hz (Q ~2, burst
         * spectrum 925 with content to 3 k), echoes 12 ms apart, each dying in
         * a few ms; tail from the main hit at ~0.65 of it, single-exponential
         * tau ~30 ms matching the measured 0.62/0.43/0.23/0.10 ride-down. */
        c9->tune = 950.0f;  c9->res = 2.0f;
        c9->spread = 12.0f; c9->burst_decay = 50.0f;
        c9->tail_decay = 345.0f; c9->tail_level = 0.58f;
        c9->drive = 1.6f;  c9->dist_type = 0.0f; c9->level = 1.3f;
        w9_clap909_init(c9, _sr);
    }

    /* ---- Samplers ---- */
    /* Balanced against the measured kit: at the old defaults the closed hat
     * peaked 16 dB under the kick and the ride 17 — inaudible next to the
     * (then overpowered) toms. ohh/rc/cr/chh. */
    static const float vols[W9_NUM_SAMPLERS]   = { 0.85f, 0.45f, 0.5f, 0.95f };
    static const float sdec[W9_NUM_SAMPLERS]  = { 450.0f, 1800.0f, 1800.0f, 110.0f };
    /* The closed hat plays the open hat's buffer — same cymbals, shorter gate. */
    static const char *files[W9_NUM_SAMPLERS]  = { "hh.wav", "ride.wav", "crash.wav", NULL };
    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
    {
        w9_sampler_t *s = &e->sampler[i];
        s->decay = 450.0f;      /* open hat; ride/crash overridden below */
        s->decay_closed = 110.0f;
        s->volume = vols[i];
        s->decay  = sdec[i];
        s->drive  = 0.2f;      /* min = transparent; turn up to distort */
        s->dist_type = 0.0f;
        s->pitch  = 1.0f;
        s->rate_ratio = 1.0f; /* Orbit: preserve upstream unless adapter supplies source rate. */
        wa_param_init(&s->out, 0.0f);
        s->pos = -1.0;   /* idle */

        if(!files[i])
        {
            /* Shares the open hat's data: not loaded again, and not freed
             * through this index either (see w9_engine_free). */
            s->buffer = e->sample_data[W9_SAMP_OHH];
            s->length = e->sample_len[W9_SAMP_OHH];
        }
        else if(_module_dir)
        {
            char path[1024];
            snprintf(path, sizeof(path), "%s/samples/%s", _module_dir, files[i]);
            e->sample_data[i] = load_wav_mono(path, &e->sample_len[i]);
            s->buffer = e->sample_data[i];
            s->length = e->sample_len[i];
        }
    }

    /* ---- Send FX ---- */
    e->verb.decay = 0.62f; e->verb.tone = 0.45f;
    e->verb.hpf_hz = 150.0f; e->verb.level = 0.8f;
    wa_biquad_set(&e->verb.hp, WA_HIGHPASS, e->verb.hpf_hz, 0.7071f, 0.0f, _sr);
    wa_biquad_reset(&e->verb.hp);
    e->dly.divi = 7.0f;            /* dotted eighth */
    e->dly.bpm  = 120.0f;
    e->dly.fdbk = 0.35f; e->dly.tone = 0.4f;
    e->dly.hpf_hz = 150.0f; e->dly.level = 0.8f;
    w9_dly_retime(&e->dly, _sr);
    e->dly.dcur = e->dly.time_ms * 0.001f * _sr;
    wa_biquad_set(&e->dly.hp, WA_HIGHPASS, e->dly.hpf_hz, 0.7071f, 0.0f, _sr);
    wa_biquad_reset(&e->dly.hp);
    /* sends stay 0 from the memset: the kit is bit-identical until used */

    /* ---- Master ---- */
    e->master.comp         = 0.0f;
    e->master.comp_env_db  = 0.0f;
    e->master.comp_det     = 0.0f;
    e->master.drive        = 2.0f;   /* amount when a mode is chosen */
    e->master.dist_mode    = 0.0f;   /* Off by default */
    /* 0.35, not 0.5. With accent (x2) a single voice at its default Level was
     * already sitting at 0 dBFS, so the top half of every Level pot did
     * nothing but clip harder — the panel felt broken because it was. This
     * leaves room for the pot to travel and for eleven voices to sum. */
    e->master.volume       = 0.35f;  /* globalParams.volume  */
    e->master.accent       = 2.0f;   /* full-velocity gain, as it always was */
    e->master.vel_depth    = 1.0f;   /* velocity live by default */

    /* Derive initial pot positions from the defaults above. */
    w9_engine_seed_pots(e);
}

void w9_engine_free(w9_engine_t *e)
{
    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
    {
        free(e->sample_data[i]);        /* NULL for the closed hat: borrowed */
        e->sample_data[i] = NULL;
        e->sampler[i].buffer = NULL;
    }
}

/* ===================================================================== */
/* Triggering                                                             */
/* ===================================================================== */

void w9_engine_trigger(w9_engine_t *e, const w9_trigger_t _which, const int _velocity)
{

    /* How loud this hit is, from its velocity.
     *
     * Accent is the top: a full-velocity hit reaches it whatever Velocity is
     * set to, and softer hits come down from there. Velocity is how far down
     * — 0 means every hit plays at Accent, which is what this kit sounded
     * like before velocity existed at all.
     *
     * Three earlier cuts got this wrong, each worth not repeating:
     *   1. The 909's accent SWITCH, reproduced literally. From a sequencer
     *      that is a 6 dB cliff between velocity 99 and 100 with a flat shelf
     *      either side, and velocity looks broken.
     *   2. One line, but pivoting mid-range, so turning Velocity UP made hard
     *      hits ~6 dB louder — the knob moved the kit's loudness, not just its
     *      dynamics.
     *   3. Anchored at 1.0 with Accent deleted. That looks tidy and quietly
     *      drops the whole kit 6 dB, because 1.0 is the UNACCENTED level and
     *      a sequencer pattern had always been playing at the accented one.
     * Anchoring at Accent keeps the reference level exactly where it has
     * always been, and the knob only ever carves downwards from it. */
    const int   vi = _velocity < 0 ? 0 : (_velocity > 127 ? 127 : _velocity);
    const float vgain = e->master.accent
                      * (1.0f - e->master.vel_depth
                                * (1.0f - (float)vi * (1.0f/127.0f)));
    const float sr = e->sample_rate;

    switch(_which)
    {
    case W9_BD: case W9_SD: case W9_LT: case W9_MT: case W9_HT:
        /* Everything on the snare that is not a panel pot is fixed circuitry:
         * the shell interval (one pitch CV, D-network ratio 1.585), the pitch
         * pulse (ENV1 — "the same for all SD notes"), the shell decay
         * (C70/C79 discharge) and the noise bandpass. Pinned here, at the
         * moment they matter, so no stale saved state can detune the pair,
         * re-arm the old 1.6x pitch swoop, or shorten the shells — the blob
         * still restores those fields, and this overrules it. */
        /* Same for the kick: everything not on its panel is fixed circuitry.
         * The sub layer, tube stage and drift were our additions — no 909 has
         * them — and the diode pair provides the tone now. */
        if(_which == W9_BD)
        {
            w9_bt_t *bd = &e->bt[W9_BD];
            bd->tune2 = 0.0f; bd->osc2_mix = 0.0f;
            bd->click_tone = 2500.0f;
            bd->amp_hold = 40.0f;
            /* Stock kick, no mods. Tune = the sweep knob, exactly as the
             * four lettered recordings show (base fixed at 49 Hz on all of
             * them); the pot value is the sweep's time constant in ms. */
            bd->bd_sweep = 1;
        }
        /* Clap: the real 909 gives it ONE pot (Level). Tune and Tail stay
         * because Gus likes them; the burst geometry is the circuit's and is
         * pinned — echo spacing, echo decay, tail share and the filter's Q. */
        if(_which == W9_HC)
        {
            w9_clap909_t *c9 = &e->clap909;
            c9->spread = 12.0f; c9->burst_decay = 50.0f;
            c9->tail_level = 0.58f;
            if(c9->res != 2.0f) { c9->res = 2.0f; w9_clap909_retune(c9); }
        }
        if(_which == W9_SD)
        {
            w9_bt_t *sd = &e->bt[W9_SD];
            sd->tune2       = sd->tune * 1.585f;
            sd->osc2_mix    = 0.40f;   /* ~28% of shell 1 at the output */
            sd->sweep_depth = 1.045f;  /* 206->198 Hz measured, not 8% */
            sd->sweep_time  = 30.0f;
            /* Shell ring time, measured directly off both references: tau
             * 28 ms on Gus's 909 and 30 ms on the TR-909 plugin capture. 420
             * gave 36 ms — the shells hung around too long. Note this is ONE
             * envelope for both shells on purpose: the schematic gives them
             * separate caps (C70 4.7n, C79 10n) and separate VCAs, but the
             * instrument decays them together (tau ratio 0.99 and 0.95 in the
             * two recordings), so the discharge resistors must compensate.
             * Modelling the parts list here would contradict the drum. */
            sd->decay       = 340.0f;
            sd->attack      = 0.0f;    /* the 909 SD has no click path */
            if(sd->noise_hp < 990.0f || sd->noise_hp > 1010.0f)
                w9_engine_set_raw(e, "sd_c_noise_hp", 1000.0f);
        }
        /* The toms are their own circuit: three oscillators with separate
         * envelopes plus a noise attack (9w9_tom909.h). They read the same
         * panel values as the two-oscillator voice, so nothing else changes. */
        if(_which >= W9_LT && _which <= W9_HT)
            w9_tom909_trigger(&e->tom909[_which - W9_LT], &e->bt[_which], vgain);
        else
            w9_bt_trigger(&e->bt[_which], vgain);
        break;

    case W9_RS: {
        {
            /* Fixed circuitry, not pots: the upper resonance is the same
             * network as the lower (measured 480/210 = 2.286, so it tracks
             * Tune), both ring at Q ~10, and the trigger pulse carries a set
             * amount of noise. Pinned here so no saved state can drift them. */
            w9_rim909_t *r9 = &e->rim909;
            const float t2 = r9->tune * 2.286f, q = 10.0f;
            if(r9->tune2 != t2 || r9->res != q)
            { r9->tune2 = t2; r9->res = q; w9_rim909_retune(r9); }
            r9->noise_mix = 0.35f;   /* inside the 2-sample pulse only */
            r9->decay     = 200.0f;  /* -30 dB at ~30 ms, as measured */
            w9_rim909_trigger(r9, vgain);
            break;
        }
    }

    case W9_HC: {
        w9_clap909_trigger(&e->clap909, vgain);
        break;
    }

    case W9_OHH: case W9_CHH: case W9_RC: case W9_CR: {
        const int idx = _which == W9_OHH ? W9_SAMP_OHH
                      : _which == W9_CHH ? W9_SAMP_CHH
                      : _which == W9_RC  ? W9_SAMP_RC : W9_SAMP_CR;
        w9_sampler_t *s = &e->sampler[idx];

        /* One pair of cymbals: closing the pedal stops the open hat ringing,
         * and opening it cuts the closed sound short. Anything else lets two
         * hats sound at once, which the hardware cannot do. */
        if(idx == W9_SAMP_OHH || idx == W9_SAMP_CHH)
        {
            w9_sampler_t *other = &e->sampler[idx == W9_SAMP_OHH
                                                ? W9_SAMP_CHH : W9_SAMP_OHH];
            /* A short fade, not a hard stop: cutting a ringing buffer mid-cycle
             * is a click. 3 ms is under a hat's own attack. */
            wa_exp_ramp(&other->out, 0.00001f, ms_to_samples(3.0f, sr));
            other->mute_countdown = ms_to_samples(6.0f, sr);
        }

        wa_set_value(&s->out, s->volume * vgain);
        wa_exp_ramp(&s->out, 0.00001f, ms_to_samples(s->decay, sr));
        s->pos = 0.0;                          /* restart the buffer */
        s->mute_countdown = ms_to_samples(s->decay, sr);
        break;
    }

    default: break;
    }
}

/* ===================================================================== */
/* Per-voice rendering                                                    */
/* ===================================================================== */

static float render_sampler(w9_sampler_t *s)
{
    if(s->mute_countdown <= 0.0 || s->pos < 0.0 || !s->buffer) return 0.0f;
    s->mute_countdown -= 1.0;

    float v = 0.0f;
    if(s->pos < (double)s->length)
    {
        /* linear interpolation, matching playbackRate resampling */
        const uint32_t i = (uint32_t)s->pos;
        const float fr = (float)(s->pos - (double)i);
        const float a = s->buffer[i];
        const float b = (i + 1 < s->length) ? s->buffer[i + 1] : 0.0f;
        v = a + (b - a) * fr;
        s->pos += (double)(s->pitch > 0.0f ? s->pitch : 1.0f) * s->rate_ratio;
    }
    v *= wa_param_tick(&s->out);
    if(s->drive > 0.25f)                     /* below this the shape is inaudible */
        v = w9_shape_st(v * (1.0f + s->drive * 0.5f), s->drive, s->dist_type, s->crush_st);
    return v;
}


static const int W9_VERB_CL[4] = { 1116, 1188, 1277, 1356 };
static const int W9_VERB_AL[2] = { 556, 441 };

static float w9_verb_tick(w9_verb_t *r, const float _in)
{
    const float x = wa_biquad_tick(&r->hp, _in);
    /* Tone: bright opens the loop's damping, dark closes it. */
    const float damp = 0.75f - r->tone * 0.55f;
    const float fb   = r->decay;
    float acc = 0.0f;
    for(int i = 0; i < 4; ++i)
    {
        float *b = r->comb[i];
        const int n = W9_VERB_CL[i];
        const float y = b[r->cpos[i]];
        acc += y;
        r->cdmp[i] = y + (r->cdmp[i] - y) * damp;
        float st = x + r->cdmp[i] * fb;
        /* The early-rack character: the loop runs at 12 bits. Truncate
         * toward zero — floor injects -0.5 LSB of DC per pass, and a DC-fed
         * comb loop settles into a -70 dB hum that never stops. */
        st = truncf(st * 2048.0f) * (1.0f / 2048.0f);
        b[r->cpos[i]] = st;
        if(++r->cpos[i] >= n) r->cpos[i] = 0;
    }
    float y = acc * 0.25f;
    for(int j = 0; j < 2; ++j)
    {
        float *b = r->apb[j];
        const int n = W9_VERB_AL[j];
        const float bo = b[r->apos[j]];
        b[r->apos[j]] = y + bo * 0.5f;
        y = bo - y * 0.5f;
        if(++r->apos[j] >= n) r->apos[j] = 0;
    }
    return y * r->level;
}

/* Note divisions, shortest to longest, in beats. Matches the dly_time enum:
 * 1/32 1/16T 1/16 1/8T 1/16. 1/8 1/4T 1/8. 1/4 1/2T 1/4. 1/2 1/2. */
static const float w9_dly_beats[W9_DLY_DIVS] = {
    0.125f, 1.0f/6.0f, 0.25f, 1.0f/3.0f, 0.375f, 0.5f, 2.0f/3.0f,
    0.75f, 1.0f, 4.0f/3.0f, 1.5f, 2.0f, 3.0f
};

static void w9_dly_retime(w9_dly_t *d, float _sr)
{
    int i = (int)(d->divi + 0.5f);
    if(i < 0) i = 0;
    if(i >= W9_DLY_DIVS) i = W9_DLY_DIVS - 1;
    const float bpm = d->bpm > 20.0f ? d->bpm : 120.0f;
    float ms = w9_dly_beats[i] * 60000.0f / bpm;
    /* The line is 2 s; longer divisions at slow tempos clamp to it. */
    const float max_ms = (float)(W9_DLY_MAX - 256) / _sr * 1000.0f;
    if(ms > max_ms) ms = max_ms;
    d->time_ms = ms;
}

static float w9_dly_tick(w9_dly_t *d, const float _in, const float _sr)
{
    const float x = wa_biquad_tick(&d->hp, _in);
    const float target = d->time_ms * 0.001f * _sr;
    /* Slewed, so turning Time warps the echo like the old units instead of
     * clicking. ~30 ms to settle. */
    d->dcur += (target - d->dcur) * 0.0008f;
    float rp = (float)d->w - d->dcur;
    while(rp < 0.0f) rp += (float)W9_DLY_MAX;
    int i0 = (int)rp;
    const float fr = rp - (float)i0;
    /* float spacing at 36000 is 1/256: a read a hair under the wrap point
     * rounds UP to exactly W9_DLY_MAX and indexes past the buffer. */
    if(i0 >= W9_DLY_MAX) i0 -= W9_DLY_MAX;
    const int   i1 = i0 + 1 >= W9_DLY_MAX ? 0 : i0 + 1;
    const float y  = d->buf[i0] + (d->buf[i1] - d->buf[i0]) * fr;

    /* Feedback through a one-pole: every repeat gets darker, like tape and
     * the early digitals both did. 12-bit write for the grain. */
    const float tc = 0.06f + d->tone * 0.6f;
    d->lp += (y * d->fdbk - d->lp) * tc;
    if(fabsf(d->lp) < 1e-20f) d->lp = 0.0f;   /* no denormal tail */
    float st = x + d->lp;
    st = truncf(st * 2048.0f) * (1.0f / 2048.0f);   /* toward zero: no DC */
    d->buf[d->w] = st;
    if(++d->w >= W9_DLY_MAX) d->w = 0;
    return y * d->level;
}

/* One-knob glue. The knob lowers the threshold (-8 to -26 dB), raises the
 * ratio (2:1 to 5:1) and applies makeup scaled to the deeper threshold, so
 * loudness stays in the same ballpark while the kit pulls together. Detector
 * is smoothed — 8 ms attack so transients punch through, 150 ms release, 6 dB
 * soft knee. The old inherited compressor's sins are not repeated: no zero-
 * attack waveshaping, and amount zero means the stage is not in the path. */
static float master_glue(w9_master_t *m, const float _in, const float _sr)
{
    const float a = m->comp;
    /* Gentler curve than the first cut: -26 dB threshold at 5:1 measured
     * 12 dB of reduction on the kit and the 6.5 dB makeup then amplified the
     * transients the 8 ms attack let through — peak UP 50%%, RMS halved.
     * Glue, not a pump: shallower threshold, 4:1 tops, 3 ms attack. */
    /* Anchored to the bus's actual level: the sum runs about +6 dBFS peak
     * BEFORE master volume scales it down, so dBFS-style thresholds sit tens
     * of dB under the signal and slam it (first cut measured 17 dB of
     * standing reduction). 4-6a puts max reduction near 6 dB at full knob. */
    /* The measured bus: a single voice peaks ~+6 dB (pre-volume), a four-
     * voice accented stack reaches +15 with its body riding +5..+12. The
     * knob walks the threshold down through that range: at low amounts only
     * stacked hits get caught; at full, single hits breathe too. */
    /* Second calibration: the first only caught four-voice accented stacks,
     * which no real pattern is — on the device it audibly did nothing. Now
     * the threshold ends 6 dB under a single accented voice's peak, so at
     * full knob every hit is worked. */
    const float thr = 10.0f - 18.0f * a;
    const float ratio = 2.0f + 3.0f * a;
    const float knee = 6.0f;

    /* Level-follow BEFORE the dB math. Detecting on the instantaneous sample
     * made the stage track the waveform of a 50 Hz kick — its period is 20 ms,
     * far longer than the attack — locking the gain to the cycle's peaks and
     * crushing everything between them (RMS fell to a third while peaks
     * passed). The follower rides the envelope instead. */
    const float mag = fabsf(_in);
    const float drel = expf(-1.0f / (0.035f * _sr));
    m->comp_det = mag > m->comp_det ? mag : m->comp_det * drel;
    const float in_db = m->comp_det > 1e-9f ? 20.0f * log10f(m->comp_det) : -120.0f;
    const float over = in_db - thr;
    float gr = 0.0f;
    if(over >= knee * 0.5f)
        gr = -(over) * (1.0f - 1.0f / ratio);
    else if(over > -knee * 0.5f)
    {
        const float t = over + knee * 0.5f;
        gr = -(t * t) / (2.0f * knee) * (1.0f - 1.0f / ratio);
    }

    const float atk = expf(-1.0f / (0.003f * _sr));
    const float rel = expf(-1.0f / (0.120f * _sr));
    const float coef = gr < m->comp_env_db ? atk : rel;
    m->comp_env_db = gr + (m->comp_env_db - gr) * coef;

    /* AutoGain: fitted so the pattern's loudness stays flat across the whole
     * knob. Linear makeup left +1 dB at noon and -1.8 at full (the reduction
     * grows faster than linearly as the threshold walks down); 2.2a + 6.6a^2
     * is the measured loss inverted. */
    const float makeup = a * 2.2f + a * a * 6.6f;
    return _in * powf(10.0f, (m->comp_env_db + makeup) / 20.0f);
}

void w9_engine_render(w9_engine_t *e, float *out, const int frames)
{
    for(int n=0; n<frames; ++n)
    {
        const float noise = wa_noise_tick(&e->noise);

        float mix = 0.0f, rbus = 0.0f, dbus = 0.0f;
        for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
        {
            float v;
            if(i >= W9_LT && i <= W9_HT)
                v = w9_tom909_render(&e->tom909[i - W9_LT], &e->bt[i], noise);
            else
                v = w9_bt_render(&e->bt[i], noise);
            mix += v;
            rbus += v * e->send_rev[i];
            dbus += v * e->send_dly[i];
        }
        {
            const float v = w9_rim909_render(&e->rim909, noise);
            mix += v; rbus += v * e->send_rev[W9_RS]; dbus += v * e->send_dly[W9_RS];
        }
        {
            const float v = w9_clap909_render(&e->clap909, noise);
            mix += v; rbus += v * e->send_rev[W9_HC]; dbus += v * e->send_dly[W9_HC];
        }
        {
            static const int strig[W9_NUM_SAMPLERS] = { W9_OHH, W9_RC, W9_CR, W9_CHH };
            for(int i=0; i<W9_NUM_SAMPLERS; ++i)
            {
                const float v = render_sampler(&e->sampler[i]);
                mix += v;
                rbus += v * e->send_rev[strig[i]];
                dbus += v * e->send_dly[strig[i]];
            }
        }

        /* FX returns join the bus before the master stages, so master
         * distortion and the Comp work on the wet signal too. */
        mix += w9_verb_tick(&e->verb, rbus);
        mix += w9_dly_tick(&e->dly, dbus, e->sample_rate);

        if(e->master.dist_mode >= 0.5f)
            mix = w9_shape_st(mix * e->master.drive, e->master.drive,
                             e->master.dist_mode - 1.0f, e->master.crush_st) * 0.7f;
        if(e->master.comp > 0.001f)
            mix = master_glue(&e->master, mix, e->sample_rate);
        out[n] = mix * e->master.volume;
    }
}

/* ===================================================================== */
/* Parameters                                                             */
/* ===================================================================== */

typedef struct { const char *name; size_t off; } field_t;

/* Circuit-voice fields, shared by set_param/get_param and state save/load. */
typedef struct { const char *n; size_t off; } bt_field_t;
#define BTF(n) { #n, offsetof(w9_bt_t, n) }
static const bt_field_t g_bt_fields[] = {
    BTF(tune), BTF(sweep_depth), BTF(sweep_time), BTF(decay), BTF(attack),
    BTF(click_tone), BTF(drive), BTF(level), BTF(dist_type),
    BTF(tune2), BTF(osc2_mix), BTF(snappy), BTF(noise_decay), BTF(noise_hp),
    BTF(pitch_mod),
};
#define W9_BT_FIELD_COUNT (sizeof(g_bt_fields)/sizeof(g_bt_fields[0]))

/* Everything else that must round-trip in a saved state. */
static const char *const g_other_state_keys[] = {
    "rs_decay", "rs_volume", "rs_saturation",
    "hc_decay", "hc_spread", "hc_volume", "hc_tone_decay",
    "ohh_decay", "ohh_volume", "ohh_pitch",
    "chh_decay", "chh_volume", "chh_pitch", "chh_drive",
    "rc_decay", "rc_volume", "rc_pitch",
    "cr_decay", "cr_volume", "cr_pitch",
    "volume", "accent", "vel_depth", "master_dist", "master_comp", "dly_time",
    "rs_dist_type", "hc_dist_type", "ohh_dist_type", "chh_dist_type",
    "rc_dist_type", "cr_dist_type",
};
#define W9_OTHER_KEY_COUNT (sizeof(g_other_state_keys)/sizeof(g_other_state_keys[0]))

/* Per-voice FX sends. The kick has none, deliberately. */
static const struct { const char *k; unsigned char voice, dly; } g_send_keys[] = {
    {"sd_c_rev",1,0},{"sd_c_dly",1,1},
    {"lt_c_rev",2,0},{"lt_c_dly",2,1},
    {"mt_c_rev",3,0},{"mt_c_dly",3,1},
    {"ht_c_rev",4,0},{"ht_c_dly",4,1},
    {"rs_rev",5,0},{"rs_dly",5,1},
    {"hc_rev",6,0},{"hc_dly",6,1},
    {"ohh_rev",7,0},{"ohh_dly",7,1},
    {"chh_rev",8,0},{"chh_dly",8,1},
    {"rc_rev",9,0},{"rc_dly",9,1},
    {"cr_rev",10,0},{"cr_dly",10,1},
};
#define W9_SEND_KEYS ((int)(sizeof(g_send_keys)/sizeof(g_send_keys[0])))

int w9_engine_set_raw(w9_engine_t *e, const char *key, const float value)
{
    for(int i=0; i<W9_SEND_KEYS; ++i)
        if(!strcmp(key, g_send_keys[i].k))
        {
            float *a = g_send_keys[i].dly ? e->send_dly : e->send_rev;
            a[g_send_keys[i].voice] = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
            return 1;
        }
    if(!strcmp(key, "rev_decay")) { e->verb.decay = value; return 1; }
    if(!strcmp(key, "rev_tone"))  { e->verb.tone = value; return 1; }
    if(!strcmp(key, "rev_level")) { e->verb.level = value; return 1; }
    if(!strcmp(key, "rev_hpf"))
    {
        e->verb.hpf_hz = value;
        wa_biquad_set(&e->verb.hp, WA_HIGHPASS, value, 0.7071f, 0.0f, e->sample_rate);
        return 1;
    }
    if(!strcmp(key, "dly_time"))
    {
        e->dly.divi = value < 0.0f ? 0.0f
                    : (value > W9_DLY_DIVS - 1 ? W9_DLY_DIVS - 1 : value);
        w9_dly_retime(&e->dly, e->sample_rate);
        return 1;
    }
    if(!strcmp(key, "dly_bpm"))
    {
        e->dly.bpm = value;
        w9_dly_retime(&e->dly, e->sample_rate);
        return 1;
    }
    if(!strcmp(key, "dly_fdbk"))  { e->dly.fdbk = value; return 1; }
    if(!strcmp(key, "dly_tone"))  { e->dly.tone = value; return 1; }
    if(!strcmp(key, "dly_level")) { e->dly.level = value; return 1; }
    if(!strcmp(key, "dly_hpf"))
    {
        e->dly.hpf_hz = value;
        wa_biquad_set(&e->dly.hp, WA_HIGHPASS, value, 0.7071f, 0.0f, e->sample_rate);
        return 1;
    }

    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
        {
            char pre[16];
            snprintf(pre, sizeof(pre), "%s_c_", w9_trigger_names[i]);
            const size_t plen = strlen(pre);
            if(strncmp(key, pre, plen)) continue;
            for(size_t k=0; k<W9_BT_FIELD_COUNT; ++k)
            {
                if(strcmp(key + plen, g_bt_fields[k].n)) continue;
                *(float*)((char*)&e->bt[i] + g_bt_fields[k].off) = value;
                wa_biquad_set(&e->bt[i].click_lp, WA_LOWPASS,
                              e->bt[i].click_tone > 0.0f ? e->bt[i].click_tone : 3000.0f,
                              0.7071f, 0.0f, e->sample_rate);
                wa_biquad_set(&e->bt[i].noise_hpf, WA_HIGHPASS,
                              e->bt[i].noise_hp > 0.0f ? e->bt[i].noise_hp : 800.0f,
                              0.7071f, 0.0f, e->sample_rate);
                return 1;
            }
        }
    {
        w9_rim909_t *r9 = &e->rim909; w9_clap909_t *c9 = &e->clap909;
        if(!strcmp(key,"rs_decay"))     { r9->decay = value; return 1; }
        if(!strcmp(key,"rs_volume"))    { r9->level = value; return 1; }
        if(!strcmp(key,"rs_saturation")){ r9->drive = value; return 1; }
        if(!strcmp(key,"rs_tune"))      { r9->tune = value;  w9_rim909_retune(r9); return 1; }
        if(!strcmp(key,"rs_tune2"))     { r9->tune2 = value; w9_rim909_retune(r9); return 1; }
        if(!strcmp(key,"rs_res"))       { r9->res = value;   w9_rim909_retune(r9); return 1; }
        if(!strcmp(key,"rs_noise"))     { r9->noise_mix = value; return 1; }
        if(!strcmp(key,"hc_decay"))     { c9->tail_decay = value; return 1; }
        if(!strcmp(key,"hc_spread"))    { c9->spread = value; return 1; }
        if(!strcmp(key,"hc_volume"))    { c9->level = value; return 1; }
        if(!strcmp(key,"hc_tone_decay")){ c9->burst_decay = value; return 1; }
        if(!strcmp(key,"hc_tune"))      { c9->tune = value; w9_clap909_retune(c9); return 1; }
        if(!strcmp(key,"hc_tail"))      { c9->tail_level = value; return 1; }
        if(!strcmp(key,"hc_drive"))     { c9->drive = value; return 1; }
    }

    /* Patches saved before the closed hat became its own voice carry its decay
     * as ohh_decay_closed. Keep accepting it. */
    if(!strcmp(key, "ohh_decay_closed")) { e->sampler[W9_SAMP_CHH].decay = value; return 1; }

    static const char *sn[W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s_decay", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].decay = value; return 1; }
        snprintf(buf, sizeof(buf), "%s_volume", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].volume = value; return 1; }
        snprintf(buf, sizeof(buf), "%s_pitch", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].pitch = value; return 1; }
        snprintf(buf, sizeof(buf), "%s_decay_closed", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].decay_closed = value; return 1; }
        snprintf(buf, sizeof(buf), "%s_drive", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].drive = value; return 1; }
        snprintf(buf, sizeof(buf), "%s_dist_type", sn[i]);
        if(!strcmp(key, buf)) { e->sampler[i].dist_type = value < 0 ? 0 : (value > 6 ? 6 : value); return 1; }
    }

    if(!strcmp(key, "rs_dist_type")) { e->rim909.dist_type = value < 0 ? 0 : (value > 6 ? 6 : value); return 1; }
    if(!strcmp(key, "hc_dist_type")) { e->clap909.dist_type = value < 0 ? 0 : (value > 6 ? 6 : value); return 1; }

    if(!strcmp(key, "master_drive")) { e->master.drive = value; return 1; }
    if(!strcmp(key, "master_dist"))  { e->master.dist_mode = value < 0 ? 0 : (value > 7 ? 7 : value); return 1; }
    if(!strcmp(key, "master_comp")) { e->master.comp = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); return 1; }
    if(!strcmp(key, "volume")) { e->master.volume = value; return 1; }
    if(!strcmp(key, "accent")) { e->master.accent = value; return 1; }
    if(!strcmp(key, "vel_depth"))
    {
        e->master.vel_depth = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
        return 1;
    }
    return 0;
}

int w9_engine_get_raw(const w9_engine_t *e, const char *key, float *out)
{
    for(int i=0; i<W9_SEND_KEYS; ++i)
        if(!strcmp(key, g_send_keys[i].k))
        {
            const float *a = g_send_keys[i].dly ? e->send_dly : e->send_rev;
            *out = a[g_send_keys[i].voice];
            return 1;
        }
    if(!strcmp(key, "rev_decay")) { *out = e->verb.decay; return 1; }
    if(!strcmp(key, "rev_tone"))  { *out = e->verb.tone; return 1; }
    if(!strcmp(key, "rev_level")) { *out = e->verb.level; return 1; }
    if(!strcmp(key, "rev_hpf"))   { *out = e->verb.hpf_hz; return 1; }
    if(!strcmp(key, "dly_time"))  { *out = e->dly.divi; return 1; }
    if(!strcmp(key, "dly_fdbk"))  { *out = e->dly.fdbk; return 1; }
    if(!strcmp(key, "dly_tone"))  { *out = e->dly.tone; return 1; }
    if(!strcmp(key, "dly_level")) { *out = e->dly.level; return 1; }
    if(!strcmp(key, "dly_hpf"))   { *out = e->dly.hpf_hz; return 1; }

    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
    {
        char pre[16];
        snprintf(pre, sizeof(pre), "%s_c_", w9_trigger_names[i]);
        const size_t plen = strlen(pre);
        if(strncmp(key, pre, plen)) continue;
        for(size_t k=0; k<W9_BT_FIELD_COUNT; ++k)
            if(!strcmp(key + plen, g_bt_fields[k].n))
            { *out = *(const float*)((const char*)&e->bt[i] + g_bt_fields[k].off); return 1; }
    }
    {
        if(!strcmp(key, "ohh_decay_closed")) { *out = e->sampler[W9_SAMP_CHH].decay; return 1; }
        static const char *sn[W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
        for(int i=0; i<W9_NUM_SAMPLERS; ++i)
        {
            char b[32];
            snprintf(b, sizeof(b), "%s_decay", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].decay; return 1; }
            snprintf(b, sizeof(b), "%s_volume", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].volume; return 1; }
            snprintf(b, sizeof(b), "%s_pitch", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].pitch; return 1; }
            snprintf(b, sizeof(b), "%s_decay_closed", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].decay_closed; return 1; }
            snprintf(b, sizeof(b), "%s_drive", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].drive; return 1; }
            snprintf(b, sizeof(b), "%s_dist_type", sn[i]);
            if(!strcmp(key, b)) { *out = e->sampler[i].dist_type; return 1; }
        }
    }
    if(!strcmp(key, "rs_dist_type")) { *out = e->rim909.dist_type; return 1; }
    if(!strcmp(key, "hc_dist_type")) { *out = e->clap909.dist_type; return 1; }
    {
        const w9_rim909_t *r9 = &e->rim909; const w9_clap909_t *c9 = &e->clap909;
        if(!strcmp(key,"rs_decay"))     { *out = r9->decay; return 1; }
        if(!strcmp(key,"rs_volume"))    { *out = r9->level; return 1; }
        if(!strcmp(key,"rs_saturation")){ *out = r9->drive; return 1; }
        if(!strcmp(key,"rs_tune"))      { *out = r9->tune;  return 1; }
        if(!strcmp(key,"rs_tune2"))     { *out = r9->tune2; return 1; }
        if(!strcmp(key,"rs_res"))       { *out = r9->res;   return 1; }
        if(!strcmp(key,"rs_noise"))     { *out = r9->noise_mix; return 1; }
        if(!strcmp(key,"hc_decay"))     { *out = c9->tail_decay; return 1; }
        if(!strcmp(key,"hc_spread"))    { *out = c9->spread; return 1; }
        if(!strcmp(key,"hc_volume"))    { *out = c9->level; return 1; }
        if(!strcmp(key,"hc_tone_decay")){ *out = c9->burst_decay; return 1; }
        if(!strcmp(key,"hc_tune"))      { *out = c9->tune; return 1; }
        if(!strcmp(key,"hc_tail"))      { *out = c9->tail_level; return 1; }
        if(!strcmp(key,"hc_drive"))     { *out = c9->drive; return 1; }
    }
    if(!strcmp(key, "master_drive")) { *out = e->master.drive; return 1; }
    if(!strcmp(key, "master_dist"))  { *out = e->master.dist_mode; return 1; }
    if(!strcmp(key, "master_comp")) { *out = e->master.comp; return 1; }
    if(!strcmp(key, "volume"))    { *out = e->master.volume; return 1; }
    if(!strcmp(key, "accent"))    { *out = e->master.accent; return 1; }
    if(!strcmp(key, "vel_depth")) { *out = e->master.vel_depth; return 1; }
    return 0;
}


/* Keys that are switches/enums, not pots: passed through unscaled. */
static int is_enum_key(const char *key)
{
    if(!strcmp(key, "master_dist")) return 1;
    if(!strcmp(key, "dly_time")) return 1;     /* note division picker */
    const size_t n = strlen(key);
    return n > 10 && !strcmp(key + n - 10, "_dist_type");
}

/* ===================================================================== */
/* State — self-contained, so slot autosave and User Presets round-trip   */
/* ===================================================================== */

int w9_engine_get_state(const w9_engine_t *e, char *buf, const int len)
{
    if(!buf || len <= 0) return -1;
    int n = 0;
    /* v3: the voice-rebuild cut. v2 blobs carry pot positions whose meanings
     * and ranges changed under them (tom decay ranges, snare Tune centre, the
     * old octave tune2, the 1.6x sweep, osc2_mix 0.7) — restoring one
     * resurrects the pre-rebuild sound no matter how the defaults are set,
     * which is exactly what kept happening. Old blobs are discarded whole. */
    n += snprintf(buf + n, (size_t)(len - n), W9_STATE_TAG);

    /* Pot positions: what the panel shows, and exactly what restores it. */
    for(int i=0; i<W9_POT_COUNT && n < len - 1; ++i)
        n += snprintf(buf + n, (size_t)(len - n), "%s=%d;",
                      g_w9_pots[i].key, (int)e->pots[i]);

    /* Switches keep their own values. */
    for(size_t k=0; k<W9_OTHER_KEY_COUNT && n < len - 1; ++k)
    {
        if(!is_enum_key(g_other_state_keys[k])) continue;
        float v = 0.0f;
        if(w9_engine_get_raw(e, g_other_state_keys[k], &v))
            n += snprintf(buf + n, (size_t)(len - n), "%s=%.0f;", g_other_state_keys[k], v);
    }
    for(int i=0; i<W9_NUM_INSTRUMENTS && n < len - 1; ++i)
        n += snprintf(buf + n, (size_t)(len - n), "%s_c_dist_type=%.0f;",
                      w9_trigger_names[i], e->bt[i].dist_type);

    return n < len ? n : len - 1;
}

/*
 * The current tag, or any v3 tag written before it: a short first segment
 * ending in "v3" and holding no '='. v3 is the pot-position format, so the
 * values that follow mean the same thing whichever name it was saved under.
 */
static int w9_state_tag_ok(const char *blob)
{
    if(strncmp(blob, W9_STATE_TAG, sizeof(W9_STATE_TAG) - 1) == 0) return 1;
    const char *semi = strchr(blob, ';');
    if(!semi) return 0;
    const size_t n = (size_t)(semi - blob);
    return n >= 3 && n <= 8 && memcmp(semi - 2, "v3", 2) == 0 &&
           memchr(blob, '=', n) == NULL;
}

int w9_engine_set_state(w9_engine_t *e, const char *blob)
{
    if(!blob) return 0;

    /*
     * v1 blobs stored RAW engineering values (volume=0.5, decay=190). Feeding
     * those to the pot-based setter reads 0.5 as pot 0 — master volume zero,
     * i.e. total silence. Anything that isn't v2 is discarded so the engine
     * keeps its (audible) defaults instead of being silently muted.
     */
    if(!w9_state_tag_ok(blob))
        return 0;      /* v1/v2/unknown: keep the (correct) defaults */

    const char *p = blob;
    while(*p)
    {
        const char *semi = strchr(p, ';');
        const size_t seglen = semi ? (size_t)(semi - p) : strlen(p);
        if(seglen > 0 && seglen < 96)
        {
            char seg[96];
            memcpy(seg, p, seglen);
            seg[seglen] = '\0';
            char *eq = strchr(seg, '=');
            if(eq)
            {
                *eq = '\0';
                w9_engine_set_param(e, seg, (float)atof(eq + 1));
            }
        }
        if(!semi) break;
        p = semi + 1;
    }
    return 1;
}


/* ===================================================================== */
/* Pot layer — the external 0..127 parameter surface                      */
/* ===================================================================== */

/* Renames the pot layer honours, so a patch saved under an old key lands on the
 * control that replaced it — pot position included, not just the engine field.
 * ohh_decay_closed became chh_decay when the closed hat got its own voice. */
static const char *pot_alias(const char *key)
{
    if(!strcmp(key, "ohh_decay_closed")) return "chh_decay";
    return key;
}

static int pot_index(const char *key)
{
    key = pot_alias(key);
    for(int i=0; i<W9_POT_COUNT; ++i)
        if(!strcmp(key, g_w9_pots[i].key)) return i;
    return -1;
}


void w9_engine_seed_pots(w9_engine_t *e)
{
    for(int i=0; i<W9_POT_COUNT; ++i)
    {
        float v = 0.0f;
        if(w9_engine_get_raw(e, g_w9_pots[i].key, &v))
            e->pots[i] = (unsigned char)w9_value_to_pot(&g_w9_pots[i], v);
    }
}

int w9_engine_set_param(w9_engine_t *e, const char *key, const float pot)
{
    if(is_enum_key(key))
        return w9_engine_set_raw(e, key, pot);

    const int idx = pot_index(key);
    if(idx < 0)
        return w9_engine_set_raw(e, key, pot);   /* unknown: treat as raw */

    int p = (int)(pot + 0.5f);
    if(p < 0)   p = 0;
    if(p > 127) p = 127;
    e->pots[idx] = (unsigned char)p;
    return w9_engine_set_raw(e, key, w9_pot_to_value(&g_w9_pots[idx], p));
}

int w9_engine_get_param(const w9_engine_t *e, const char *key, float *pot)
{
    if(is_enum_key(key))
        return w9_engine_get_raw(e, key, pot);

    const int idx = pot_index(key);
    if(idx < 0)
        return w9_engine_get_raw(e, key, pot);

    *pot = (float)e->pots[idx];
    return 1;
}

