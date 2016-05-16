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

#ifndef LV2_OSC_WRITER_H
#define LV2_OSC_WRITER_H

#include <stdbool.h>
#include <string.h>
#include <endian.h>

#include <osc.h>
#include <util.h>

#include <lv2/lv2plug.in/ns/ext/atom/util.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LV2_OSC_Writer LV2_OSC_Writer;
typedef struct _LV2_OSC_Writer_Frame LV2_OSC_Writer_Frame;

struct _LV2_OSC_Writer {
	uint8_t *buf;
	uint8_t *ptr;
	const uint8_t *end;
};

struct _LV2_OSC_Writer_Frame {
	uint8_t *ref;
};

static inline void
osc_writer_initialize(LV2_OSC_Writer *writer, uint8_t *buf, size_t size)
{
	writer->buf = buf;
	writer->ptr = buf;
	writer->end = buf + size;
}

static inline size_t
osc_writer_get_size(LV2_OSC_Writer *writer)
{
	if(writer->ptr > writer->buf)
		return writer->ptr - writer->buf;

	return 0;
}

static inline uint8_t * 
osc_writer_finalize(LV2_OSC_Writer *writer, size_t *size)
{
	*size = osc_writer_get_size(writer);

	if(*size)
		return writer->buf;

	return NULL;
}

static inline bool
osc_writer_overflow(LV2_OSC_Writer *writer, size_t size)
{
	return writer->ptr + size >= writer->end;
}

static inline bool
osc_writer_htobe32(LV2_OSC_Writer *writer, union swap32_t *s32)
{
	if(osc_writer_overflow(writer, 4))
		return false;

	s32->u = htobe32(s32->u);
	*(uint32_t *)writer->ptr = s32->u;
	writer->ptr += 4;

	return true;
}

static inline bool 
osc_writer_htobe64(LV2_OSC_Writer *writer, union swap64_t *s64)
{
	if(osc_writer_overflow(writer, 8))
		return false;

	s64->u = htobe64(s64->u);
	*(uint64_t *)writer->ptr = s64->u;
	writer->ptr += 8;

	return true;
}

static inline bool
osc_writer_add_int32(LV2_OSC_Writer *writer, int32_t i)
{
	return osc_writer_htobe32(writer, &(union swap32_t){ .i = i });
}

static inline bool 
osc_writer_add_float(LV2_OSC_Writer *writer, float f)
{
	return osc_writer_htobe32(writer, &(union swap32_t){ .f = f });
}

static inline bool 
osc_writer_add_string(LV2_OSC_Writer *writer, const char *s)
{
	const size_t padded = LV2_OSC_PADDED_SIZE(strlen(s) + 1);
	if(osc_writer_overflow(writer, padded))
		return false;

	strncpy((char *)writer->ptr, s, padded);
	writer->ptr += padded;

	return true;
}

static inline bool
osc_writer_add_symbol(LV2_OSC_Writer *writer, const char *S)
{
	return osc_writer_add_string(writer, S);
}

static inline bool
osc_writer_add_int64(LV2_OSC_Writer *writer, int64_t h)
{
	return osc_writer_htobe64(writer, &(union swap64_t){ .h = h });
}

static inline bool 
osc_writer_add_double(LV2_OSC_Writer *writer, double d)
{
	return osc_writer_htobe64(writer, &(union swap64_t){ .d = d });
}

static inline bool
osc_writer_add_timetag(LV2_OSC_Writer *writer, uint64_t u)
{
	return osc_writer_htobe64(writer, &(union swap64_t){ .u = u });
}

static inline bool 
osc_writer_add_blob_inline(LV2_OSC_Writer *writer, int32_t len, uint8_t **body)
{
	const size_t len_padded = LV2_OSC_PADDED_SIZE(len);
	const size_t size = 4 + len_padded;
	if(osc_writer_overflow(writer, size))
		return false;

	if(!osc_writer_add_int32(writer, len))
		return false;

	*body = writer->ptr;
	memset(&writer->ptr[len], 0x0, len_padded - len);
	writer->ptr += len_padded;

	return true;
}

static inline bool 
osc_writer_add_blob(LV2_OSC_Writer *writer, int32_t len, const uint8_t *body)
{
	uint8_t *dst;
	if(!osc_writer_add_blob_inline(writer, len, &dst))
		return false;

	memcpy(dst, body, len);

	return true;
}

static inline bool
osc_writer_add_midi_inline(LV2_OSC_Writer *writer, int32_t len, uint8_t **m)
{
	if( (len > 4) || osc_writer_overflow(writer, 4))
		return false;

	*m = writer->ptr;
	memset(&writer->ptr[len], 0x0, 4 - len);
	writer->ptr += 4;

	return true;
}

