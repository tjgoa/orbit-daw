/**
 * File name: humanizer.h
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

#ifndef GKICK_HUMANIZER_H
#define GKICK_HUMANIZER_H

#include "qx_randomizer.h"

struct gkick_humanizer {
        bool enabled;
        float velocity_percent;
        float timing;
        struct qx_randomizer velocity_randomizer;
        struct qx_randomizer timing_randomizer;
};

void gkick_humanizer_init(struct gkick_humanizer *humanizer);
void gkick_humanizer_enable(struct gkick_humanizer *humanizer,
                            bool enable);
bool gkick_humanizer_is_enabled(const struct gkick_humanizer *humanizer);
float gkick_humanizer_get_velocity_percent(const struct gkick_humanizer *humanizer);
void gkick_humanizer_set_velocity_percent(struct gkick_humanizer *humanizer,
                                          float percent);
void gkick_humanizer_velocity(struct gkick_humanizer *humanizer,
                              signed char *velocity);
float gkick_humanizer_get_timing_percent(const struct gkick_humanizer *humanizer);
void gkick_humanizer_set_timing_percent(struct gkick_humanizer *humanizer,
                                          float percent);
void gkick_humanizer_timing(struct gkick_humanizer *humanizer,
                            float *time);

#endif // GKICK_HUMANIZER_H
