/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
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
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
	
#define LV2_OSC__OscEvent					"http://opensoundcontrol.org#OscEvent"

#define SHERLOCK_URI							"http://open-music-kontrollers.ch/lv2/sherlock"

#define SHERLOCK_OBJECT_URI				SHERLOCK_URI"#object"
#define SHERLOCK_EVENT_URI				SHERLOCK_URI"#frametime"
#define SHERLOCK_FRAMETIME_URI		SHERLOCK_URI"#event"

#define SHERLOCK_ATOM_URI					SHERLOCK_URI"#atom"
#define SHERLOCK_ATOM_UI_URI			SHERLOCK_URI"#atom_ui"
#define SHERLOCK_ATOM_EO_URI			SHERLOCK_URI"#atom_eo"

const LV2_Descriptor atom;
const LV2UI_Descriptor atom_ui;
const LV2UI_Descriptor atom_eo;

#endif // _SHERLOCK_LV2_H
