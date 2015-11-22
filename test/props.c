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

#include <stdio.h>

#include <props.h>

#define PROPS_PREFIX		"http://open-music-kontrollers.ch/lv2/props#"
#define PROPS_TEST_URI	PROPS_PREFIX"test"

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Atom_Forge forge;

	props_t *props;
	
	int32_t val1;
	int64_t val2;
	float val3;
	double val4;
	int32_t val5;
	int32_t val6;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
};

static const prop_def_t def1 = {
	.label = "Int",
	.property = PROPS_PREFIX"Int",
	.access = LV2_PATCH__writable,
	.type = LV2_ATOM__Int,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.i = 0,
	.maximum.i = 10,
	.scale_points = NULL
};

static const prop_def_t def2 = {
	.label = "Long",
	.property = PROPS_PREFIX"Long",
	.access = LV2_PATCH__writable,
	.type = LV2_ATOM__Long,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.h = 0,
	.maximum.h = 10,
	.scale_points = NULL
};

static const prop_def_t def3 = {
	.label = "Float",
	.property = PROPS_PREFIX"Float",
	.access = LV2_PATCH__readable,
	.type = LV2_ATOM__Float,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.f = -1.f,
	.maximum.f = 1.f,
	.scale_points = NULL
};

static const prop_def_t def4 = {
	.label = "Double",
	.property = PROPS_PREFIX"Double",
	.access = LV2_PATCH__readable,
	.type = LV2_ATOM__Double,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.d = -1.0,
	.maximum.d = 1.0,
	.scale_points = NULL
};

static const prop_scale_point_t scale_points5 [] = {
	{.label = "One",		.value.i = 0},
	{.label = "Two",		.value.i = 1},
	{.label = "Three",	.value.i = 2},
	{.label = "Four",		.value.i = 3},

	{.label = NULL }
};

static const prop_def_t def5 = {
	.label = "scaleInt",
	.property = PROPS_PREFIX"scaleInt",
	.access = LV2_PATCH__writable,
	.type = LV2_ATOM__Int,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.i = 0,
	.maximum.i = 4,
	.scale_points = scale_points5
};

static const prop_def_t def6 = {
	.label = "Bool",
	.property = PROPS_PREFIX"Bool",
	.access = LV2_PATCH__writable,
	.type = LV2_ATOM__Bool,
	.mode = PROP_MODE_DYNAMIC,
	.minimum.d = 0,
	.maximum.d = 1,
	.scale_points = NULL
};

const unsigned max_nprops = 32;

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	const char *bundle_path, const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
	}

	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	lv2_atom_forge_init(&handle->forge, handle->map);
	handle->props = props_new(max_nprops, descriptor->URI, handle->map, 48000.0);
	if(!handle->props)
	{
		fprintf(stderr, "failed to allocate property structure\n");
		free(handle);
		return NULL;
	}

	handle->val1 = 2;
	handle->val2 = 3;
	handle->val3 = 0.2f;
	handle->val4 = 0.3;
	handle->val5 = 0;
	handle->val6 = true;

	props_register(handle->props, &def1, &handle->val1);
	props_register(handle->props, &def2, &handle->val2);
	props_register(handle->props, &def3, &handle->val3);
	props_register(handle->props, &def4, &handle->val4);
	props_register(handle->props, &def5, &handle->val5);
	props_register(handle->props, &def6, &handle->val6);

	props_sort(handle->props);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	plughandle_t *handle = (plughandle_t *)instance;

	switch(port)
	{
		case 0:
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->event_out = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	plughandle_t *handle = instance;

	uint32_t capacity = handle->event_out->atom.size;
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	LV2_Atom_Forge_Ref ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		if(ref)
			props_advance(handle->props, &handle->forge, ev->time.frames, obj, &ref); //TODO handle return
	}
	if(ref)
		lv2_atom_forge_pop(&handle->forge, &frame);
	else
		lv2_atom_sequence_clear(handle->event_out);
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	props_free(handle->props);
	free(handle);
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = (plughandle_t *)instance;

	return props_save(handle->props, &handle->forge, store, state, flags, features);
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = (plughandle_t *)instance;

	return props_restore(handle->props, &handle->forge, retrieve, state, flags, features);
}

LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

static const void *
extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	return NULL;
}

const LV2_Descriptor props_test = {
	.URI						= PROPS_TEST_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &props_test;
		default:
			return NULL;
	}
}
