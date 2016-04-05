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

#include <inttypes.h>
#include <stdio.h>

#include <sherlock.h>
#include <common.h>

#include <Elementary.h>

#include <sratom/sratom.h>

#define COUNT_MAX 2048 // maximal amount of events shown
#define STRING_BUF_SIZE 2048
#define STRING_MAX 256
#define STRING_OFF (STRING_MAX - 4)

#define NS_RDF (const uint8_t*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD (const uint8_t*)"http://www.w3.org/2001/XMLSchema#"
#define NS_OSC (const uint8_t*)"http://open-music-kontrollers.ch/lv2/osc#"
#define NS_XPRESS (const uint8_t*)"http://open-music-kontrollers.ch/lv2/xpress#"
#define NS_SPOD (const uint8_t*)"http://open-music-kontrollers.ch/lv2/synthpod#"

typedef struct _UI UI;

struct _UI {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	struct {
		LV2_URID event_transfer;
	} uris;
	
	LV2_Atom_Forge forge;

	Evas_Object *widget;
	Evas_Object *table;
	Evas_Object *list;
	Evas_Object *info;
	Evas_Object *clear;
	Evas_Object *autoclear;
	Evas_Object *autoblock;
	Evas_Object *popup;

	Elm_Genlist_Item_Class *itc_list;
	Elm_Genlist_Item_Class *itc_group;

	char string_buf [STRING_BUF_SIZE];
	char *logo_path;

	Eina_Hash *urids;

	Sratom *sratom;
	const char *base_uri;

	char *chunk;
};

#define CODE_PRE "<style=shadow,bottom>"
#define CODE_POST "</style>"

#define URI(VAL,TYP) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#fff font=Default>"TYP"</color>")

static void
_encoder_begin(void *data)
{
	UI *ui = data;

	ui->chunk = strdup("");
}

static void
_encoder_append(const char *str, void *data)
{
	UI *ui = data;

	size_t size = 0;
	size += ui->chunk ? strlen(ui->chunk) : 0;
	size += str ? strlen(str) + 1 : 0;

	if(size)
		ui->chunk = realloc(ui->chunk, size);

	if(ui->chunk && str)
		strcat(ui->chunk, str);
}

static void
_encoder_end(void *data)
{
	UI *ui = data;

	elm_entry_entry_set(ui->info, ui->chunk);
	free(ui->chunk);
}

static moony_encoder_t enc = {
	.begin = _encoder_begin,
	.append = _encoder_append,
	.end = _encoder_end,
	.data = NULL
};
moony_encoder_t *encoder = &enc;

static void
_hash_del(void *data)
{
	char *uri = data;

	if(uri)
		free(uri);
}

static inline const char *
_hash_set(UI *ui, LV2_URID urid)
{
	const char *uri = ui->unmap->unmap(ui->unmap->handle, urid);
	if(uri)
		eina_hash_add(ui->urids, &urid, strdup(uri));

	//printf("prefill: %s (%"PRIu32")\n", uri, urid);

	return uri;
}

static inline const char *
_hash_get(UI *ui, LV2_URID urid)
{
	const char *uri = eina_hash_find(ui->urids, &urid);

	if(!uri)
		uri = _hash_set(ui, urid);

	return uri;
}

static char * 
_list_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
	{
		char *buf = ui->string_buf;
		char *ptr = buf;
		char *end = buf + STRING_BUF_SIZE;

		sprintf(ptr, CODE_PRE);
		ptr += strlen(ptr);

		const char *type = _hash_get(ui, atom->type);
		sprintf(ptr, URI(" ", "%s (%"PRIu32")"), type, atom->type);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}

static Evas_Object * 
_seq_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#bb0 font=Mono>%04"PRIi64"</color>", ev->time.frames);

		Evas_Object *label = elm_label_add(obj);
		if(label)
		{
			elm_object_part_text_set(label, "default", buf);
			evas_object_show(label);
		}

		return label;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#0bb font=Mono>%4"PRIu32"</color>", atom->size);

		Evas_Object *label = elm_label_add(obj);
		if(label)
		{
			elm_object_part_text_set(label, "default", buf);
			evas_object_show(label);
		}

		return label;
	}

	return NULL;
}

static Evas_Object * 
_group_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const position_t *pos = data;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#000 font=Mono>0x%"PRIx64"</color>", pos->offset);

		Evas_Object *label = elm_label_add(obj);
		if(label)
		{
			elm_object_part_text_set(label, "default", buf);
			evas_object_show(label);
		}

		return label;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#0bb font=Mono>%"PRIu32"</color>", pos->nsamples);

		Evas_Object *label = elm_label_add(obj);
		if(label)
		{
			elm_object_part_text_set(label, "default", buf);
			evas_object_show(label);
		}

		return label;
	}

	return NULL;
}

