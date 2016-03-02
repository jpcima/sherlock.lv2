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
#include <stdlib.h>

#include <sherlock.h>

typedef struct _handle_t handle_t;

struct _handle_t {
	LV2_URID_Map *map;
	const LV2_Atom_Sequence *control_in;
	LV2_Atom_Sequence *control_out;
	LV2_Atom_Sequence *notify;
	LV2_Atom_Forge forge;

	uint64_t offset;
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

	lv2_atom_forge_init(&handle->forge, handle->map);

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
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref;

	// size of input sequence
	size_t size = sizeof(LV2_Atom) + handle->control_in->atom.size;
	
	// copy whole input sequence to through port
	capacity = handle->control_out->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->control_out, capacity);
	ref = lv2_atom_forge_raw(forge, handle->control_in, size);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);
	else
		lv2_atom_sequence_clear(handle->control_out);

	// forge whole sequence as single event
	capacity = handle->notify->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);
	ref = lv2_atom_forge_sequence_head(forge, &frame, 0);

	// only serialize sequence to UI if there were actually any events
	if(handle->control_in->atom.size > sizeof(LV2_Atom_Sequence_Body))
	{
		LV2_Atom_Forge_Frame tup_frame;

		if(ref)
			ref = lv2_atom_forge_frame_time(forge, 0);
		if(ref)
			ref = lv2_atom_forge_tuple(forge, &tup_frame);
		if(ref)
			ref = lv2_atom_forge_long(forge, handle->offset);
		if(ref)
			ref = lv2_atom_forge_int(forge, nsamples);
		if(ref)
			ref = lv2_atom_forge_raw(forge, handle->control_in, size);
		if(ref)
			lv2_atom_forge_pad(forge, size);
		if(ref)
			lv2_atom_forge_pop(forge, &tup_frame);

	}

	if(ref)
		lv2_atom_forge_pop(forge, &frame);
	else
		lv2_atom_sequence_clear(handle->notify);

	handle->offset += nsamples;
}

static void
cleanup(LV2_Handle instance)
{
	handle_t *handle = (handle_t *)instance;

	free(handle);
}

const LV2_Descriptor atom_inspector = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= NULL
};
