/**
 * File name: humanizer.c
 * Project: Geonkick (A percussive synthesizer)
 *
 * Copyright (C) 2025 Iurie Nistor
 *
 * This file is part of Geonkick.
 *
 * GeonKick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "humanizer.h"

#include "qx_math.h"

void gkick_humanizer_init(struct gkick_humanizer *humanizer)
{
        humanizer->enabled = false;
        humanizer->velocity_percent = 5.0f;
        humanizer->timing = 5.0f;
        qx_randomizer_init(&humanizer->velocity_randomizer,
                           -humanizer->velocity_percent, // min %
                           humanizer->velocity_percent,  // max %
                           1.0f);  // resolution %
        qx_randomizer_init(&humanizer->timing_randomizer,
                           0.0f, // min ms
                           humanizer->timing,  // max ms
                           0.25f);  // resolution in ms
}

void gkick_humanizer_enable(struct gkick_humanizer *humanizer,
                            bool enable)
{
        humanizer->enabled = enable;
}

bool gkick_humanizer_is_enabled(const struct gkick_humanizer *humanizer)
{
        return humanizer->enabled;
}

void gkick_humanizer_set_velocity_percent(struct gkick_humanizer *humanizer,
                                          float percent)
{
        humanizer->velocity_percent = percent;
        qx_randomizer_set_range(&humanizer->velocity_randomizer,
                                -humanizer->velocity_percent,
                                humanizer->velocity_percent);
}

float gkick_humanizer_get_velocity_percent(const struct gkick_humanizer *humanizer)
{
        return humanizer->velocity_percent;
}

void gkick_humanizer_velocity(struct gkick_humanizer *humanizer,
                              signed char *velocity)
{
        if (!humanizer->enabled)
                return;

        float r = qx_randomizer_get_float(&humanizer->velocity_randomizer) / 100.0f;
        float v = qx_clamp_float((1.0f + r) * (*velocity), 1.0f, 127.0);
        *velocity = (signed char)v;
}

void gkick_humanizer_set_timing_percent(struct gkick_humanizer *humanizer,
                                        float time)
{
        humanizer->timing = time;
        qx_randomizer_set_range(&humanizer->timing_randomizer,
                                0,
                                humanizer->timing);
}

float gkick_humanizer_get_timing_percent(const struct gkick_humanizer *humanizer)
{
        return  humanizer->timing;
}

void gkick_humanizer_timing(struct gkick_humanizer *humanizer, float *time)
{
        if (!humanizer->enabled) {
                *time = 0.0f;
                return;
        }

        float r = qx_randomizer_get_float(&humanizer->timing_randomizer);
        *time = qx_clamp_float(r, 0.0f, humanizer->timing);
}

