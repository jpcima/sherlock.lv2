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

#include <time.h>
#include <math.h>
#include <inttypes.h>

#include <sherlock.h>
#include <sherlock_nk.h>

#include <osc.lv2/util.h>

typedef struct _mem_t mem_t;

struct _mem_t {
	size_t size;
	char *buf;
};

static inline void
_mem_printf(mem_t *mem, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	char *src;
	if(vasprintf(&src, fmt, args) != -1)
	{
		const size_t sz = strlen(src);

		mem->buf = realloc(mem->buf, mem->size + sz);
		if(mem->buf)
		{
			char *dst = mem->buf + mem->size - 1;

			strncpy(dst, src, sz + 1);
			mem->size += sz;
		}
		else
			mem->size = 0;

		free(src);
	}

	va_end(args);
}

static void
_osc_timetag(mem_t *mem, LV2_OSC_Timetag *tt)
{
	if(tt->integral <= 1UL)
	{
		_mem_printf(mem, "t:%s", "immediate");
	}
	else
	{
		const uint32_t us = floor(tt->fraction * 0x1p-32 * 1e6);
		const time_t ttime = tt->integral - 0x83aa7e80;
		const struct tm *ltime = localtime(&ttime);
		
		char tmp [32];
		if(strftime(tmp, 32, "%d-%b-%Y %T", ltime))
			_mem_printf(mem, "t:%s.%06"PRIu32, tmp, us);
	}
}

static void
_osc_message(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj, float offset)
{
	const LV2_Atom_String *path = NULL;
	const LV2_Atom_Tuple *args = NULL;
	lv2_osc_message_get(&handle->osc_urid, obj, &path, &args);

	mem_t mem = {
		.size = 1,
		.buf = calloc(1, sizeof(char))
	};

	LV2_ATOM_TUPLE_FOREACH(args, arg)
	{
		const LV2_OSC_Type type = lv2_osc_argument_type(&handle->osc_urid, arg);

		switch(type)
		{
			case LV2_OSC_INT32:
			{
				int32_t i;
				lv2_osc_int32_get(&handle->osc_urid, arg, &i);
				_mem_printf(&mem, "i:%"PRIi32, i);
			} break;
			case LV2_OSC_FLOAT:
			{
				float f;
				lv2_osc_float_get(&handle->osc_urid, arg, &f);
				_mem_printf(&mem, "f:%f", f);
			} break;
			case LV2_OSC_STRING:
			{
				const char *s;
				lv2_osc_string_get(&handle->osc_urid, arg, &s);
				_mem_printf(&mem, "s:%s", s);
			} break;
			case LV2_OSC_BLOB:
			{
				uint32_t sz;
				const uint8_t *b;
				lv2_osc_blob_get(&handle->osc_urid, arg, &sz, &b);
				_mem_printf(&mem, "b(%"PRIu32"):", sz);
				for(unsigned i=0; i<sz; i++)
					_mem_printf(&mem, "%02"PRIx8, b[i]);
			} break;

			case LV2_OSC_TRUE:
			{
				_mem_printf(&mem, "T:rue");
			} break;
			case LV2_OSC_FALSE:
			{
				_mem_printf(&mem, "F:alse");
			} break;
			case LV2_OSC_NIL:
			{
				_mem_printf(&mem, "N:il");
			} break;
			case LV2_OSC_IMPULSE:
			{
				_mem_printf(&mem, "I:impulse");
			} break;

			case LV2_OSC_INT64:
			{
				int64_t h;
				lv2_osc_int64_get(&handle->osc_urid, arg, &h);
				_mem_printf(&mem, "h:%"PRIi64, h);
			} break;
			case LV2_OSC_DOUBLE:
			{
				double d;
				lv2_osc_double_get(&handle->osc_urid, arg, &d);
				_mem_printf(&mem, "d:%lf", d);
			} break;
			case LV2_OSC_TIMETAG:
			{
				LV2_OSC_Timetag tt;
				lv2_osc_timetag_get(&handle->osc_urid, arg, &tt);
				_osc_timetag(&mem, &tt);
			} break;

			case LV2_OSC_SYMBOL:
			{
				const char *S;
				lv2_osc_symbol_get(&handle->osc_urid, arg, &S);
				_mem_printf(&mem, "S:%s", S);
			} break;
			case LV2_OSC_CHAR:
			{
				char c;
				lv2_osc_char_get(&handle->osc_urid, arg, &c);
				_mem_printf(&mem, "c:%c", c);
			} break;
			case LV2_OSC_RGBA:
			{
				uint8_t r [4];
				lv2_osc_rgba_get(&handle->osc_urid, arg, r+0, r+1, r+2, r+3);
				_mem_printf(&mem, "r:");
				for(unsigned i=0; i<4; i++)
					_mem_printf(&mem, "%02"PRIx8, r[i]);
			} break;
			case LV2_OSC_MIDI:
			{
				uint32_t sz;
				const uint8_t *m;
				lv2_osc_midi_get(&handle->osc_urid, arg, &sz, &m);
				_mem_printf(&mem, "m(%"PRIu32"):", sz);
				for(unsigned i=0; i<sz; i++)
					_mem_printf(&mem, "%02"PRIx8, m[i]);
			} break;
		}

		_mem_printf(&mem, " ");
	}

	nk_layout_row_push(ctx, 0.4 - offset);
	nk_label_colored(ctx, LV2_ATOM_BODY_CONST(path), NK_TEXT_LEFT, magenta);

	nk_layout_row_push(ctx, 0.5);
	if(mem.buf)
	{
		nk_label_colored(ctx, mem.buf, NK_TEXT_LEFT, white);
		free(mem.buf);
	}
	else
		_empty(ctx);

	nk_layout_row_push(ctx, 0.1);
	nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, obj->atom.size);

	nk_layout_row_end(ctx);
}