static inline bool 
osc_writer_add_midi(LV2_OSC_Writer *writer, int32_t len, const uint8_t *m)
{
	uint8_t *dst;
	if(!osc_writer_add_midi_inline(writer, len, &dst))
		return false;

	memcpy(dst, m, len);

	return true;
}

static inline bool
osc_writer_add_rgba(LV2_OSC_Writer *writer, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if(osc_writer_overflow(writer, 4))
		return false;

	writer->ptr[0] = r;
	writer->ptr[1] = g;
	writer->ptr[2] = b;
	writer->ptr[3] = a;
	writer->ptr += 4;

	return true;
}

static inline bool
osc_writer_add_char(LV2_OSC_Writer *writer, char c)
{
	return osc_writer_add_int32(writer, (int32_t)c);
}

static inline bool 
osc_writer_push_bundle(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame, uint64_t t)
{
	if(osc_writer_overflow(writer, 16))
		return false;

	frame->ref = writer->ptr;

	strncpy((char *)writer->ptr, "#bundle", 8);
	writer->ptr += 8;

	return osc_writer_add_timetag(writer, t);
}

static inline bool 
osc_writer_pop_bundle(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	union swap32_t s32 = { .i = writer->ptr - frame->ref - 16};

	if(s32.i <= 0)
	{
		writer->ptr = frame->ref;
		return false;
	}

	return true;
}

static inline  bool
osc_writer_push_item(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	if(osc_writer_overflow(writer, 4))
		return false;

	frame->ref = writer->ptr;
	writer->ptr += 4;

	return true;
}

static inline bool
osc_writer_pop_item(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	union swap32_t s32 = { .i = writer->ptr - frame->ref - 4};

	if(s32.i <= 0)
	{
		writer->ptr = frame->ref;
		return false;
	}

	s32.u = htobe32(s32.u);
	*(uint32_t *)frame->ref = s32.u;

	return true;
}

static inline bool 
osc_writer_add_path(LV2_OSC_Writer *writer, const char *path)
{
	return osc_writer_add_string(writer, path);
}

static inline bool 
osc_writer_add_format(LV2_OSC_Writer *writer, const char *fmt)
{
	const size_t padded = LV2_OSC_PADDED_SIZE(strlen(fmt) + 2);
	if(osc_writer_overflow(writer, padded))
		return false;

	*writer->ptr++ = ',';
	strncpy((char *)writer->ptr, fmt, padded - 1);
	writer->ptr += padded - 1;

	return true;
}

static inline bool
osc_writer_arg_varlist(LV2_OSC_Writer *writer, const char *fmt, va_list args)
{
	for(const char *type = fmt; *type; type++)
	{
		switch( (LV2_OSC_Type)*type)
		{
			case LV2_OSC_INT32:
				if(!osc_writer_add_int32(writer, va_arg(args, int32_t)))
					return false;
				break;
			case LV2_OSC_FLOAT:
				if(!osc_writer_add_float(writer, (float)va_arg(args, double)))
					return false;
				break;
			case LV2_OSC_STRING:
				if(!osc_writer_add_string(writer, va_arg(args, const char *)))
					return false;
				break;
			case LV2_OSC_BLOB:
				if(!osc_writer_add_blob(writer, va_arg(args, int32_t), va_arg(args, const uint8_t *)))
					return false;
				break;

			case LV2_OSC_TRUE:
			case LV2_OSC_FALSE:
			case LV2_OSC_NIL:
			case LV2_OSC_IMPULSE:
				break;

			case LV2_OSC_INT64:
				if(!osc_writer_add_int64(writer, va_arg(args, int64_t)))
					return false;
				break;
			case LV2_OSC_DOUBLE:
				if(!osc_writer_add_double(writer, va_arg(args, double)))
					return false;
				break;
			case LV2_OSC_TIMETAG:
				if(!osc_writer_add_timetag(writer, va_arg(args, uint64_t)))
					return false;
				break;

			case LV2_OSC_MIDI:
				if(!osc_writer_add_midi(writer, va_arg(args, int32_t), va_arg(args, const uint8_t *)))
					return false;
				break;
			case LV2_OSC_SYMBOL:
				if(!osc_writer_add_symbol(writer, va_arg(args, const char *)))
					return false;
				break;
			case LV2_OSC_CHAR:
				if(!osc_writer_add_char(writer, va_arg(args, int)))
					return false;
				break;
			case LV2_OSC_RGBA:
				if(!osc_writer_add_rgba(writer, va_arg(args, unsigned), va_arg(args, unsigned),
						va_arg(args, unsigned), va_arg(args, unsigned)))
					return false;
				break;
		}
	}

	return true;
}

