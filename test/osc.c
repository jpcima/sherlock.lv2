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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <osc.h>
#include <writer.h>
#include <reader.h>
#include <forge.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#define LV2_OSC_TEST_URI    LV2_OSC_PREFIX"test"

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Ref ref;

	LV2_OSC_Schedule *sched;
	LV2_URID osc_event;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
};

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
		else if(!strcmp(features[i]->URI, LV2_OSC__schedule))
			handle->sched= features[i]->data;
	}

	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	handle->osc_event = handle->map->map(handle->map->handle, LV2_OSC__Event);

	lv2_atom_forge_init(&handle->forge, handle->map);

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

static inline void
_osc_packet(plughandle_t *handle, LV2_OSC_Reader *reader, uint64_t timetag, int32_t size)
{
	if(osc_reader_is_bundle(reader))
	{
		OSC_READER_BUNDLE_FOREACH(reader, itm, size)
			_osc_packet(handle, reader, itm->timetag, itm->size);
	}
	else if(osc_reader_is_message(reader))
	{
		OSC_READER_MESSAGE_FOREACH(reader, arg, size)
			printf("(%c %u) ", *arg->type, arg->size);
		printf("\n");
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	plughandle_t *handle = instance;

	uint32_t capacity = handle->event_out->atom.size;
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	handle->ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const LV2_Atom *atom = &ev->body;

		if(atom->type == handle->osc_event)
		{
			const uint8_t *body = LV2_ATOM_BODY_CONST(atom);
			LV2_OSC_Reader reader;
			osc_reader_initialize(&reader, body, atom->size);

			_osc_packet(handle, &reader, LV2_OSC_IMMEDIATE, atom->size);
		}
	}

	if(handle->ref)
		handle->ref = osc_forge_message_vararg(&handle->forge, handle->osc_event, "/", "");

	if(handle->ref)
		lv2_atom_forge_pop(&handle->forge, &frame);
	else
		lv2_atom_sequence_clear(handle->event_out);
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	free(handle);
}

const LV2_Descriptor osc_test = {
	.URI						= LV2_OSC_TEST_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= NULL
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
			return &osc_test;
		default:
			return NULL;
	}
}
