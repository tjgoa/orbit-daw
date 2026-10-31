/* Orbit port. GPL-3.0 with additional terms; modified version.
 * 9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct OrbitNineKit OrbitNineKit;
OrbitNineKit* orbit_nine_create(void);
void orbit_nine_destroy(OrbitNineKit*);
void orbit_nine_prepare(OrbitNineKit*,double rate);
float orbit_nine_get(const OrbitNineKit*,const char* key);
void orbit_nine_set(OrbitNineKit*,const char* key,float value);
void orbit_nine_tempo(OrbitNineKit*,float bpm);
void orbit_nine_trigger(OrbitNineKit*,int instrument,int velocity,int accent,float amount);
void orbit_nine_render(OrbitNineKit*,float* output,int frames);
void orbit_nine_panic(OrbitNineKit*);
#ifdef __cplusplus
}
#endif
