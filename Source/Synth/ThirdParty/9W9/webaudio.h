/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
/*
 * webaudio.h — a handful of W3C Web Audio API nodes, in C.
 *
 * Each node implements the behaviour the W3C Web Audio spec defines
 * mathematically, so results are verifiable rather than tuned by ear.
 *
 * Spec references:
 *   BiquadFilterNode  https://www.w3.org/TR/webaudio/#filters-characteristics
 *   AudioParam ramps  https://www.w3.org/TR/webaudio/#dom-audioparam-*
 *   WaveShaperNode    https://www.w3.org/TR/webaudio/#WaveShaperNode
 *
 * GPL-3.0.
 */

#ifndef W9_WEBAUDIO_H
#define W9_WEBAUDIO_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Web Audio clamps exponential ramp endpoints away from zero; envelopes rely
 * on this by ramping to 0.00001 rather than 0. */
#define WA_MIN_EXP 1.0e-9f

/* ===================================================================== */
/* AudioParam — the automation timeline                                   */
/* ===================================================================== */
/*
 * The voices use four automation methods. Rather than model a full event list we
 * keep the single active segment, which is sufficient because every voice
 * re-arms its envelopes on trigger (the one exception, the clap's 5 scheduled
 * taps, is handled by the clap voice itself).
 */

typedef enum {
    WA_SEG_CONST = 0,
    WA_SEG_LINEAR,       /* linearRampToValueAtTime      */
    WA_SEG_EXPONENTIAL,  /* exponentialRampToValueAtTime */
    WA_SEG_TARGET        /* setTargetAtTime              */
} wa_seg_type_t;

typedef struct {
    wa_seg_type_t type;
    float  v0;           /* value at segment start        */
    float  v1;           /* target value                  */
    double t0;           /* segment start, in samples     */
    double t1;           /* segment end, in samples       */
    float  time_const;   /* setTargetAtTime only, samples */
    float  value;        /* current value                 */
} wa_param_t;

static inline void wa_param_init(wa_param_t *p, const float _value)
{
    memset(p, 0, sizeof(*p));
    p->type  = WA_SEG_CONST;
    p->value = _value;
    p->v0    = _value;
    p->v1    = _value;
}

/* setValueAtTime(value, now) */
static inline void wa_set_value(wa_param_t *p, const float _v)
{
    p->type  = WA_SEG_CONST;
    p->value = _v;
    p->v0    = _v;
    p->v1    = _v;
}

/* linearRampToValueAtTime(value, now + duration)
 *   v(t) = v0 + (v1 - v0) * (t - t0) / (t1 - t0)                            */
static inline void wa_linear_ramp(wa_param_t *p, const float _v1,
                                  const double _duration_samples)
{
    p->type = WA_SEG_LINEAR;
    p->v0   = p->value;
    p->v1   = _v1;
    p->t0   = 0.0;
    p->t1   = _duration_samples > 0.0 ? _duration_samples : 1.0;
}

/* exponentialRampToValueAtTime(value, now + duration)
 *   v(t) = v0 * (v1/v0)^((t - t0)/(t1 - t0))
 * Spec: v0 and v1 must be non-zero and same-signed; we clamp like browsers do. */
static inline void wa_exp_ramp(wa_param_t *p, float _v1,
                               const double _duration_samples)
{
    if(_v1 < WA_MIN_EXP) _v1 = WA_MIN_EXP;
    if(p->value < WA_MIN_EXP) p->value = WA_MIN_EXP;

    p->type = WA_SEG_EXPONENTIAL;
    p->v0   = p->value;
    p->v1   = _v1;
    p->t0   = 0.0;
    p->t1   = _duration_samples > 0.0 ? _duration_samples : 1.0;
}

/* setTargetAtTime(target, now, timeConstant)
 *   v(t) = v1 + (v0 - v1) * exp(-(t - t0) / timeConstant)                    */
static inline void wa_set_target(wa_param_t *p, const float _target,
                                 const float _time_constant_samples)
{
    p->type       = WA_SEG_TARGET;
    p->v0         = p->value;
    p->v1         = _target;
    p->t0         = 0.0;
    p->time_const = _time_constant_samples > 0.0f ? _time_constant_samples : 1.0f;
}

