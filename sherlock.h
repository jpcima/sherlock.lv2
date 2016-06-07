/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _SHERLOCK_LV2_H
#define _SHERLOCK_LV2_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
	
#define LV2_OSC__OscEvent								"http://opensoundcontrol.org#OscEvent"

#define SHERLOCK_URI										"http://open-music-kontrollers.ch/lv2/sherlock"

#define SHERLOCK_ATOM_INSPECTOR_URI			SHERLOCK_URI"#atom_inspector"
#define SHERLOCK_ATOM_INSPECTOR_UI_URI	SHERLOCK_URI"#atom_inspector_1_ui"
#define SHERLOCK_ATOM_INSPECTOR_KX_URI	SHERLOCK_URI"#atom_inspector_2_kx"
#define SHERLOCK_ATOM_INSPECTOR_EO_URI	SHERLOCK_URI"#atom_inspector_3_eo"

#define SHERLOCK_MIDI_INSPECTOR_URI			SHERLOCK_URI"#midi_inspector"
#define SHERLOCK_MIDI_INSPECTOR_UI_URI	SHERLOCK_URI"#midi_inspector_1_ui"
#define SHERLOCK_MIDI_INSPECTOR_KX_URI	SHERLOCK_URI"#midi_inspector_2_kx"
#define SHERLOCK_MIDI_INSPECTOR_EO_URI	SHERLOCK_URI"#midi_inspector_3_eo"

#define SHERLOCK_OSC_INSPECTOR_URI			SHERLOCK_URI"#osc_inspector"
#define SHERLOCK_OSC_INSPECTOR_UI_URI		SHERLOCK_URI"#osc_inspector_1_ui"
#define SHERLOCK_OSC_INSPECTOR_KX_URI		SHERLOCK_URI"#osc_inspector_2_kx"
#define SHERLOCK_OSC_INSPECTOR_EO_URI		SHERLOCK_URI"#osc_inspector_3_eo"

extern const LV2_Descriptor atom_inspector;
extern const LV2UI_Descriptor atom_inspector_ui;
extern const LV2UI_Descriptor atom_inspector_kx;
extern const LV2UI_Descriptor atom_inspector_eo;

extern const LV2_Descriptor midi_inspector;
extern const LV2UI_Descriptor midi_inspector_ui;
extern const LV2UI_Descriptor midi_inspector_kx;
extern const LV2UI_Descriptor midi_inspector_eo;

extern const LV2_Descriptor osc_inspector;
extern const LV2UI_Descriptor osc_inspector_ui;
extern const LV2UI_Descriptor osc_inspector_kx;
extern const LV2UI_Descriptor osc_inspector_eo;

typedef struct _position_t position_t;

struct _position_t {
	uint64_t offset;
	uint32_t nsamples;
};

#endif // _SHERLOCK_LV2_H
