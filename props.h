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

#ifndef _LV2_PROPS_H_
#define _LV2_PROPS_H_

#include <stdlib.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>

typedef union _props_value_raw_t props_value_raw_t;
typedef union _props_value_ptr_t props_value_ptr_t;

typedef enum _props_mode_t props_mode_t;
typedef enum _props_event_t props_event_t;
typedef struct _props_scale_point_t props_scale_point_t;
typedef struct _props_def_t props_def_t;
typedef struct _props_impl_t props_impl_t;
typedef struct _props_t props_t;
typedef LV2_Atom_Forge_Ref (*props_cb_t)(props_t *props, LV2_Atom_Forge *forge,
	int64_t frames, props_event_t event, props_impl_t *impl, const LV2_Atom *value);

union _props_value_raw_t {
	const int32_t i;			// Int
	const int64_t h;			// Long
	const float f;				// Float
	const double d;				// Double
	const int32_t b;			// Bool
	const uint32_t u;			// URID
	const char *s;				// String
	//TODO
};

union _props_value_ptr_t {
	int32_t *i;
	int64_t *h;
	float *f;
	double *d;
	int32_t *b;
	uint32_t *u;
	char *s;
	//TODO
	void *p;
};

enum _props_mode_t {
	PROP_MODE_STATIC,
	PROP_MODE_DYNAMIC
};

enum _props_event_t {
	PROP_EVENT_GET,
	PROP_EVENT_SET,
	PROP_EVENT_REG
};

struct _props_scale_point_t {
	const char *label;
	props_value_raw_t value;
};

struct _props_def_t {
	const char *label;
	const char *property;
	const char *access;
	const char *type;
	props_mode_t mode;

	props_value_raw_t minimum;
	props_value_raw_t maximum;
	const props_scale_point_t *scale_points;
};

struct _props_impl_t {
	LV2_URID property;
	LV2_URID access;
	LV2_URID type;
	const props_def_t *def;
	props_cb_t cb;
	props_value_ptr_t value;
};

struct _props_t {
	struct {
		LV2_URID subject;

		LV2_URID patch_get;
		LV2_URID patch_set;
		LV2_URID patch_patch;
		LV2_URID patch_wildcard;
		LV2_URID patch_add;
		LV2_URID patch_remove;
		LV2_URID patch_subject;
		LV2_URID patch_property;
		LV2_URID patch_value;
		LV2_URID patch_writable;
		LV2_URID patch_readable;

		LV2_URID rdf_value;

		LV2_URID rdfs_label;
		LV2_URID rdfs_range;

		LV2_URID lv2_minimum;
		LV2_URID lv2_maximum;
		LV2_URID lv2_scale_point;
	} urid;

	LV2_URID_Map *map;

	unsigned max_nimpls;
	unsigned nimpls;
	props_impl_t impls [0];
};

static inline props_t *
props_new(const size_t max_nimpls, const char *subject, LV2_URID_Map *map)
{
	props_t *props = calloc(1, sizeof(props_t) + max_nimpls*sizeof(props_impl_t));
	if(!props)
		return NULL;

	props->nimpls = 0;
	props->max_nimpls = max_nimpls;
	props->map = map;
	
	props->urid.subject = map->map(map->handle, subject);
	
	props->urid.patch_get = map->map(map->handle, LV2_PATCH__Get);
	props->urid.patch_set = map->map(map->handle, LV2_PATCH__Set);
	props->urid.patch_patch = map->map(map->handle, LV2_PATCH__Patch);
	props->urid.patch_wildcard = map->map(map->handle, LV2_PATCH__wildcard);
	props->urid.patch_add = map->map(map->handle, LV2_PATCH__add);
	props->urid.patch_remove = map->map(map->handle, LV2_PATCH__remove);
	props->urid.patch_subject = map->map(map->handle, LV2_PATCH__subject);
	props->urid.patch_property = map->map(map->handle, LV2_PATCH__property);
	props->urid.patch_value = map->map(map->handle, LV2_PATCH__value);
	props->urid.patch_writable = map->map(map->handle, LV2_PATCH__writable);
	props->urid.patch_readable = map->map(map->handle, LV2_PATCH__readable);
	
	props->urid.rdf_value = map->map(map->handle,
		"http://www.w3.org/1999/02/22-rdf-syntax-ns#value");

	props->urid.rdfs_label = map->map(map->handle,
		"http://www.w3.org/2000/01/rdf-schema#label");
	props->urid.rdfs_range = map->map(map->handle,
		"http://www.w3.org/2000/01/rdf-schema#range");

	props->urid.lv2_minimum = map->map(map->handle, LV2_CORE__minimum);
	props->urid.lv2_maximum = map->map(map->handle, LV2_CORE__maximum);
	props->urid.lv2_scale_point = map->map(map->handle, LV2_CORE__scalePoint);

	//TODO
	return props;
}

