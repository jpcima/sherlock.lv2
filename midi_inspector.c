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

#include <stdio.h>
#include <stdlib.h>

#include <sherlock.h>

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

typedef struct _handle_t handle_t;

struct _handle_t {
	LV2_URID_Map *map;
	const LV2_Atom_Sequence *control_in;
	LV2_Atom_Sequence *control_out;
	LV2_Atom_Sequence *notify;
	LV2_Atom_Forge forge;

	LV2_URID time_position;
	LV2_URID time_frame;
	LV2_URID midi_event;

	int64_t frame;

	PROPS_T(props, MAX_NPROPS);
	state_t state;
	state_t stash;
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	const char *bundle_path, const LV2_Feature *const *features)
{
	int i;
	handle_t *handle = calloc(1, sizeof(handle_t));
	if(!handle)
		return NULL;

	for(i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = (LV2_URID_Map *)features[i]->data;

	if(!handle->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	handle->time_position = handle->map->map(handle->map->handle, LV2_TIME__Position);
	handle->time_frame = handle->map->map(handle->map->handle, LV2_TIME__frame);

	handle->midi_event = handle->map->map(handle->map->handle, LV2_MIDI__MidiEvent);

	lv2_atom_forge_init(&handle->forge, handle->map);

	if(!props_init(&handle->props, MAX_NPROPS, descriptor->URI, handle->map, handle))
	{
		fprintf(stderr, "failed to allocate property structure\n");
		free(handle);
		return NULL;
	}

	if(!props_register(&handle->props, defs, MAX_NPROPS, &handle->state, &handle->stash))
	{
		free(handle);
		return NULL;
	}

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	handle_t *handle = (handle_t *)instance;

	switch(port)
	{
		case 0:
			handle->control_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->control_out = (LV2_Atom_Sequence *)data;
			break;
		case 2:
			handle->notify = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	handle_t *handle = (handle_t *)instance;
	uint32_t capacity;
	LV2_Atom_Forge *forge = &handle->forge;
	LV2_Atom_Forge_Frame frame [3];
	LV2_Atom_Forge_Ref ref;

	// size of input sequence
	const size_t size = lv2_atom_total_size(&handle->control_in->atom);
	
	// copy whole input sequence to through port
	capacity = handle->control_out->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->control_out, capacity);
	ref = lv2_atom_forge_sequence_head(forge, frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(handle->control_in, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
		const int64_t frames = ev->time.frames;

		// copy all events to through port
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, frames);
		if(ref)
			ref = lv2_atom_forge_raw(forge, &obj->atom, lv2_atom_total_size(&obj->atom));
		if(ref)
			lv2_atom_forge_pad(forge, obj->atom.size);

		if(  !props_advance(&handle->props, forge, frames, obj, &ref)
			&& lv2_atom_forge_is_object_type(forge, obj->atom.type)
			&& (obj->body.otype == handle->time_position) )
		{
			const LV2_Atom_Long *time_frame = NULL;
			lv2_atom_object_get(obj, handle->time_frame, &time_frame, NULL);
			if(time_frame)
				handle->frame = time_frame->body - frames;
		}
	}

	if(ref)
		lv2_atom_forge_pop(forge, frame);
	else
		lv2_atom_sequence_clear(handle->control_out);

	// forge whole sequence as single event
	capacity = handle->notify->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);

	bool has_midi = false;

	ref = lv2_atom_forge_sequence_head(forge, &frame[0], 0);
	if(ref)
		ref = lv2_atom_forge_frame_time(forge, 0);
	if(ref)
		ref = lv2_atom_forge_tuple(forge, &frame[1]);
	if(ref)
		ref = lv2_atom_forge_long(forge, handle->frame);
	if(ref)
		ref = lv2_atom_forge_int(forge, nsamples);
	if(ref)
		ref = lv2_atom_forge_sequence_head(forge, &frame[2], 0);

	// only serialize MIDI events to UI
	LV2_ATOM_SEQUENCE_FOREACH(handle->control_in, ev)
	{
		if(ev->body.type == handle->midi_event)
		{
			has_midi = true;
			if(ref)
				ref = lv2_atom_forge_frame_time(forge, ev->time.frames);
			if(ref)
				ref = lv2_atom_forge_write(forge, &ev->body, sizeof(LV2_Atom) + ev->body.size);
		}
	}

	if(ref)
		lv2_atom_forge_pop(forge, &frame[2]);
	if(ref)
		lv2_atom_forge_pop(forge, &frame[1]);
	if(ref)
		lv2_atom_forge_pop(forge, &frame[0]);
	else
		lv2_atom_sequence_clear(handle->notify);

	if(!has_midi) // don't send anything
		lv2_atom_sequence_clear(handle->notify);

	handle->frame += nsamples;
}

static void
cleanup(LV2_Handle instance)
{
	handle_t *handle = (handle_t *)instance;

	free(handle);
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	handle_t *handle = instance;

	return props_save(&handle->props, store, state, flags, features);
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	handle_t *handle = instance;

	return props_restore(&handle->props, retrieve, state, flags, features);
}

static const LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

static inline LV2_Worker_Status
_work(LV2_Handle instance, LV2_Worker_Respond_Function respond,
LV2_Worker_Respond_Handle worker, uint32_t size, const void *body)
{
	handle_t *handle = instance;

	return props_work(&handle->props, respond, worker, size, body);
}

static inline LV2_Worker_Status
_work_response(LV2_Handle instance, uint32_t size, const void *body)
{
	handle_t *handle = instance;

	return props_work_response(&handle->props, size, body);
}

static const LV2_Worker_Interface work_iface = {
	.work = _work,
	.work_response = _work_response,
	.end_run = NULL
};

static const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else if(!strcmp(uri, LV2_WORKER__interface))
		return &work_iface;

	return NULL;
}

const LV2_Descriptor midi_inspector = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