/* Advance one sample and return the current value. */
static inline float wa_param_tick(wa_param_t *p)
{
    switch(p->type)
    {
    case WA_SEG_CONST:
        break;

    case WA_SEG_LINEAR:
        if(p->t0 >= p->t1) { p->value = p->v1; p->type = WA_SEG_CONST; break; }
        p->value = p->v0 + (p->v1 - p->v0) * (float)(p->t0 / p->t1);
        p->t0 += 1.0;
        if(p->t0 >= p->t1) { p->value = p->v1; p->type = WA_SEG_CONST; }
        break;

    case WA_SEG_EXPONENTIAL:
        if(p->t0 >= p->t1) { p->value = p->v1; p->type = WA_SEG_CONST; break; }
        p->value = p->v0 * powf(p->v1 / p->v0, (float)(p->t0 / p->t1));
        p->t0 += 1.0;
        if(p->t0 >= p->t1) { p->value = p->v1; p->type = WA_SEG_CONST; }
        break;

    case WA_SEG_TARGET:
        p->value = p->v1 + (p->v0 - p->v1) * expf(-(float)p->t0 / p->time_const);
        p->t0 += 1.0;
        break;
    }
    return p->value;
}

/* ===================================================================== */
/* BiquadFilterNode                                                       */
/* ===================================================================== */

typedef enum {
    WA_LOWPASS = 0,
    WA_HIGHPASS,
    WA_BANDPASS,
    WA_NOTCH,
    WA_PEAKING
} wa_filter_type_t;

typedef struct {
    float b0, b1, b2, a1, a2;   /* normalised coefficients */
    float x1, x2, y1, y2;       /* direct form 1 state     */
} wa_biquad_t;

/*
 * Coefficients exactly as the Web Audio spec defines them (which is the RBJ
 * cookbook). Note the spec's BANDPASS is the constant-0dB-peak-gain variant,
 * and Q is interpreted in dB for lowpass/highpass but linearly for
 * bandpass/notch/peaking — the voices leave Q at default (1) for LP/HP/notch
 * and set it explicitly only on bandpass, so we take Q linearly throughout.
 */
static inline void wa_biquad_set(wa_biquad_t *f, const wa_filter_type_t _type,
                                 const float _freq, const float _q,
                                 const float _gain_db, const float _sample_rate)
{
    float nyquist = _sample_rate * 0.5f;
    float fc = _freq / nyquist;                  /* normalised 0..1 */
    if(fc >= 1.0f) fc = 0.9999f;
    if(fc <= 0.0f) fc = 0.0001f;

    const float w0    = (float)M_PI * fc;
    const float cosw0 = cosf(w0);
    const float sinw0 = sinf(w0);

    float q = _q;
    if(q < 0.0001f) q = 0.0001f;

    const float alpha = sinw0 / (2.0f * q);
    float b0, b1, b2, a0, a1, a2;

    switch(_type)
    {
    case WA_LOWPASS:
        b0 = (1.0f - cosw0) * 0.5f;
        b1 =  1.0f - cosw0;
        b2 = (1.0f - cosw0) * 0.5f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;

    case WA_HIGHPASS:
        b0 =  (1.0f + cosw0) * 0.5f;
        b1 = -(1.0f + cosw0);
        b2 =  (1.0f + cosw0) * 0.5f;
        a0 =   1.0f + alpha;
        a1 =  -2.0f * cosw0;
        a2 =   1.0f - alpha;
        break;

    case WA_BANDPASS:              /* constant 0 dB peak gain */
        b0 =  alpha;
        b1 =  0.0f;
        b2 = -alpha;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;

    case WA_NOTCH:
        b0 =  1.0f;
        b1 = -2.0f * cosw0;
        b2 =  1.0f;
        a0 =  1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha;
        break;

    case WA_PEAKING:
    default: {
        const float A = powf(10.0f, _gain_db / 40.0f);
        b0 =  1.0f + alpha * A;
        b1 = -2.0f * cosw0;
        b2 =  1.0f - alpha * A;
        a0 =  1.0f + alpha / A;
        a1 = -2.0f * cosw0;
        a2 =  1.0f - alpha / A;
        break;
    }
    }

    const float inv_a0 = 1.0f / a0;
    f->b0 = b0 * inv_a0;
    f->b1 = b1 * inv_a0;
    f->b2 = b2 * inv_a0;
    f->a1 = a1 * inv_a0;
    f->a2 = a2 * inv_a0;
}

static inline void wa_biquad_reset(wa_biquad_t *f)
{
    f->x1 = f->x2 = f->y1 = f->y2 = 0.0f;
}

static inline float wa_biquad_tick(wa_biquad_t *f, const float _in)
{
    const float y = f->b0 * _in + f->b1 * f->x1 + f->b2 * f->x2
                                - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1; f->x1 = _in;
    f->y2 = f->y1; f->y1 = y;
    return y;
}

/* ===================================================================== */
/* WaveShaperNode                                                         */
/* ===================================================================== */
/*
 * Saturation curve:
 *   curve[i] = (PI + amount) * x / (PI + amount * |x|),  x = i*2/n - 1
 *
 * The node runs at oversample='2x'. We reproduce that: upsample x2 (linear),
 * shape, then decimate through a halfband-ish lowpass, which is what keeps the
 * saturated bass drum from aliasing.
 */

