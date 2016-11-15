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

#ifndef _SHERLOCK_NK_H
#define _SHERLOCK_NK_H

#include "nk_pugl/nk_pugl.h"

#include <osc.lv2/osc.h>
#include <sratom/sratom.h>

typedef struct _plughandle_t plughandle_t;
typedef struct _atom_ser_t atom_ser_t;

struct _atom_ser_t {
	uint32_t size;
	uint32_t offset;
	union {
		uint8_t *buf;
		LV2_Atom *atom;
	};
};

struct _plughandle_t {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge mem;
	int32_t count;
	bool bottom;
	LV2_Atom_Forge_Frame frame;
	LV2_URID event_transfer;
	LV2_OSC_URID osc_urid;

	PROPS_T(props, MAX_NPROPS);
	struct {
		LV2_URID count;
		LV2_URID overwrite;
		LV2_URID block;
		LV2_URID follow;
	} urid;
	state_t state;
	state_t stash;

	nk_pugl_window_t win;

	atom_ser_t ser;
	const LV2_Atom *selected;
	struct nk_str str;

	Sratom *sratom;
	const char *base_uri;

	float dy;
};

extern const char *max_items [5];
extern const int32_t max_values [5];

extern const struct nk_color white;
extern const struct nk_color gray;
extern const struct nk_color yellow;
extern const struct nk_color magenta;
extern const struct nk_color green;
extern const struct nk_color blue;
extern const struct nk_color orange;
extern const struct nk_color violet;
extern const struct nk_color red;

void
_midi_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_atom_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_osc_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_empty(struct nk_context *ctx);

void
_toggle(plughandle_t *handle, LV2_URID property, int32_t val, bool is_bool);

void
_clear(plughandle_t *handle);

void
_ruler(struct nk_context *ctx, float line_thickness, struct nk_color color);

void
_post_redisplay(plughandle_t *handle);

#endif // _SHERLOCK_NK_H
