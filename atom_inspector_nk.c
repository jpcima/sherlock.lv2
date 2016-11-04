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

#include <ctype.h>

#include <sherlock.h>
#include <sherlock_nk.h>

static const uint8_t ful = 0xff;
static const uint8_t one = 0xbb;
static const uint8_t two = 0x66;
static const uint8_t non = 0x0;
static const struct nk_color white		= {.r = one, .g = one, .b = one, .a = ful};
static const struct nk_color gray			= {.r = two, .g = two, .b = two, .a = ful};
static const struct nk_color yellow		= {.r = one, .g = one, .b = non, .a = ful};
static const struct nk_color magenta	= {.r = one, .g = two, .b = one, .a = ful};
static const struct nk_color green		= {.r = non, .g = one, .b = non, .a = ful};
static const struct nk_color blue			= {.r = non, .g = one, .b = one, .a = ful};
static const struct nk_color orange		= {.r = one, .g = two, .b = non, .a = ful};
static const struct nk_color violet		= {.r = two, .g = two, .b = one, .a = ful};

#define NS_RDF (const uint8_t*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS (const uint8_t*)"http://www.w3.org/2000/01/rdf-schema#"
#define NS_XSD (const uint8_t*)"http://www.w3.org/2001/XMLSchema#"
#define NS_OSC (const uint8_t*)"http://open-music-kontrollers.ch/lv2/osc#"
#define NS_XPRESS (const uint8_t*)"http://open-music-kontrollers.ch/lv2/xpress#"
#define NS_SPOD (const uint8_t*)"http://open-music-kontrollers.ch/lv2/synthpod#"
#define NS_CANVAS (const uint8_t*)"http://open-music-kontrollers.ch/lv2/canvas#"

// copyied and adapted from libsratom 
static inline char *
_sratom_to_turtle(Sratom*         sratom,
                 LV2_URID_Unmap* unmap,
                 const char*     base_uri,
                 const SerdNode* subject,
                 const SerdNode* predicate,
                 uint32_t        type,
                 uint32_t        size,
                 const void*     body)
{
	SerdURI   buri = SERD_URI_NULL;
	SerdNode  base = serd_node_new_uri_from_string((uint8_t *)(base_uri), NULL, &buri);
	SerdEnv*  env  = serd_env_new(&base);
	SerdChunk str  = { NULL, 0 };

	serd_env_set_prefix_from_strings(env, (const uint8_t *)"rdf", NS_RDF);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"rdfs", NS_RDFS);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xsd", NS_XSD);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"lv2", (const uint8_t *)LV2_CORE_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"midi", (const uint8_t *)LV2_MIDI_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"atom", (const uint8_t *)LV2_ATOM_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"units", (const uint8_t *)LV2_UNITS_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"ui", (const uint8_t *)LV2_UI_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"time", (const uint8_t *)LV2_TIME_URI"#");
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"patch", (const uint8_t *)LV2_PATCH_PREFIX);

	serd_env_set_prefix_from_strings(env, (const uint8_t *)"osc", NS_OSC);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xpress", NS_XPRESS);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"spod", NS_SPOD);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"canvas", NS_CANVAS);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_ABBREVIATED |
		            SERD_STYLE_RESOLVED |
		            SERD_STYLE_CURIED |
								SERD_STYLE_ASCII),
		env, &buri, serd_chunk_sink, &str);

	// Write @prefix directives
	serd_env_foreach(env,
	                 (SerdPrefixSink)serd_writer_set_prefix,
	                 writer);

	sratom_set_sink(sratom, base_uri,
	                (SerdStatementSink)serd_writer_write_statement,
	                (SerdEndSink)serd_writer_end_anon,
	                writer);
	sratom_write(sratom, unmap, SERD_EMPTY_S,
	             subject, predicate, type, size, body);
	serd_writer_finish(writer);

	serd_writer_free(writer);
	serd_env_free(env);
	serd_node_free(&base);
	return (char*)serd_chunk_sink_finish(&str);
}

void
_atom_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	const float widget_h = 20;
	bool ttl_dirty = false;

	if(nk_begin(ctx, "Window", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		nk_layout_row_dynamic(ctx, wbounds.h, 2);
		if(nk_group_begin(ctx, "Left", NK_WINDOW_NO_SCROLLBAR))
		{
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
				const LV2_Atom *selected = NULL;

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

					const LV2_Atom *last = NULL;
					LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
					{
						const LV2_Atom *body = &ev->body;
						const int64_t frames = ev->time.frames;
						const char *uri = handle->unmap->unmap(handle->unmap->handle, body->type);

						nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 3);
						{
							nk_layout_row_push(ctx, 0.1);
							nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);

							nk_layout_row_push(ctx, 0.8);
							if(nk_select_label(ctx, uri, NK_TEXT_LEFT, handle->selected == body))
							{
								ttl_dirty = handle->selected != body; // has selection actually changed?
								handle->selected = body;
							}

							nk_layout_row_push(ctx, 0.1);
							nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, body->size);
						}
						nk_layout_row_end(ctx);

						last = body;
					}

					if(handle->bottom)
					{
						ttl_dirty = true;
						handle->selected = last;
					}

					counter += 1;
				}

				const struct nk_panel *panel = nk_window_get_panel(ctx);
				if(handle->bottom)
				{
					panel->offset->y = panel->at_y;
					handle->bottom = false;

					_post_redisplay(handle);
				}

				nk_group_end(ctx);
			}

			nk_group_end(ctx);
		}

		if(nk_group_begin(ctx, "Right", NK_WINDOW_NO_SCROLLBAR))
		{
			const LV2_Atom *atom = handle->selected;

			if(ttl_dirty && atom)
			{
				char *ttl = _sratom_to_turtle(handle->sratom, handle->unmap,
					handle->base_uri, NULL, NULL,
					atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
				if(ttl)
				{
					struct nk_str *str = &handle->edit.string;
					const size_t len = strlen(ttl);

					nk_str_clear(str);

					char *from, *to;
					for(from=ttl, to=strchr(from, '\t');
						to;
						from=to+1, to=strchr(from, '\t'))
					{
						nk_str_append_text_utf8(str, from, to-from);
						nk_str_append_text_utf8(str, "  ", 2);
					}
					nk_str_append_text_utf8(str, from, strlen(from));

					free(ttl);
				}
			}

			nk_layout_row_dynamic(ctx, wbounds.h, 1);
			const nk_flags flags = NK_EDIT_EDITOR | NK_EDIT_READ_ONLY;
			const nk_flags mode = nk_edit_buffer(ctx, flags, &handle->edit, nk_filter_default);
			(void)mode;

			nk_group_end(ctx);
		}
	}
	nk_end(ctx);
}
