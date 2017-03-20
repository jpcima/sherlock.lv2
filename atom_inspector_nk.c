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
#include <inttypes.h>

#include <sherlock.h>
#include <sherlock_nk.h>
#include <encoder.h>

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

static void
_set_string(struct nk_str *str, uint32_t size, const char *body)
{
	nk_str_clear(str);

	// replace tab with 2 spaces
	const char *end = body + size - 1;
	const char *from = body;
	for(const char *to = strchr(from, '\t');
		to && (to < end);
		from = to + 1, to = strchr(from, '\t'))
	{
		nk_str_append_text_utf8(str, from, to-from);
		nk_str_append_text_utf8(str, "  ", 2);
	}
	nk_str_append_text_utf8(str, from, end-from);
}

void
_atom_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	handle->dy = 20.f * _get_scale(handle);
	const float widget_h = handle->dy;
	struct nk_style *style = &ctx->style;
  const struct nk_vec2 window_padding = style->window.padding;
  const struct nk_vec2 group_padding = style->window.group_padding;

	style->selectable.normal.data.color.a = 0x0;
	style->selectable.hover.data.color.a = 0x0;

	if(nk_begin(ctx, "Window", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);
		struct nk_panel *panel= nk_window_get_panel(ctx);
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		const float body_h = panel->bounds.h - 2*window_padding.y;
		nk_layout_row_dynamic(ctx, body_h, 2);
		if(nk_group_begin(ctx, "Left", NK_WINDOW_NO_SCROLLBAR))
		{
			const float content_h = nk_window_get_height(ctx) - 2*window_padding.y - 4*group_padding.y - 2*widget_h;
			nk_layout_row_dynamic(ctx, content_h, 1);
			nk_flags flags = NK_WINDOW_BORDER;
			if(handle->state.follow)
				flags |= NK_WINDOW_NO_SCROLLBAR;
			struct nk_list_view lview;
			if(nk_list_view_begin(ctx, &lview, "Events", flags, widget_h, NK_MIN(handle->n_item, MAX_LINES)))
			{
				if(handle->state.follow)
				{
					lview.end = NK_MAX(handle->n_item, 0);
					lview.begin = NK_MAX(lview.end - lview.count, 0);
				}
				for(int l = lview.begin; (l < lview.end) && (l < handle->n_item); l++)
				{
					item_t *itm = handle->items[l];

					switch(itm->type)
					{
						case ITEM_TYPE_NONE:
						{
							// skip, never reached
						} break;
						case ITEM_TYPE_FRAME:
						{
							nk_layout_row_dynamic(ctx, widget_h, 3);
							{
								struct nk_rect b = nk_widget_bounds(ctx);
								b.x -= group_padding.x;
								b.w *= 3;
								b.w += 4*group_padding.x;
								nk_fill_rect(canvas, b, 0.f, nk_rgb(0x18, 0x18, 0x18));
							}

							nk_labelf_colored(ctx, NK_TEXT_LEFT, orange, "@%"PRIi64, itm->frame.offset);
							nk_labelf_colored(ctx, NK_TEXT_CENTERED, green, "-%"PRIu32"-", itm->frame.counter);
							nk_labelf_colored(ctx, NK_TEXT_RIGHT, violet, "%"PRIi32, itm->frame.nsamples);
						} break;

						case ITEM_TYPE_EVENT:
						{
							LV2_Atom_Event *ev = &itm->event.ev;
							const LV2_Atom *body = &ev->body;
							const int64_t frames = ev->time.frames;
							const char *uri = NULL;
							if(lv2_atom_forge_is_object_type(&handle->forge, body->type))
							{
								const LV2_Atom_Object *obj = (const LV2_Atom_Object *)body;

								if(obj->body.otype)
									uri = handle->unmap->unmap(handle->unmap->handle, obj->body.otype);
								else if(obj->body.id)
									uri = handle->unmap->unmap(handle->unmap->handle, obj->body.id);
								else
									uri = "Unknown";
							}
							else // not an object
							{
								uri = handle->unmap->unmap(handle->unmap->handle, body->type);
							}

							nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 3);
							{
								nk_layout_row_push(ctx, 0.1);
								if(l % 2 == 0)
								{
									struct nk_rect b = nk_widget_bounds(ctx);
									b.x -= group_padding.x;
									b.w *= 10;
									b.w += 8*group_padding.x;
									nk_fill_rect(canvas, b, 0.f, nk_rgb(0x28, 0x28, 0x28));
								}
								nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);

								nk_layout_row_push(ctx, 0.8);
								if(nk_select_label(ctx, uri, NK_TEXT_LEFT, handle->selected == body))
								{
									handle->ttl_dirty = handle->ttl_dirty
										|| (handle->selected != body); // has selection actually changed?
									handle->selected = body;
								}

								nk_layout_row_push(ctx, 0.1);
								nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, body->size);
							}
							nk_layout_row_end(ctx);
						} break;
					}
				}

				nk_list_view_end(&lview);
			}

			nk_layout_row_dynamic(ctx, widget_h, 4);
			{
				if(nk_checkbox_label(ctx, "overwrite", &handle->state.overwrite))
					_toggle(handle, handle->urid.overwrite, handle->state.overwrite, true);
				if(nk_checkbox_label(ctx, "block", &handle->state.block))
					_toggle(handle, handle->urid.block, handle->state.block, true);
				if(nk_checkbox_label(ctx, "follow", &handle->state.follow))
					_toggle(handle, handle->urid.follow, handle->state.follow, true);
				if(nk_checkbox_label(ctx, "pretty", &handle->pretty_numbers))
				{
					handle->ttl_dirty = true;
					sratom_set_pretty_numbers(handle->sratom, handle->pretty_numbers);
				}
			}

			const bool max_reached = handle->n_item >= MAX_LINES;
			nk_layout_row_dynamic(ctx, widget_h, 2);
			if(nk_button_symbol_label(ctx,
				max_reached ? NK_SYMBOL_TRIANGLE_RIGHT: NK_SYMBOL_NONE,
				"clear", NK_TEXT_LEFT))
			{
				_clear(handle);
			}
			nk_label(ctx, "Sherlock.lv2: "SHERLOCK_VERSION, NK_TEXT_RIGHT);

			nk_group_end(ctx);
		}

		if(nk_group_begin(ctx, "Right", NK_WINDOW_NO_SCROLLBAR))
		{
			const LV2_Atom *atom = handle->selected;
			if(handle->ttl_dirty && atom)
			{
				char *ttl = _sratom_to_turtle(handle->sratom, handle->unmap,
					handle->base_uri, NULL, NULL,
					atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
				if(ttl)
				{
					struct nk_str *str = &handle->editor.string;
					const size_t len = strlen(ttl);

					_set_string(str, len, ttl);

					handle->editor.lexer.needs_refresh = 1;

					free(ttl);
				}

				handle->ttl_dirty = false;
			}

			const nk_flags flags = NK_EDIT_EDITOR;
			char *str = nk_str_get(&handle->editor.string);
			int len = nk_str_len(&handle->editor.string);

			if(len > 0) //FIXME
			{
				const float content_h = nk_window_get_height(ctx) - 2*window_padding.y - 2*group_padding.y;
				nk_layout_row_dynamic(ctx, content_h, 1);
				nk_edit_focus(ctx, flags);
				const nk_flags mode = nk_edit_buffer(ctx, flags, &handle->editor, nk_filter_default);
				(void)mode;
			}

			nk_group_end(ctx);
		}
	}
	nk_end(ctx);
}