static void
_osc_packet(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj, float offset);

static void
_osc_bundle(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj, float offset)
{
	const float widget_h = handle->dy;

	const LV2_Atom_Object *timetag = NULL;
	const LV2_Atom_Tuple *items = NULL;
	lv2_osc_bundle_get(&handle->osc_urid, obj, &timetag, &items);

	// format bundle timestamp
	LV2_OSC_Timetag tt;
	lv2_osc_timetag_get(&handle->osc_urid, &timetag->atom, &tt);

	mem_t mem = {
		.size = 1,
		.buf = calloc(1, sizeof(char))
	};

	nk_layout_row_push(ctx, 0.4 - offset);
	nk_label_colored(ctx, "#bundle", NK_TEXT_LEFT, red);

	nk_layout_row_push(ctx, 0.5);
	_osc_timetag(&mem, &tt);
	if(mem.buf)
	{
		nk_label_colored(ctx, mem.buf, NK_TEXT_LEFT, white);
		free(mem.buf);
	}
	else
		_empty(ctx);

	nk_layout_row_push(ctx, 0.1);
	nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, obj->atom.size);

	nk_layout_row_end(ctx);

	offset += 0.025;
	LV2_ATOM_TUPLE_FOREACH(items, item)
	{
		nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 4);

		nk_layout_row_push(ctx, offset);
		_empty(ctx);

		_osc_packet(handle, ctx, (const LV2_Atom_Object *)item, offset);
	}
}

static void
_osc_packet(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj, float offset)
{
	if(lv2_osc_is_message_type(&handle->osc_urid, obj->body.otype))
	{
		_osc_message(handle, ctx, obj, offset);
	}
	else if(lv2_osc_is_bundle_type(&handle->osc_urid, obj->body.otype))
	{
		_osc_bundle(handle, ctx, obj, offset);
	}
}

void
_osc_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	const float widget_h = handle->dy;

	if(nk_begin(ctx, "Window", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		nk_layout_row_dynamic(ctx, widget_h, 3);
		{
			if(nk_checkbox_label(ctx, "overwrite", &handle->state.overwrite))
				_toggle(handle, handle->urid.overwrite, handle->state.overwrite, true);
			if(nk_checkbox_label(ctx, "block", &handle->state.block))
				_toggle(handle, handle->urid.block, handle->state.block, true);
			if(nk_checkbox_label(ctx, "follow", &handle->state.follow))
				_toggle(handle, handle->urid.follow, handle->state.follow, true);
		}

		nk_layout_row_dynamic(ctx, widget_h, 2);
		{
			if(nk_button_label(ctx, "clear"))
				_clear(handle);

			int selected = 0;
			for(int i = 0; i < 5; i++)
			{
				if(handle->state.count == max_values[i])
				{
					selected = i;
					break;
				}
			}

			selected = nk_combo(ctx, max_items, 5, selected, widget_h,
				nk_vec2(wbounds.w/3, widget_h*5));
			if(handle->state.count != max_values[selected])
			{
				handle->state.count = max_values[selected];
				_toggle(handle, handle->urid.count, handle->state.count, false);
			}
		}

		nk_layout_row_dynamic(ctx, wbounds.h - 2*widget_h, 1);
		if(nk_group_begin(ctx, "Events", 0))
		{
			uint32_t counter = 0;

			LV2_ATOM_TUPLE_FOREACH((const LV2_Atom_Tuple *)handle->ser.atom, atom)
			{
				const LV2_Atom_Tuple *tup = (const LV2_Atom_Tuple *)atom;
				const LV2_Atom_Long *offset = (const LV2_Atom_Long *)lv2_atom_tuple_begin(tup);
				const LV2_Atom_Int *nsamples = (const LV2_Atom_Int *)lv2_atom_tuple_next(&offset->atom);
				const LV2_Atom_Sequence *seq = (const LV2_Atom_Sequence *)lv2_atom_tuple_next(&nsamples->atom);

				nk_layout_row_dynamic(ctx, 2.f, 1);
				_ruler(ctx, 2.f, gray);

				nk_layout_row_dynamic(ctx, widget_h, 3);
				nk_labelf_colored(ctx, NK_TEXT_LEFT, orange, "@%"PRIi64, offset->body);
				nk_labelf_colored(ctx, NK_TEXT_CENTERED, green, "-%"PRIu32"-", counter);
				nk_labelf_colored(ctx, NK_TEXT_RIGHT, violet, "%"PRIi32, nsamples->body);

				nk_layout_row_dynamic(ctx, 2.f, 1);
				_ruler(ctx, 1.f, gray);

				LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
				{
					const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
					const int64_t frames = ev->time.frames;
					const float off = 0.1;

					nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 4);

					nk_layout_row_push(ctx, off);
					nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);

					_osc_packet(handle, ctx, obj, off);

					counter += 1;
				}
			}

			handle->count = counter;

			const struct nk_panel *panel = nk_window_get_panel(ctx);
			if(handle->bottom)
			{
				panel->offset->y = panel->at_y;
				handle->bottom = false;

				_post_redisplay(handle);
			}

			nk_group_end(ctx);
		}
	}
	nk_end(ctx);
}
