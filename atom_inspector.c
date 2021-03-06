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

typedef struct _handle_t handle_t;

struct _handle_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	const LV2_Atom_Sequence *control;
	craft_t through;
	craft_t notify;

	LV2_URID time_position;
	LV2_URID time_frame;

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
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
	}

	if(!handle->map || !handle->unmap)
	{
		fprintf(stderr, "%s: Host does not support urid:(un)map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	handle->time_position = handle->map->map(handle->map->handle, LV2_TIME__Position);
	handle->time_frame = handle->map->map(handle->map->handle, LV2_TIME__frame);

	lv2_atom_forge_init(&handle->through.forge, handle->map);
	lv2_atom_forge_init(&handle->notify.forge, handle->map);

	if(!props_init(&handle->props, descriptor->URI,
		defs, MAX_NPROPS, &handle->state, &handle->stash,
		handle->map, handle))
	{
		fprintf(stderr, "failed to allocate property structure\n");
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
			handle->control = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->through.seq = (LV2_Atom_Sequence *)data;
			break;
		case 2:
			handle->notify.seq = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	handle_t *handle = (handle_t *)instance;
	craft_t *through = &handle->through;
	craft_t *notify = &handle->notify;

	uint32_t capacity = through->seq->atom.size;
	lv2_atom_forge_set_buffer(&through->forge, through->buf, capacity);
	through->ref = lv2_atom_forge_sequence_head(&through->forge, &through->frame[0], 0);

	capacity = notify->seq->atom.size;
	lv2_atom_forge_set_buffer(&notify->forge, notify->buf, capacity);
	notify->ref = lv2_atom_forge_sequence_head(&notify->forge, &notify->frame[0], 0);

	props_idle(&handle->props, &notify->forge, 0, &notify->ref);

	LV2_ATOM_SEQUENCE_FOREACH(handle->control, ev)
	{
		const LV2_Atom *atom = &ev->body;
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
		const int64_t frames = ev->time.frames;

		if(handle->state.trace && handle->log)
		{
			if(lv2_atom_forge_is_object_type(&through->forge, atom->type))
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", object, %s\n", ev->time.frames,
					handle->unmap->unmap(handle->unmap->handle, obj->body.otype));
				//FIXME introspect object?
			}
			else if(atom->type == through->forge.Bool)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", bool  , %s\n", ev->time.frames,
					((const LV2_Atom_Bool *)atom)->body ? "true" : "false");
			}
			else if(atom->type == through->forge.Int)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", int32 , %"PRIi32"\n", ev->time.frames,
					((const LV2_Atom_Int *)atom)->body);
			}
			else if(atom->type == through->forge.Long)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", int64 , %"PRIi64"\n", ev->time.frames,
					((const LV2_Atom_Long *)atom)->body);
			}
			else if(atom->type == through->forge.Float)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", flt32 , %f\n", ev->time.frames,
					((const LV2_Atom_Float *)atom)->body);
			}
			else if(atom->type == through->forge.Double)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", flt64 , %lf\n", ev->time.frames,
					((const LV2_Atom_Double *)atom)->body);
			}
			else if(atom->type == through->forge.String)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", urid  , %s\n", ev->time.frames,
					handle->unmap->unmap(handle->unmap->handle, ((const LV2_Atom_URID *)atom)->body));
			}
			else if(atom->type == through->forge.String)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", string, %s\n", ev->time.frames,
					(const char *)LV2_ATOM_BODY_CONST(atom));
			}
			else if(atom->type == through->forge.URI)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", uri   , %s\n", ev->time.frames,
					(const char *)LV2_ATOM_BODY_CONST(atom));
			}
			else if(atom->type == through->forge.Path)
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", path  , %s\n", ev->time.frames,
					(const char *)LV2_ATOM_BODY_CONST(atom));
			}
			//FIXME more types
			else
			{
				lv2_log_trace(&handle->logger, "%4"PRIi64", %s\n", ev->time.frames,
					handle->unmap->unmap(handle->unmap->handle, atom->type));
			}
		}

		// copy all events to through port
		if(through->ref)
			through->ref = lv2_atom_forge_frame_time(&through->forge, frames);
		if(through->ref)
			through->ref = lv2_atom_forge_raw(&through->forge, &obj->atom, lv2_atom_total_size(&obj->atom));
		if(through->ref)
			lv2_atom_forge_pad(&through->forge, obj->atom.size);

		if(  !props_advance(&handle->props, &notify->forge, frames, obj, &notify->ref)
			&& lv2_atom_forge_is_object_type(&notify->forge, obj->atom.type)
			&& (obj->body.otype == handle->time_position) )
		{
			const LV2_Atom_Long *time_frame = NULL;
			lv2_atom_object_get(obj, handle->time_frame, &time_frame, NULL);
			if(time_frame)
				handle->frame = time_frame->body - frames;
		}
	}

	if(through->ref)
		lv2_atom_forge_pop(&through->forge, &through->frame[0]);
	else
	{
		lv2_atom_sequence_clear(through->seq);

		if(handle->log)
			lv2_log_trace(&handle->logger, "through buffer overflow\n");
	}

	bool has_event = notify->seq->atom.size > sizeof(LV2_Atom_Sequence_Body);

	if(notify->ref)
		notify->ref = lv2_atom_forge_frame_time(&notify->forge, nsamples-1);
	if(notify->ref)
		notify->ref = lv2_atom_forge_tuple(&notify->forge, &notify->frame[1]);
	if(notify->ref)
		notify->ref = lv2_atom_forge_long(&notify->forge, handle->frame);
	if(notify->ref)
		notify->ref = lv2_atom_forge_int(&notify->forge, nsamples);
	if(notify->ref)
		notify->ref = lv2_atom_forge_sequence_head(&notify->forge, &notify->frame[2], 0);

	// only serialize filtered events to UI
	LV2_ATOM_SEQUENCE_FOREACH(handle->control, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		const bool type_matches = lv2_atom_forge_is_object_type(&notify->forge, obj->atom.type)
			? (obj->body.otype == handle->state.filter)
			: (obj->atom.type == handle->state.filter);

		if(  (!handle->state.negate && type_matches)
			|| (handle->state.negate && !type_matches) )
		{
			has_event = true;
			if(notify->ref)
				notify->ref = lv2_atom_forge_frame_time(&notify->forge, ev->time.frames);
			if(notify->ref)
				notify->ref = lv2_atom_forge_write(&notify->forge, &ev->body, sizeof(LV2_Atom) + ev->body.size);
		}
	}

	if(notify->ref)
		lv2_atom_forge_pop(&notify->forge, &notify->frame[2]);
	if(notify->ref)
		lv2_atom_forge_pop(&notify->forge, &notify->frame[1]);
	if(notify->ref)
		lv2_atom_forge_pop(&notify->forge, &notify->frame[0]);
	else
	{
		lv2_atom_sequence_clear(notify->seq);

		if(handle->log)
			lv2_log_trace(&handle->logger, "notify buffer overflow\n");
	}

	if(!has_event) // don't send anything
		lv2_atom_sequence_clear(notify->seq);

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

static const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;

	return NULL;
}

const LV2_Descriptor atom_inspector = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