static void
_del(void *data, Evas_Object *obj)
{
	free(data);
}

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
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xsd", NS_XSD);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"lv2", (const uint8_t *)LV2_CORE_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"midi", (const uint8_t *)LV2_MIDI_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"atom", (const uint8_t *)LV2_ATOM_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"units", (const uint8_t *)LV2_UNITS_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"ui", (const uint8_t *)LV2_UI_PREFIX);

	serd_env_set_prefix_from_strings(env, (const uint8_t *)"osc", NS_OSC);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xpress", NS_XPRESS);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"spod", NS_SPOD);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_ABBREVIATED |
		            SERD_STYLE_RESOLVED |
		            SERD_STYLE_CURIED),
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
_item_selected(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	const LV2_Atom_Event *ev = elm_object_item_data_get(itm);
	const LV2_Atom *atom = &ev->body;

	char *ttl = _sratom_to_turtle(ui->sratom, ui->unmap,
		ui->base_uri, NULL, NULL,
		atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
	if(ttl)
	{
		enc.data = ui;
		ttl_to_markup(ttl, NULL);

		free(ttl);
	}
}

static void
_clear_update(UI *ui, int count)
{
	if(!ui->clear)
		return;

	char *buf = ui->string_buf;
	sprintf(buf, "Clear (%"PRIi32" of %"PRIi32")", count, COUNT_MAX);
	elm_object_text_set(ui->clear, buf);
}

static void
_clear_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	if(ui->list)
		elm_genlist_clear(ui->list);

	_clear_update(ui, 0);			
}

static void
_info_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	// toggle popup
	if(ui->popup)
	{
		if(evas_object_visible_get(ui->popup))
			evas_object_hide(ui->popup);
		else
			evas_object_show(ui->popup);
	}
}

static void
_content_free(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	ui->widget = NULL;
}

static void
_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	evas_object_del(ui->widget);
}