#define WA_CURVE_SIZE 256

typedef struct {
    float curve[WA_CURVE_SIZE];
    wa_biquad_t up_lp;    /* anti-imaging  */
    wa_biquad_t down_lp;  /* anti-aliasing */
    float prev_in;
} wa_shaper_t;

static inline void wa_shaper_init(wa_shaper_t *s, const float _amount,
                                  const float _sample_rate)
{
    for(int i=0; i<WA_CURVE_SIZE; ++i)
    {
        const float x = (float)i * 2.0f / (float)WA_CURVE_SIZE - 1.0f;
        s->curve[i] = ((float)M_PI + _amount) * x
                    / ((float)M_PI + _amount * fabsf(x));
    }
    /* Filters run at 2x rate; cutoff just under the base-rate Nyquist. */
    wa_biquad_set(&s->up_lp,   WA_LOWPASS, _sample_rate * 0.45f, 0.7071f, 0.0f,
                  _sample_rate * 2.0f);
    wa_biquad_set(&s->down_lp, WA_LOWPASS, _sample_rate * 0.45f, 0.7071f, 0.0f,
                  _sample_rate * 2.0f);
    wa_biquad_reset(&s->up_lp);
    wa_biquad_reset(&s->down_lp);
    s->prev_in = 0.0f;
}

/* Curve lookup with linear interpolation, per spec's shaping function. */
static inline float wa_shaper_lookup(const wa_shaper_t *s, float _x)
{
    if(_x < -1.0f) _x = -1.0f;
    if(_x >  1.0f) _x =  1.0f;
    const float pos = (_x + 1.0f) * 0.5f * (float)(WA_CURVE_SIZE - 1);
    const int   i   = (int)pos;
    const int   j   = i < WA_CURVE_SIZE - 1 ? i + 1 : i;
    const float fr  = pos - (float)i;
    return s->curve[i] + (s->curve[j] - s->curve[i]) * fr;
}

static inline float wa_shaper_tick(wa_shaper_t *s, const float _in)
{
    /* 2x upsample: zero-stuff equivalent via linear interpolation of the pair */
    const float mid = 0.5f * (s->prev_in + _in);
    s->prev_in = _in;

    const float a = wa_shaper_lookup(s, wa_biquad_tick(&s->up_lp, mid));
    const float b = wa_shaper_lookup(s, wa_biquad_tick(&s->up_lp, _in));

    /* decimate: filter both, keep the second */
    wa_biquad_tick(&s->down_lp, a);
    return wa_biquad_tick(&s->down_lp, b);
}

/* ===================================================================== */
/* Oscillator (triangle / sawtooth)                                       */
/* ===================================================================== */

typedef struct {
    double phase;       /* 0..1 */
    double sample_rate;
} wa_osc_t;

static inline void wa_osc_init(wa_osc_t *o, const double _sample_rate)
{
    o->phase = 0.0;
    o->sample_rate = _sample_rate;
}

/* Restart the cycle at a known point. Analog drum voices are shock-excited by
 * the trigger pulse, so every hit starts identically; a free-running phase is
 * a synth behaviour, not a drum one. */
static inline void wa_osc_set_phase(wa_osc_t *o, const double _phase)
{
    o->phase = _phase;
}

static inline float wa_osc_triangle(wa_osc_t *o, const float _freq)
{
    o->phase += (double)_freq / o->sample_rate;
    while(o->phase >= 1.0) o->phase -= 1.0;
    /* Web Audio triangle: rises -1..1 over first half, falls over second */
    const double p = o->phase;
    return (float)(p < 0.5 ? (4.0 * p - 1.0) : (3.0 - 4.0 * p));
}

static inline float wa_osc_sawtooth(wa_osc_t *o, const float _freq)
{
    o->phase += (double)_freq / o->sample_rate;
    while(o->phase >= 1.0) o->phase -= 1.0;
    return (float)(2.0 * o->phase - 1.0);
}

/* ===================================================================== */
/* White noise, uniform in [-1, 1)                                        */
/* ===================================================================== */

typedef struct { uint32_t state; } wa_noise_t;

static inline void wa_noise_init(wa_noise_t *n, const uint32_t _seed)
{
    n->state = _seed ? _seed : 0x12345678u;
}

static inline float wa_noise_tick(wa_noise_t *n)
{
    /* xorshift32 — uniform in [-1,1), matching Math.random()*2-1 */
    n->state ^= n->state << 13;
    n->state ^= n->state >> 17;
    n->state ^= n->state << 5;
    return (float)((double)n->state / 2147483648.0) - 1.0f;
}

#endif /* W9_WEBAUDIO_H */