static inline void
props_free(props_t *props)
{
	free(props);
}

static inline void
props_clear(props_t *props)
{
	for(unsigned i = 0; i< props->nimpls; i++)
	{
		props_impl_t *impl = &props->impls[i];
		//TODO deregister?
	}

	props->nimpls = 0;
}

static inline int
_signum(LV2_URID urid1, LV2_URID urid2)
{
	if(urid1 < urid2)
		return -1;
	else if(urid1 > urid2)
		return 1;
	
	return 0;
}

static int
_cmp_sort(const void *itm1, const void *itm2)
{
	const props_impl_t *impl1 = itm1;
	const props_impl_t *impl2 = itm2;

	return _signum(impl1->property, impl2->property);
}

static inline void
props_sort(props_t *props)
{
	qsort(props->impls, props->nimpls, sizeof(props_impl_t), _cmp_sort);
}

static int
_cmp_search(const void *itm1, const void *itm2)
{
	const LV2_URID *property = itm1;
	const props_impl_t *impl = itm2;

	return _signum(*property, impl->property);
}

static inline props_impl_t *
_props_search(props_t *props, LV2_URID property)
{
	return bsearch(&property, props->impls, props->nimpls, sizeof(props_impl_t), _cmp_search);
}

static inline LV2_Atom_Forge_Ref
_props_impl_forge(props_t *props, LV2_Atom_Forge *forge, props_impl_t *impl)
{
	if(impl->type == forge->Int)
		return lv2_atom_forge_int(forge, *impl->value.i);
	else if(impl->type == forge->Long)
		return lv2_atom_forge_long(forge, *impl->value.h);
	else if(impl->type == forge->Float)
		return lv2_atom_forge_float(forge, *impl->value.f);
	else if(impl->type == forge->Double)
		return lv2_atom_forge_double(forge, *impl->value.d);
	else if(impl->type == forge->Bool)
		return lv2_atom_forge_bool(forge, *impl->value.b);
	//TODO handle more types
	
	return 0;
}

static inline LV2_Atom_Forge_Ref
_props_def_forge(props_t *props, LV2_Atom_Forge *forge, props_impl_t *impl,
	const props_value_raw_t *value)
{
	if(impl->type == forge->Int)
		return lv2_atom_forge_int(forge, value->i);
	else if(impl->type == forge->Long)
		return lv2_atom_forge_long(forge, value->h);
	else if(impl->type == forge->Float)
		return lv2_atom_forge_float(forge, value->f);
	else if(impl->type == forge->Double)
		return lv2_atom_forge_double(forge, value->d);
	else if(impl->type == forge->Bool)
		return lv2_atom_forge_bool(forge, value->b);
	//TODO handle more types
	
	return 0;
}