static inline bool
osc_writer_arg_vararg(LV2_OSC_Writer *writer, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	const bool res = osc_writer_arg_varlist(writer, fmt, args);

	va_end(args);

	return res;
}

static inline bool
osc_writer_message_varlist(LV2_OSC_Writer *writer, const char *path, const char *fmt, va_list args)
{
	if(!osc_writer_add_path(writer, path))
		return false;

	if(!osc_writer_add_format(writer, fmt))
		return false;

	return osc_writer_arg_varlist(writer, fmt, args);
}

static inline bool
osc_writer_message_vararg(LV2_OSC_Writer *writer, const char *path, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	const bool res = osc_writer_message_varlist(writer, path, fmt, args);

	va_end(args);

	return res;
}

static inline bool
osc_writer_packet(LV2_OSC_Writer *writer, LV2_OSC_URID *osc_urid,
	LV2_URID_Unmap *unmap, uint32_t size, const LV2_Atom_Object_Body *body)
{
	if(body->otype == osc_urid->OSC_Bundle)
	{
		const LV2_Atom_Object *timetag = NULL;
		const LV2_Atom_Tuple *items = NULL;

		if(!lv2_osc_bundle_body_get(osc_urid, size, body, &timetag, &items))
			return false;

		LV2_OSC_Timetag tt;
		LV2_OSC_Writer_Frame bndl;

		lv2_osc_timetag_get(osc_urid, timetag, &tt);
		if(!osc_writer_push_bundle(writer, &bndl, lv2_osc_timetag_parse(&tt)))
			return false;

		LV2_ATOM_OBJECT_BODY_FOREACH(body, size, prop)
		{
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&prop->value;
			LV2_OSC_Writer_Frame itm;

			if(  !osc_writer_push_item(writer, &itm)
				|| !osc_writer_packet(writer, osc_urid, unmap, obj->atom.size, &obj->body)
				|| !osc_writer_pop_item(writer, &itm) )
			{
				return false;
			}
		}

		return osc_writer_pop_bundle(writer, &bndl);
	}
	else if(body->otype == osc_urid->OSC_Message)
	{
		const LV2_Atom_String *path = NULL;
		const LV2_Atom_Tuple *arguments = NULL;

		if(lv2_osc_message_body_get(osc_urid, size, body, &path, &arguments))
		{
			if(!osc_writer_add_path(writer, LV2_ATOM_BODY_CONST(path)))
				return false;

			LV2_ATOM_OBJECT_BODY_FOREACH(body, size, prop)
			{
				//FIXME write format string
			}

			LV2_ATOM_OBJECT_BODY_FOREACH(body, size, prop)
			{
				const LV2_Atom *atom = &prop->value;
				const LV2_Atom_Object *obj= (const LV2_Atom_Object *)&prop->value;

				if(atom->type == osc_urid->ATOM_Int)
				{
					if(!osc_writer_add_int32(writer, ((const LV2_Atom_Int *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Float)
				{
					if(!osc_writer_add_float(writer, ((const LV2_Atom_Float *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_String)
				{
					if(!osc_writer_add_string(writer, LV2_ATOM_BODY_CONST(atom)))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Chunk)
				{
					if(!osc_writer_add_blob(writer, atom->size, LV2_ATOM_BODY_CONST(atom)))
						return false;
				}

				else if(atom->type == osc_urid->ATOM_Long)
				{
					if(!osc_writer_add_int64(writer, ((const LV2_Atom_Long *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Double)
				{
					if(!osc_writer_add_double(writer, ((const LV2_Atom_Double *)atom)->body))
						return false;
				}
				else if( (atom->type == osc_urid->ATOM_Object) && (obj->body.otype == osc_urid->OSC_Timetag) )
				{
					LV2_OSC_Timetag tt;
					lv2_osc_timetag_get(osc_urid, obj, &tt);
					if(!osc_writer_add_timetag(writer, lv2_osc_timetag_parse(&tt)))
						return false;
				}

				else if(atom->type == osc_urid->ATOM_URID)
				{
					const char *symbol = unmap->unmap(unmap->handle, ((const LV2_Atom_URID *)atom)->body);
					if(!symbol || !osc_writer_add_symbol(writer, symbol))
						return false;
				}
				else if(atom->type == osc_urid->MIDI_MidiEvent)
				{
					if(!osc_writer_add_midi(writer, atom->size, LV2_ATOM_BODY_CONST(atom)))
						return false;
				}
				//FIXME CHAR, RGBA
			}
		}

		return true;
	}

	return false;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_WRITER_H
