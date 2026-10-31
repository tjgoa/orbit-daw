// Orbit audition adapter, 2026-10-30. GPL-3.0-or-later.
// Real-time, exclusively owned Geonkick oscillators, envelopes, filters and
// distortion; does not use the upstream UI, worker, mixer or sample player.
#include "GeonkickVoice.h"
#include "ThirdParty/Geonkick/oscillator.h"
#include <stdint.h>

#define VOICES 8
#define LAYERS 3
struct OrbitGeonkickVoice {
    struct gkick_oscillator* osc[LAYERS];
    unsigned int age, length;
    float seconds, gain;
    int active;
};
struct OrbitGeonkick {
    struct OrbitGeonkickVoice voices[VOICES];
    double rate;
    float controls[8];
    unsigned int sequence;
};

// Replaces geonkick.c's platform-specific rand/rand_r helper only. Private,
// deterministic state allows simultaneous voices/instances on all platforms.
int geonkick_rand(unsigned int* seed)
{
    *seed = *seed * 1664525u + 1013904223u;
    return (int)((*seed >> 1) % (unsigned int)RAND_MAX);
}
static float bounded(float x, float low, float high)
{ return isfinite(x) ? GKICK_CLAMP(x,low,high) : low; }

// Six preallocated points, mutated only by their audio-thread owner.
static void points(struct gkick_envelope* env, const float x[6], const float y[6])
{
    struct gkick_envelope_point* p=env->first;
    for(int i=0;i<6 && p;i++,p=p->next){p->x=x[i];p->y=y[i];p->is_control=false;}
}
static int reserve_points(struct gkick_envelope* env)
{
    gkick_envelope_clear(env);
    for(int i=0;i<6;i++) {
        struct gkick_envelope_point_info p={(float)i/5,1,false};
        if(!gkick_envelope_add_point(env,&p))return 0;
    }
    gkick_envelope_set_apply_type(env,GEONKICK_ENVELOPE_APPLY_LINEAR);
    return 1;
}
OrbitGeonkick* orbit_geonkick_create(void)
{
    OrbitGeonkick* self=calloc(1,sizeof(*self));
    if(!self)return NULL;
    self->rate=48000;
    const float defaults[8]={0,0,350,65,35,4500,0,-9};
    memcpy(self->controls,defaults,sizeof(defaults));
    for(int v=0;v<VOICES;v++)for(int i=0;i<LAYERS;i++) {
        struct gkick_oscillator* o=gkick_osc_create(48000);
        self->voices[v].osc[i]=o;
        if(!o || !reserve_points(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE])
              || !reserve_points(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE])) {
            orbit_geonkick_destroy(self);return NULL;
        }
        o->filter_enabled=true;
        o->filter->factor=.7f;
        o->filter->coefficients[1]=.7f;
        o->distortion->type=GEONKICK_DISTORTION_SOFT_CLIPPING_TANH;
        o->distortion->in_limiter=1;
        o->distortion->out_limiter=1;
    }
    return self;
}
void orbit_geonkick_destroy(OrbitGeonkick* self)
{
    if(!self)return;
    for(int v=0;v<VOICES;v++)for(int i=0;i<LAYERS;i++)gkick_osc_free(&self->voices[v].osc[i]);
    free(self);
}
void orbit_geonkick_panic(OrbitGeonkick* self)
{ if(self)for(int v=0;v<VOICES;v++)self->voices[v].active=0; }
void orbit_geonkick_prepare(OrbitGeonkick* self,double rate)
{
    if(!self)return;
    self->rate=isfinite(rate) && rate>=8000 && rate<=768000?rate:48000;
    self->sequence=0;
    for(int v=0;v<VOICES;v++)for(int i=0;i<LAYERS;i++) {
        struct gkick_oscillator* o=self->voices[v].osc[i];
        o->sample_rate=(int)self->rate;o->filter->sample_rate=(int)self->rate;o->distortion->sample_rate=(int)self->rate;
    }
    orbit_geonkick_panic(self);
}
void orbit_geonkick_controls(OrbitGeonkick* self,const float p[8])
{
    if(!self || !p)return;
    const float low[8]={0,-24,30,0,0,200,0,-36},high[8]={5,24,2000,100,100,12000,100,6};
    for(int i=0;i<8;i++)self->controls[i]=bounded(p[i],low[i],high[i]);
    // Continuous tone/drive/output automation affects sounding voices too.
    for(int v=0;v<VOICES;v++)for(int i=0;i<LAYERS;i++) {
        struct gkick_oscillator* o=self->voices[v].osc[i];
        o->filter->cutoff_freq=fminf(self->controls[5],(float)self->rate*.18f);
        o->distortion->enabled=self->controls[6]>.001f;
        o->distortion->drive=1+self->controls[6]*.15f;
    }
}
void orbit_geonkick_trigger(OrbitGeonkick* self,int note,int velocity)
{
    if(!self || velocity<=0)return;
    struct OrbitGeonkickVoice* voice=NULL;
    for(int v=0;v<VOICES;v++)if(!self->voices[v].active){voice=&self->voices[v];break;}
    if(!voice) {
        voice=&self->voices[0];
        for(int v=1;v<VOICES;v++)if(self->voices[v].age>voice->age)voice=&self->voices[v];
    }
    const int mode=(int)(self->controls[0]+.5f);
    const float tune=powf(2,((float)GKICK_CLAMP(note,0,127)-36+self->controls[1])/12);
    const float base[6]={52,170,5100,110,900,1500};
    const float noise=self->controls[4]*.01f;
    voice->seconds=self->controls[2]*.001f;
    voice->length=(unsigned int)(self->rate*voice->seconds);
    voice->age=0;voice->gain=bounded((float)velocity/127,0,1);voice->active=1;
    ++self->sequence;
    const float amplitude_x[6]={0,.003f,.10f,.28f,.60f,1};
    const float amplitude_y[6]={0,1,.65f,.25f,.04f,0};
    const float clap_x[6]={0,.02f,.04f,.065f,.1f,1};
    const float clap_y[6]={1,0,1,0,1,0};
    const float frequency_x[6]={0,.025f,.07f,.16f,.35f,1};
    const float sweep=self->controls[3]*.01f;
    const float frequency_y[6]={1+4*sweep,1+1.5f*sweep,1+.3f*sweep,1,1,1};
    const float flat_y[6]={1,1,1,1,1,1};
    for(int i=0;i<LAYERS;i++) {
        struct gkick_oscillator* o=voice->osc[i];
        o->phase=o->initial_phase=0;o->fm_input=0;o->brownian=0;
        o->seed=o->seedp=100u+self->sequence*137u+(unsigned int)i*503u;
        gkick_filter_init(o->filter);
        o->func=GEONKICK_OSC_FUNC_SINE;
        o->frequency=fminf(base[mode]*tune*(i==1?1.47f:1),(float)self->rate*.03f);
        o->amplitude=i==0?.8f:i==1?.12f:noise*.25f;
        if(mode==1){o->amplitude=i==2?noise*.9f:i==0?.6f:.25f;}
        if(mode==2 || mode==5){o->func=GEONKICK_OSC_FUNC_SQUARE;o->frequency=fminf((mode==2?3800:1500)*tune*(i==0?1:i==1?1.34f:1.87f),(float)self->rate*.18f);o->amplitude=.18f;}
        if(mode==3){o->amplitude=i==0?.8f:i==1?.12f:noise*.1f;}
        if(i==2 && mode!=5){o->func=GEONKICK_OSC_FUNC_NOISE_WHITE;o->amplitude=mode==4?.85f:mode==2?noise*.6f:o->amplitude;}
        if(mode==4 && i!=2)o->amplitude=0;
        points(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],mode==4?clap_x:amplitude_x,mode==4?clap_y:amplitude_y);
        points(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],frequency_x,(mode==0 || mode==3)?frequency_y:flat_y);
    }
}
float orbit_geonkick_sample(OrbitGeonkick* self)
{
    if(!self)return 0;
    float result=0;
    for(int v=0;v<VOICES;v++) {
        struct OrbitGeonkickVoice* voice=&self->voices[v];
        if(!voice->active)continue;
        const float time=(float)(voice->age/self->rate);
        float sample=0;
        for(int i=0;i<LAYERS;i++) {
            struct gkick_oscillator* o=voice->osc[i];
            sample+=gkick_osc_value(o,time,voice->seconds);
            gkick_osc_increment_phase(o,time,voice->seconds);
        }
        if(++voice->age>=voice->length)voice->active=0;
        result+=sample*voice->gain;
    }
    result*=powf(10,self->controls[7]/20);
    return isfinite(result)?GKICK_CLAMP(result,-1,1):0;
}