static inline LV2_Atom_Forge_Ref
_props_reg(props_t *props, LV2_Atom_Forge *forge, uint32_t frames, props_impl_t *impl)
{
	const props_def_t *def = impl->def;
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame remove_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);
	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, props->urid.patch_patch);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_subject);
		if(ref)
			ref = lv2_atom_forge_urid(forge, props->urid.subject);

		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_remove);
		if(ref)
			ref = lv2_atom_forge_object(forge, &remove_frame, 0, 0);
		{
			if(ref)
				ref = lv2_atom_forge_key(forge, impl->access);
			if(ref)
				ref = lv2_atom_forge_urid(forge, impl->property);
		}
		if(ref)
			lv2_atom_forge_pop(forge, &remove_frame);

		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_add);
		if(ref)
			ref = lv2_atom_forge_object(forge, &add_frame, 0, 0);
		{
			if(ref)
				ref = lv2_atom_forge_key(forge, impl->access);
			if(ref)
				ref = lv2_atom_forge_urid(forge, impl->property);
		}
		if(ref)
			lv2_atom_forge_pop(forge, &add_frame);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	if(ref)
		ref = lv2_atom_forge_frame_time(forge, frames);
	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, props->urid.patch_patch);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_subject);
		if(ref)
			ref = lv2_atom_forge_urid(forge, impl->property);

		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_remove);
		if(ref)
			ref = lv2_atom_forge_object(forge, &remove_frame, 0, 0);
		{
			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.rdfs_range);
			if(ref)
				ref = lv2_atom_forge_urid(forge, props->urid.patch_wildcard);

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.rdfs_label);
			if(ref)
				ref = lv2_atom_forge_urid(forge, props->urid.patch_wildcard);

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.lv2_minimum);
			if(ref)
				ref = lv2_atom_forge_urid(forge, props->urid.patch_wildcard);

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.lv2_maximum);
			if(ref)
				ref = lv2_atom_forge_urid(forge, props->urid.patch_wildcard);

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.lv2_scale_point);
			if(ref)
				ref = lv2_atom_forge_urid(forge, props->urid.patch_wildcard);
		}
		if(ref)
			lv2_atom_forge_pop(forge, &remove_frame);

		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_add);
		if(ref)
			ref = lv2_atom_forge_object(forge, &add_frame, 0, 0);
		{
			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.rdfs_range);
			if(ref)
				ref = lv2_atom_forge_urid(forge, impl->type);

			if(def->label)
			{
				if(ref)
					ref = lv2_atom_forge_key(forge, props->urid.rdfs_label);
				if(ref)
					ref = lv2_atom_forge_string(forge, def->label, strlen(def->label));
			}

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.lv2_minimum);
			if(ref)
				ref = _props_def_forge(props, forge, impl, &def->minimum);

			if(ref)
				ref = lv2_atom_forge_key(forge, props->urid.lv2_maximum);
			if(ref)
				ref = _props_def_forge(props, forge, impl, &def->maximum);

			if(def->scale_points)
			{
				for(const props_scale_point_t *sp = def->scale_points; sp->label; sp++)
				{
					LV2_Atom_Forge_Frame scale_point_frame;

					if(ref)
						ref = lv2_atom_forge_key(forge, props->urid.lv2_scale_point);
					if(ref)
						ref = lv2_atom_forge_object(forge, &scale_point_frame, 0, 0);
					{
						if(ref)
							ref = lv2_atom_forge_key(forge, props->urid.rdfs_label);
						if(ref)
							ref = lv2_atom_forge_string(forge, sp->label, strlen(sp->label));

						if(ref)
							ref = lv2_atom_forge_key(forge, props->urid.rdf_value);
						if(ref)
							ref = _props_def_forge(props, forge, impl, &sp->value);
					}
					if(ref)
						lv2_atom_forge_pop(forge, &scale_point_frame);
				}
			}
		}
		if(ref)
			lv2_atom_forge_pop(forge, &add_frame);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_props_get(props_t *props, LV2_Atom_Forge *forge, uint32_t frames, props_impl_t *impl)
{
	LV2_Atom_Forge_Frame obj_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);

	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, props->urid.patch_set);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_subject);
		if(ref)
			ref = lv2_atom_forge_urid(forge, props->urid.subject);

		if(ref)
			ref = lv2_atom_forge_key(forge, props->urid.patch_property);
		if(ref)
			ref = lv2_atom_forge_urid(forge, impl->property);

		if(ref)
			lv2_atom_forge_key(forge, props->urid.patch_value);
		if(ref)
			ref = _props_impl_forge(props, forge, impl);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	return ref;
}

static inline void
_props_set(props_t *props, LV2_Atom_Forge *forge, props_impl_t *impl, const LV2_Atom *value)
{
	if(impl->type == value->type)
	{
		if(impl->type == forge->Int)
			*impl->value.i = ((const LV2_Atom_Int *)value)->body;
		else if(impl->type == forge->Long)
			*impl->value.h = ((const LV2_Atom_Long *)value)->body;
		else if(impl->type == forge->Float)
			*impl->value.f = ((const LV2_Atom_Float *)value)->body;
		else if(impl->type == forge->Double)
			*impl->value.d = ((const LV2_Atom_Double *)value)->body;
		else if(impl->type == forge->Bool)
			*impl->value.b = ((const LV2_Atom_Bool *)value)->body;
		//TODO handle more types
	}
}

static LV2_Atom_Forge_Ref
props_default_cb(props_t *props, LV2_Atom_Forge *forge, int64_t frames,
	props_event_t event, props_impl_t *impl, const LV2_Atom *value)
{
	switch(event)
	{
		case PROP_EVENT_GET:
		{
			return _props_get(props, forge, frames, impl);
		}
		case PROP_EVENT_SET:
		{
			_props_set(props, forge, impl, value);
			return 1;
		}
		case PROP_EVENT_REG:
		{
			return _props_reg(props, forge, frames, impl);
		}
	}

	return 0;
}

