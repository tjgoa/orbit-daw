/* Orbit port. GPL-3.0 with additional terms; modified version.
 * 9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9 */
#include "NineKitVoice.h"
#include <stdlib.h>
#include "ThirdParty/9W9/9w9_engine.h"
#include "NineKitSamples.h"
struct OrbitNineKit {w9_engine_t engine;int muted;};
static void bind_samples(w9_engine_t* e)
{
    const float* buffers[4]={orbit_nine_hh,orbit_nine_ride,orbit_nine_crash,orbit_nine_hh};
    const unsigned lengths[4]={sizeof(orbit_nine_hh)/sizeof(float),sizeof(orbit_nine_ride)/sizeof(float),sizeof(orbit_nine_crash)/sizeof(float),sizeof(orbit_nine_hh)/sizeof(float)};
    for(int i=0;i<4;++i){e->sampler[i].buffer=buffers[i];e->sampler[i].length=lengths[i];e->sampler[i].rate_ratio=44100.0f/e->sample_rate;}
}
OrbitNineKit* orbit_nine_create(void)
{
    OrbitNineKit* p=calloc(1,sizeof(*p));if(p)orbit_nine_prepare(p,44100);return p;
}
void orbit_nine_destroy(OrbitNineKit* p){free(p);}
void orbit_nine_prepare(OrbitNineKit* p,double rate)
{
    if(!p)return;
    w9_engine_init(&p->engine,(float)(isfinite(rate) && rate>=22050 && rate<=192000?rate:44100),NULL);
    bind_samples(&p->engine);
    p->muted=1;
}
float orbit_nine_get(const OrbitNineKit* p,const char* key)
{float value=0;if(p && key)w9_engine_get_param(&p->engine,key,&value);return value;}
void orbit_nine_set(OrbitNineKit* p,const char* key,float value)
{if(p && key && isfinite(value))w9_engine_set_param(&p->engine,key,value);}
void orbit_nine_tempo(OrbitNineKit* p,float bpm)
{if(p && isfinite(bpm))w9_engine_set_raw(&p->engine,"dly_bpm",fmaxf(20,fminf(bpm,999)));}
void orbit_nine_trigger(OrbitNineKit* p,int instrument,int velocity,int accent,float amount)
{
    if(!p || instrument<0 || instrument>=11 || velocity<=0)return;
    p->muted=0;
    float base=p->engine.master.accent;
    if(accent && isfinite(amount))p->engine.master.accent*=1+fmaxf(0,fminf(amount,100))*.01f;
    w9_engine_trigger(&p->engine,(w9_trigger_t)instrument,velocity);
    p->engine.master.accent=base;
}
void orbit_nine_render(OrbitNineKit* p,float* output,int frames)
{
    if(!p || !output || frames<=0)return;
    w9_engine_render(&p->engine,output,frames);
    for(int n=0;n<frames;++n)if(p->muted || !isfinite(output[n]))output[n]=0;
}
void orbit_nine_panic(OrbitNineKit* p)
{
    if(!p)return;w9_engine_t* e=&p->engine;
    p->muted=1;
    for(int i=0;i<5;++i)e->bt[i].mute_countdown=0;
    for(int i=0;i<3;++i)e->tom909[i].mute_countdown=0;
    e->rim909.mute_countdown=e->clap909.mute_countdown=0;
    for(int i=0;i<4;++i){e->sampler[i].mute_countdown=0;e->sampler[i].pos=-1;wa_set_value(&e->sampler[i].out,0);}
    memset(e->verb.comb,0,sizeof(e->verb.comb));memset(e->verb.apb,0,sizeof(e->verb.apb));memset(e->verb.cpos,0,sizeof(e->verb.cpos));memset(e->verb.apos,0,sizeof(e->verb.apos));memset(e->verb.cdmp,0,sizeof(e->verb.cdmp));wa_biquad_reset(&e->verb.hp);
    memset(e->dly.buf,0,sizeof(e->dly.buf));e->dly.w=0;e->dly.lp=0;wa_biquad_reset(&e->dly.hp);
    e->master.comp_env_db=e->master.comp_det=0;memset(e->master.crush_st,0,sizeof(e->master.crush_st));
}
