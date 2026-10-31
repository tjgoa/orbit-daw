// Orbit audition adapter. GPL-3.0-or-later; upstream notices in ThirdParty/Geonkick.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct OrbitGeonkick OrbitGeonkick;
// Each context has one owner. UI code must communicate through host parameters.
// Creation/destruction allocate; prepare, controls, trigger, sample and panic do not.
OrbitGeonkick* orbit_geonkick_create(void);
void orbit_geonkick_destroy(OrbitGeonkick*);
void orbit_geonkick_prepare(OrbitGeonkick*, double sample_rate);
void orbit_geonkick_controls(OrbitGeonkick*, const float values[8]);
void orbit_geonkick_trigger(OrbitGeonkick*, int note, int velocity);
float orbit_geonkick_sample(OrbitGeonkick*);
void orbit_geonkick_panic(OrbitGeonkick*);
#ifdef __cplusplus
}
#endif
