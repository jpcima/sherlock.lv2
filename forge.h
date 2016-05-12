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

#ifndef LV2_OSC_FORGE_H
#define LV2_OSC_FORGE_H

#include <writer.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline LV2_Atom_Forge_Ref
osc_forge_initialize(LV2_Atom_Forge *forge, LV2_OSC_Writer *writer,
	LV2_Atom_Forge_Frame *frame, LV2_URID osc_event)
{
	LV2_Atom_Forge_Ref ref = lv2_atom_forge_atom(forge, 0, osc_event);
	if(ref)
	{
		frame->ref = ref;
		LV2_Atom *atom = lv2_atom_forge_deref(forge, frame->ref);

		osc_writer_initialize(writer, LV2_ATOM_BODY(atom), forge->size - forge->offset);
	}

	return ref;
}

static inline LV2_Atom_Forge_Ref
osc_forge_finalize(LV2_Atom_Forge *forge, LV2_OSC_Writer *writer,
	LV2_Atom_Forge_Frame *frame)
{
	size_t size;
	uint8_t *osc = osc_writer_finalize(writer, &size);

	if(osc)
	{
		LV2_Atom *atom = lv2_atom_forge_deref(forge, frame->ref);
		atom->size = size;

		return lv2_atom_forge_write(forge, osc, size);
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
osc_forge_message_vararg(LV2_Atom_Forge *forge, LV2_URID osc_event,
	const char *path, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	LV2_OSC_Writer writer;
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref = osc_forge_initialize(forge, &writer, &frame, osc_event);

	if(ref && osc_writer_message_varlist(&writer, path, fmt, args))
		ref = osc_forge_finalize(forge, &writer, &frame);

	va_end(args);

	return ref;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_FORGE_H