static inline int
props_advance(props_t *props, LV2_Atom_Forge *forge, uint32_t frames,
	const LV2_Atom_Object *obj, LV2_Atom_Forge_Ref *ref)
{
	if(!lv2_atom_forge_is_object_type(forge, obj->atom.type))
		return 0;

	if(obj->body.otype == props->urid.patch_get)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;

		LV2_Atom_Object_Query q [] = {
			{ props->urid.patch_subject, (const LV2_Atom **)&subject },
			{ props->urid.patch_property, (const LV2_Atom **)&property },
			LV2_ATOM_OBJECT_QUERY_END
		};
		lv2_atom_object_query(obj, q);

		if(!property)
			return 0;

		if(subject && (subject->body != props->urid.subject) )
			return 0;

		if(property->body == props->urid.patch_wildcard)
		{
			unsigned i;
			for(i = 0; i < props->nimpls; i++)
			{
				props_impl_t *impl = &props->impls[0];
				if(impl->def->mode == PROP_MODE_DYNAMIC)
				{
					if(impl->cb)
						*ref = impl->cb(props, forge, frames, PROP_EVENT_REG, impl, NULL);
					else
						*ref = _props_reg(props, forge, frames, impl);
					break;
				}
			}
			for( ; *ref && (i < props->nimpls); i++)
			{
				props_impl_t *impl = &props->impls[i];
				if(impl->def->mode == PROP_MODE_DYNAMIC)
				{
					if(impl->cb)
						*ref = impl->cb(props, forge, frames, PROP_EVENT_REG, impl, NULL);
					else
						*ref = _props_reg(props, forge, frames, impl);
				}
			}
			return 1;
		}
		else // !wildcard
		{
			props_impl_t *impl = _props_search(props, property->body);
			if(impl)
			{
				if(impl->cb)
					*ref = impl->cb(props, forge, frames, PROP_EVENT_GET, impl, NULL);
				else
					*ref = _props_get(props, forge, frames, impl);
				return 1;
			}
		}
	}
	else if(obj->body.otype == props->urid.patch_set)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;
		const LV2_Atom *value = NULL;

		LV2_Atom_Object_Query q [] = {
			{ props->urid.patch_subject, (const LV2_Atom **)&subject },
			{ props->urid.patch_property, (const LV2_Atom **)&property },
			{ props->urid.patch_value, &value },
			LV2_ATOM_OBJECT_QUERY_END
		};
		lv2_atom_object_query(obj, q);

		if(!property || !value)
			return 0;

		if(subject && (subject->body != props->urid.subject) )
			return 0;

		props_impl_t *impl = _props_search(props, property->body);
		if(impl && (impl->access == props->urid.patch_writable) )
		{
			if(impl->cb)
				impl->cb(props, forge, frames, PROP_EVENT_SET, impl, value);
			else
				_props_set(props, forge, impl, value);
			return 1;
		}
	}

	return 0; // did not handle a patch event
}

static inline int
props_register(props_t *props, const props_def_t *def, props_cb_t cb, void *value)
{
	if(props->nimpls >= props->max_nimpls)
		return -1;

	props_impl_t *impl = &props->impls[props->nimpls++];
	
	impl->property = props->map->map(props->map->handle, def->property);
	impl->access = props->map->map(props->map->handle, def->access);
	impl->type = props->map->map(props->map->handle, def->type);
	impl->def = def;
	impl->cb = cb;
	impl->value.p = value;

	//TODO register?

	return 0;
}

static inline LV2_State_Status
props_save(props_t *props, LV2_Atom_Forge *forge, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	for(unsigned i = 0; i < props->nimpls; i++)
	{
		props_impl_t *impl = &props->impls[i];

		uint32_t size = 0;
		if(impl->type == forge->Int)
			size = sizeof(int32_t);
		else if(impl->type == forge->Long)
			size = sizeof(int64_t);
		else if(impl->type == forge->Float)
			size = sizeof(float);
		else if(impl->type == forge->Double)
			size = sizeof(double);
		else if(impl->type == forge->Bool)
			size = sizeof(int32_t);
		//TODO handle more types

		if(size)
			store(state, impl->property, impl->value.p, size, impl->type, flags);
		else
			fprintf(stderr, "props_save: unknown type\n");
	}

	return LV2_STATE_SUCCESS;
}

static inline LV2_State_Status
props_restore(props_t *props, LV2_Atom_Forge *forge, LV2_State_Retrieve_Function retrieve,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	for(unsigned i = 0; i < props->nimpls; i++)
	{
		props_impl_t *impl = &props->impls[i];

		size_t size;
		uint32_t type;
		uint32_t _flags;
		const void *data = retrieve(state, impl->property, &size, &type, &_flags);
		if(data && (type == impl->type))
		{
			if(impl->type == forge->Int)
				*impl->value.i = *(const int32_t *)data;
			else if(impl->type == forge->Long)
				*impl->value.h = *(const int64_t *)data;
			else if(impl->type == forge->Float)
				*impl->value.f = *(const float *)data;
			else if(impl->type == forge->Double)
				*impl->value.d = *(const double *)data;
			else if(impl->type == forge->Bool)
				*impl->value.b = *(const int32_t *)data;
			//TODO handle more types
		}
		else
			fprintf(stderr, "props_restore: type mismatch\n");
	}

	return LV2_STATE_SUCCESS;
}

#endif // _LV2_PROPS_H_