static Evas_Object *
_content_get(UI *ui, Evas_Object *parent)
{
	ui->table = elm_table_add(parent);
	if(ui->table)
	{
		elm_table_homogeneous_set(ui->table, EINA_FALSE);
		elm_table_padding_set(ui->table, 0, 0);
		evas_object_size_hint_min_set(ui->table, 1280, 720);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_FREE, _content_free, ui);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_DEL, _content_del, ui);


		Evas_Object *panes = elm_panes_add(ui->table);
		if(panes)
		{
			elm_panes_horizontal_set(panes, EINA_FALSE);
			elm_panes_content_left_size_set(panes, 0.3);
			evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(panes, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(panes);
			elm_table_pack(ui->table, panes, 0, 0, 4, 1);

			ui->list = elm_genlist_add(panes);
			if(ui->list)
			{
				elm_genlist_homogeneous_set(ui->list, EINA_TRUE); // needef for lazy-loading
				elm_genlist_mode_set(ui->list, ELM_LIST_LIMIT);
				elm_genlist_block_count_set(ui->list, 64); // needef for lazy-loading
				elm_genlist_reorder_mode_set(ui->list, EINA_FALSE);
				elm_genlist_select_mode_set(ui->list, ELM_OBJECT_SELECT_MODE_DEFAULT);
				evas_object_data_set(ui->list, "ui", ui);
				evas_object_smart_callback_add(ui->list, "selected", _item_selected, ui);
				evas_object_size_hint_weight_set(ui->list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				evas_object_size_hint_align_set(ui->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
				evas_object_show(ui->list);
				elm_object_part_content_set(panes, "left", ui->list);
			}

			ui->info = elm_entry_add(ui->table);
			if(ui->info)
			{
				elm_entry_autosave_set(ui->info, EINA_FALSE);
				elm_entry_entry_set(ui->info, "");
				elm_entry_single_line_set(ui->info, EINA_FALSE);
				elm_entry_scrollable_set(ui->info, EINA_TRUE);
				elm_entry_editable_set(ui->info, EINA_FALSE);
				elm_entry_cnp_mode_set(ui->info, ELM_CNP_MODE_PLAINTEXT);
				evas_object_size_hint_weight_set(ui->info, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				evas_object_size_hint_align_set(ui->info, EVAS_HINT_FILL, EVAS_HINT_FILL);
				evas_object_show(ui->info);
				elm_object_part_content_set(panes, "right", ui->info);
			}
		}

		ui->clear = elm_button_add(ui->table);
		if(ui->clear)
		{
			_clear_update(ui, 0);
			evas_object_smart_callback_add(ui->clear, "clicked", _clear_clicked, ui);
			evas_object_size_hint_weight_set(ui->clear, EVAS_HINT_EXPAND, 0.f);
			evas_object_size_hint_align_set(ui->clear, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->clear);
			elm_table_pack(ui->table, ui->clear, 0, 1, 1, 1);
		}

		ui->autoclear = elm_check_add(ui->table);
		if(ui->autoclear)
		{
			elm_object_text_set(ui->autoclear, "overwrite");
			evas_object_size_hint_weight_set(ui->autoclear, 0.f, 0.f);
			evas_object_size_hint_align_set(ui->autoclear, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->autoclear);
			elm_table_pack(ui->table, ui->autoclear, 1, 1, 1, 1);
		}

		ui->autoblock = elm_check_add(ui->table);
		if(ui->autoblock)
		{
			elm_object_text_set(ui->autoblock, "block");
			evas_object_size_hint_weight_set(ui->autoblock, 0.f, 0.f);
			evas_object_size_hint_align_set(ui->autoblock, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->autoblock);
			elm_table_pack(ui->table, ui->autoblock, 2, 1, 1, 1);
		}

		Evas_Object *info = elm_button_add(ui->table);
		if(info)
		{
			evas_object_smart_callback_add(info, "clicked", _info_clicked, ui);
			evas_object_size_hint_weight_set(info, 0.f, 0.f);
			evas_object_size_hint_align_set(info, 1.f, EVAS_HINT_FILL);
			evas_object_show(info);
			elm_table_pack(ui->table, info, 3, 1, 1, 1);
				
			Evas_Object *icon = elm_icon_add(info);
			if(icon)
			{
				elm_image_file_set(icon, ui->logo_path, NULL);
				evas_object_size_hint_min_set(icon, 20, 20);
				evas_object_size_hint_max_set(icon, 32, 32);
				//evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
				evas_object_show(icon);
				elm_object_part_content_set(info, "icon", icon);
			}
		}

		ui->popup = elm_popup_add(ui->table);
		if(ui->popup)
		{
			elm_popup_allow_events_set(ui->popup, EINA_TRUE);

			Evas_Object *hbox = elm_box_add(ui->popup);
			if(hbox)
			{
				elm_box_horizontal_set(hbox, EINA_TRUE);
				elm_box_homogeneous_set(hbox, EINA_FALSE);
				elm_box_padding_set(hbox, 10, 0);
				evas_object_show(hbox);
				elm_object_content_set(ui->popup, hbox);

				Evas_Object *icon = elm_icon_add(hbox);
				if(icon)
				{
					elm_image_file_set(icon, ui->logo_path, NULL);
					evas_object_size_hint_min_set(icon, 128, 128);
					evas_object_size_hint_max_set(icon, 256, 256);
					evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
					evas_object_show(icon);
					elm_box_pack_end(hbox, icon);
				}

				Evas_Object *label = elm_label_add(hbox);
				if(label)
				{
					elm_object_text_set(label,
						"<color=#b00 shadow_color=#fff font_size=20>"
						"Sherlock - Atom Inspector"
						"</color></br><align=left>"
						"Version "SHERLOCK_VERSION"</br></br>"
						"Copyright (c) 2015 Hanspeter Portner</br></br>"
						"This is free and libre software</br>"
						"Released under Artistic License 2.0</br>"
						"By Open Music Kontrollers</br></br>"
						"<color=#bbb>"
						"http://open-music-kontrollers.ch/lv2/sherlock</br>"
						"dev@open-music-kontrollers.ch"
						"</color></align>");

					evas_object_show(label);
					elm_box_pack_end(hbox, label);
				}
			}
		}
	}

	ui->sratom = sratom_new(ui->map); //FIXME check
	sratom_set_pretty_numbers(ui->sratom, false);
	ui->base_uri = "file:///tmp/base";

	return ui->table;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(strcmp(plugin_uri, SHERLOCK_ATOM_INSPECTOR_URI))
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	ui->write_function = write_function;
	ui->controller = controller;

	Evas_Object *parent = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			ui->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
  }

	if(!ui->map || !ui->unmap)
	{
		fprintf(stderr, "LV2 URID extension not supported\n");
		free(ui);
		return NULL;
	}
	if(!parent)
	{
		free(ui);
		return NULL;
	}
	
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->itc_list = elm_genlist_item_class_new();
	if(ui->itc_list)
	{
		ui->itc_list->item_style = "default_style";
		ui->itc_list->func.text_get = _list_item_label_get;
		ui->itc_list->func.content_get = _seq_item_content_get;
		ui->itc_list->func.state_get = NULL;
		ui->itc_list->func.del = _del;
	}

	ui->itc_group = elm_genlist_item_class_new();
	if(ui->itc_group)
	{
		ui->itc_group->item_style = "default_style";
		ui->itc_group->func.text_get = NULL;
		ui->itc_group->func.content_get = _group_item_content_get;
		ui->itc_group->func.state_get = NULL;
		ui->itc_group->func.del = _del;
	}

	sprintf(ui->string_buf, "%s/omk_logo_256x256.png", bundle_path);
	ui->logo_path = strdup(ui->string_buf);

	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	ui->urids = eina_hash_int32_new(_hash_del);

	// prepopulate hash table
	_hash_set(ui, ui->forge.Bool);
	_hash_set(ui, ui->forge.Chunk);
	_hash_set(ui, ui->forge.Double);
	_hash_set(ui, ui->forge.Float);
	_hash_set(ui, ui->forge.Int);
	_hash_set(ui, ui->forge.Long);
	_hash_set(ui, ui->forge.Literal);
	_hash_set(ui, ui->forge.Object);
	_hash_set(ui, ui->forge.Path);
	_hash_set(ui, ui->forge.Property);
	_hash_set(ui, ui->forge.Sequence);
	_hash_set(ui, ui->forge.String);
	_hash_set(ui, ui->forge.Tuple);
	_hash_set(ui, ui->forge.URI);
	_hash_set(ui, ui->forge.URID);
	_hash_set(ui, ui->forge.Vector);

	ui->widget = _content_get(ui, parent);
	if(!ui->widget)
	{
		free(ui);
		return NULL;
	}
	*(Evas_Object **)widget = ui->widget;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(ui->widget)
		evas_object_del(ui->widget);
	if(ui->logo_path)
		free(ui->logo_path);

	eina_hash_free(ui->urids);

	if(ui->itc_list)
		elm_genlist_item_class_free(ui->itc_list);
	if(ui->itc_group)
		elm_genlist_item_class_free(ui->itc_group);

	if(ui->sratom)
		sratom_free(ui->sratom);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;
			
	if( (i == 2) && (urid == ui->uris.event_transfer) && ui->list)
	{
		const LV2_Atom_Tuple *tup = buf;
		const LV2_Atom_Long *offset = (const LV2_Atom_Long *)lv2_atom_tuple_begin(tup);
		const LV2_Atom_Int *nsamples = (const LV2_Atom_Int *)lv2_atom_tuple_next(&offset->atom);
		const LV2_Atom_Sequence *seq = (const LV2_Atom_Sequence *)lv2_atom_tuple_next(&nsamples->atom);
		int n = elm_genlist_items_count(ui->list);

		Elm_Object_Item *itm = NULL;
		if(seq->atom.size > sizeof(LV2_Atom_Sequence_Body)) // there are events
		{
			position_t *pos = malloc(sizeof(position_t));
			if(!pos)
				return;

			pos->offset = offset->body;
			pos->nsamples = nsamples->body;

			// check item count 
			if(n + 1 > COUNT_MAX)
			{
				if(elm_check_state_get(ui->autoclear))
				{
					elm_genlist_clear(ui->list);
					n = 0;
				}
				else
				{
					return;
				}
			}
			else if(elm_check_state_get(ui->autoblock))
			{
				return;
			}

			itm = elm_genlist_item_append(ui->list, ui->itc_group,
				pos, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
			elm_genlist_item_select_mode_set(itm, ELM_OBJECT_SELECT_MODE_NONE);

			LV2_ATOM_SEQUENCE_FOREACH(seq, elmnt)
			{
				size_t len = sizeof(LV2_Atom_Event) + elmnt->body.size;
				LV2_Atom_Event *ev = malloc(len);
				if(!ev)
					continue;

				memcpy(ev, elmnt, len);
				
				Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_list,
					ev, itm, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
				n++;
				
				// scroll to last item
				//elm_genlist_item_show(itm, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
			}
		}

		
		if(seq->atom.size > sizeof(LV2_Atom_Sequence_Body))
			_clear_update(ui, n); // only update if there where any events
	}
}

const LV2UI_Descriptor atom_inspector_eo = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL
};
