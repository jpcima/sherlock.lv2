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

#include <sherlock.h>

#include <osc.lv2/util.h>

#include <Elementary.h>

#define COUNT_MAX 2048 // maximal amount of events shown
#define STRING_BUF_SIZE 2048
#define STRING_MAX 256
#define STRING_OFF (STRING_MAX - 4)

typedef struct _UI UI;

struct _UI {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_URID event_transfer;
	LV2_URID midi_event;
	LV2_OSC_URID osc_urid;

	Evas_Object *widget;
	Evas_Object *table;
	Evas_Object *list;
	Evas_Object *clear;
	Evas_Object *autoclear;
	Evas_Object *autoblock;
	Evas_Object *popup;

	Elm_Genlist_Item_Class *itc_group;
	Elm_Genlist_Item_Class *itc_packet;
	Elm_Genlist_Item_Class *itc_item;

	char string_buf [STRING_BUF_SIZE];
	char *logo_path;
};

// there is a bug in LV2 <= 0.10
#if defined(LV2_ATOM_TUPLE_FOREACH)
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

#define CODE_PRE ("<font=Mono style=shadow,bottom>")
#define CODE_POST ("</font>")

#define PATH(VAL) ("<color=#b0b><b>"VAL"</b></color>")
#define BUNDLE(VAL) ("<color=#b0b><b>"VAL"</b></color>")
#define TYPE(TYP, VAL) ("<color=#0b0><b>"TYP"</b></color><color=#fff>"VAL"</color>")

#define TYPE_PRE(TYP, VAL) ("<color=#0b0><b>"TYP"</b></color><color=#fff>"VAL)
#define TYPE_POST(VAL) (VAL"</color>")

#define PUNKT(VAL) "<color=#00b>"VAL"</color>"

static inline char *
_timetag_stringify(UI *ui, char *ptr, char *end, uint64_t body)
{
	const uint32_t sec = body >> 32;
	const uint32_t frac = body & 0xffffffff;
	const double part = frac * 0x1p-32;

	if(sec <= 1UL)
	{
		sprintf(ptr, TYPE(" t:", "immediate"));
	}
	else
	{
		const time_t ttime = sec - 0x83aa7e80;
		const struct tm *ltime = localtime(&ttime);

		char tmp [32];
		strftime(tmp, 32, "%d-%b-%Y %T", ltime);

		sprintf(ptr, TYPE(" t:", "%s.%06"PRIu32), tmp, (uint32_t)(part*1e6));
	}

	return ptr + strlen(ptr);
}

static inline char *
_atom_stringify(UI *ui, char *ptr, char *end, const LV2_Atom *atom)
{
	//FIXME check for buffer overflows!!!
	const LV2_Atom_Object *obj = (const LV2_Atom_Object *)atom;

	if(lv2_osc_is_message_type(&ui->osc_urid, obj->body.otype))
	{
		const LV2_Atom_String *path = NULL;
		const LV2_Atom_Tuple *tup = NULL;
		lv2_osc_message_get(&ui->osc_urid, obj, &path, &tup);

		sprintf(ptr, CODE_PRE);
		ptr += strlen(ptr);

		if(path)
		{
			sprintf(ptr, PATH(" %s"), LV2_ATOM_BODY_CONST(path));
		}
		else
		{
			sprintf(ptr, " unknown path and/or format string");
		}
		ptr += strlen(ptr);

		LV2_ATOM_TUPLE_FOREACH(tup, itm)
		{
			switch(lv2_osc_argument_type(&ui->osc_urid, itm))
			{
				case LV2_OSC_INT32:
				{
					sprintf(ptr, TYPE(" i:", "%"PRIi32), ((const LV2_Atom_Int *)itm)->body);
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_FLOAT:
				{
					sprintf(ptr, TYPE(" f:", "%f"), ((const LV2_Atom_Float *)itm)->body);
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_STRING: // fall-through
				{
					const char *str = LV2_ATOM_BODY_CONST(itm);
					if(itm->size == 0)
						str = "";
					sprintf(ptr, TYPE(" s:", ""));
					ptr += strlen(ptr);

					for(unsigned i=0; i<strlen(str) + 1; i++)
					{
						switch(str[i])
						{
							case '<':
								strncpy(ptr, "&lt;", 4);
								ptr += 4;
								break;
							case '>':
								strncpy(ptr, "&gt;", 4);
								ptr += 4;
								break;
							case '&':
								strncpy(ptr, "&amp;", 5);
								ptr += 5;
								break;
							case '\n':
								strncpy(ptr, "\\n", 2);
								ptr += 2;
								break;
							case '\r':
								strncpy(ptr, "\\r", 2);
								ptr += 2;
								break;
							default:
								*ptr++ = str[i];
								break;
						}
					}
					break;
				}
				case LV2_OSC_BLOB:
				{
					const uint8_t *chunk = LV2_ATOM_BODY_CONST(itm);
					sprintf(ptr, TYPE_PRE(" b:", PUNKT("[")));
					ptr += strlen(ptr);
					if(itm->size)
					{
						sprintf(ptr, "%02"PRIX8, chunk[0]);
						ptr += strlen(ptr);

						for(unsigned i=1; i<itm->size; i++)
						{
							sprintf(ptr, " %02"PRIX8, chunk[i]);
							ptr += strlen(ptr);
						}
					}
					sprintf(ptr, TYPE_POST(PUNKT("]")));
					ptr += strlen(ptr);
					break;
				}

				case LV2_OSC_INT64:
				{
					sprintf(ptr, TYPE(" h:", "%"PRIi64), ((const LV2_Atom_Long *)itm)->body);
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_DOUBLE:
				{
					sprintf(ptr, TYPE(" d:", "%lf"), ((const LV2_Atom_Double *)itm)->body);
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_TIMETAG:
				{
					LV2_OSC_Timetag tt;
					lv2_osc_timetag_get(&ui->osc_urid, (const LV2_Atom_Object *)itm, &tt);
					ptr = _timetag_stringify(ui, ptr, end, lv2_osc_timetag_parse(&tt));
					break;
				}

				case LV2_OSC_SYMBOL:
				{
					sprintf(ptr, TYPE(" S:", "%s"), ui->unmap->unmap(ui->unmap->handle, ((const LV2_Atom_URID *)itm)->body));
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_MIDI:
				{
					const uint8_t *chunk = LV2_ATOM_BODY_CONST(itm);
					sprintf(ptr, TYPE_PRE(" m:", PUNKT("[")));
					ptr += strlen(ptr);
					if(itm->size)
					{
						sprintf(ptr, "%02"PRIX8, chunk[0]);
						ptr += strlen(ptr);

						for(unsigned i=1; i<itm->size; i++)
						{
							sprintf(ptr, " %02"PRIX8, chunk[i]);
							ptr += strlen(ptr);
						}
					}
					sprintf(ptr, TYPE_POST(PUNKT("]")));
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_CHAR:
				{
					sprintf(ptr, TYPE(" c:", "%c"), ((const LV2_Atom_Int *)itm)->body);
					ptr += strlen(ptr);
					break;
				}
				case LV2_OSC_RGBA:
				{
					const uint8_t *chunk = LV2_ATOM_BODY_CONST(itm);
					sprintf(ptr, TYPE_PRE(" r:", PUNKT("[")));
					ptr += strlen(ptr);
					if(itm->size)
					{
						sprintf(ptr, "%02"PRIX8, chunk[0]);
						ptr += strlen(ptr);

						for(unsigned i=1; i<itm->size; i++)
						{
							sprintf(ptr, " %02"PRIX8, chunk[i]);
							ptr += strlen(ptr);
						}
					}
					sprintf(ptr, TYPE_POST(PUNKT("]")));
					ptr += strlen(ptr);
					break;
				}

				case LV2_OSC_TRUE:
				{
					//if(itm->type == ui->forge.Bool)
					{
						sprintf(ptr, TYPE(" T:", "true"));
						ptr += strlen(ptr);
					}
					break;
				}
				case LV2_OSC_FALSE:
				{
					//if(itm->type == ui->forge.Bool)
					{
						sprintf(ptr, TYPE(" F:", "false"));
						ptr += strlen(ptr);
					}
					break;
				}
				case LV2_OSC_NIL:
				{
					//if(itm->type == 0)
					{
						sprintf(ptr, TYPE(" N:", "nil"));
						ptr += strlen(ptr);
					}
					break;
				}
				case LV2_OSC_IMPULSE:
				{
					//if(itm->type == ui->forge.Impulse)
					{
						sprintf(ptr, TYPE(" I:", "impulse"));
						ptr += strlen(ptr);
					}
					break;
				}
			}
		}

		sprintf(ptr, CODE_POST);

		return ptr + strlen(ptr);
	}
	else if(lv2_osc_is_bundle_type(&ui->osc_urid, obj->body.otype))
	{
		const LV2_Atom_Object *timetag = NULL;
		const LV2_Atom_Tuple *tup = NULL;
		lv2_osc_bundle_get(&ui->osc_urid, obj, &timetag, &tup);
		
		sprintf(ptr, CODE_PRE);
		ptr += strlen(ptr);

		sprintf(ptr, BUNDLE(" #bundle"));
		ptr += strlen(ptr);

		LV2_OSC_Timetag tt;
		lv2_osc_timetag_get(&ui->osc_urid, timetag, &tt);
		ptr = _timetag_stringify(ui, ptr, end, lv2_osc_timetag_parse(&tt));

		sprintf(ptr, CODE_POST);

		return ptr + strlen(ptr);
	}

	return NULL;
}

static char *
_packet_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
	{
		char *buf = ui->string_buf;
		char *ptr = buf;
		char *end = buf + STRING_BUF_SIZE;

		ptr = _atom_stringify(ui, ptr, end, &ev->body);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}

static Evas_Object * 
_packet_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	
	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#bb0 font=Mono>%04ld</color>", ev->time.frames);

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

		sprintf(buf, "<color=#0bb font=Mono>%4u</color>", ev->body.size);

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

static char *
_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
	{
		char *buf = ui->string_buf;
		char *ptr = buf;
		char *end = buf + STRING_BUF_SIZE;

		ptr = _atom_stringify(ui, ptr, end, atom);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}

static Evas_Object * 
_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;
	
	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.end"))
	{
		char *buf = ui->string_buf;

		sprintf(buf, "<color=#0bb font=Mono>%4u</color>", atom->size);

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
_item_expand_request(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_expanded_set(itm, EINA_TRUE);
}

static void
_item_contract_request(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_expanded_set(itm, EINA_FALSE);
}

static void
_item_expanded(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	const Elm_Genlist_Item_Class *class = elm_genlist_item_item_class_get(itm);
	const void *udata = elm_object_item_data_get(itm);

	if(!udata)
		return;

	const LV2_Atom_Object *_obj = NULL;

	if(class == ui->itc_packet)
	{
		const LV2_Atom_Event *ev = udata;
		_obj = (const LV2_Atom_Object *)&ev->body;
	}
	else if(class == ui->itc_item)
	{
		_obj = udata;
	}

	if(_obj && lv2_osc_is_bundle_type(&ui->osc_urid, _obj->body.otype))
	{
		const LV2_Atom_Object *timetag = NULL;
		const LV2_Atom_Tuple *tup = NULL;
		lv2_osc_bundle_get(&ui->osc_urid, _obj, &timetag, &tup);

		if(tup)
		{
			LV2_ATOM_TUPLE_FOREACH(tup, atom)
			{
				const LV2_Atom_Object *_obj2 = (const LV2_Atom_Object *)atom;

				Elm_Object_Item *itm2 = elm_genlist_item_append(obj, ui->itc_item,
					_obj2, itm, ELM_GENLIST_ITEM_TREE, NULL, NULL);
				elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
				if(lv2_osc_is_bundle_type(&ui->osc_urid, _obj2->body.otype))
					elm_genlist_item_expanded_set(itm2, EINA_TRUE);
			}
		}
	}
}

static void
_item_contracted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_subitems_clear(itm);
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
		evas_object_size_hint_min_set(ui->table, 600, 800);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_FREE, _content_free, ui);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_DEL, _content_del, ui);

		ui->list = elm_genlist_add(ui->table);
		if(ui->list)
		{
			elm_genlist_homogeneous_set(ui->list, EINA_TRUE); // needef for lazy-loading
			elm_genlist_mode_set(ui->list, ELM_LIST_LIMIT);
			elm_genlist_block_count_set(ui->list, 64); // needef for lazy-loading
			elm_genlist_reorder_mode_set(ui->list, EINA_FALSE);
			elm_genlist_select_mode_set(ui->list, ELM_OBJECT_SELECT_MODE_DEFAULT);
			evas_object_data_set(ui->list, "ui", ui);
			evas_object_smart_callback_add(ui->list, "expand,request",
				_item_expand_request, ui);
			evas_object_smart_callback_add(ui->list, "contract,request",
				_item_contract_request, ui);
			evas_object_smart_callback_add(ui->list, "expanded", _item_expanded, ui);
			evas_object_smart_callback_add(ui->list, "contracted", _item_contracted, ui);
			evas_object_size_hint_weight_set(ui->list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(ui->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->list);
			elm_table_pack(ui->table, ui->list, 0, 0, 4, 1);
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
						"Sherlock - OSC Inspector"
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

	return ui->table;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(strcmp(plugin_uri, SHERLOCK_OSC_INSPECTOR_URI))
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
		if(!strcmp(features[i]->URI, LV2_URID__unmap))
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
	
	ui->event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);
	ui->midi_event = ui->map->map(ui->map->handle, LV2_MIDI__MidiEvent);
	lv2_atom_forge_init(&ui->forge, ui->map);
	lv2_osc_urid_init(&ui->osc_urid, ui->map);

	ui->itc_packet = elm_genlist_item_class_new();
	if(ui->itc_packet)
	{
		ui->itc_packet->item_style = "default_style";
		ui->itc_packet->func.text_get = _packet_label_get;
		ui->itc_packet->func.content_get = _packet_content_get;
		ui->itc_packet->func.state_get = NULL;
		ui->itc_packet->func.del = _del;
	}

	ui->itc_item = elm_genlist_item_class_new();
	if(ui->itc_item)
	{
		ui->itc_item->item_style = "default_style";
		ui->itc_item->func.text_get = _item_label_get;
		ui->itc_item->func.content_get = _item_content_get;
		ui->itc_item->func.state_get = NULL;
		ui->itc_item->func.del = NULL;
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

	if(ui->itc_packet)
		elm_genlist_item_class_free(ui->itc_packet);
	if(ui->itc_item)
		elm_genlist_item_class_free(ui->itc_item);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;
			
	if( (i == 2) && (urid == ui->event_transfer) && ui->list)
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

				Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_packet,
					ev, itm, ELM_GENLIST_ITEM_TREE, NULL, NULL);
				elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
				elm_genlist_item_expanded_set(itm2, EINA_FALSE);
				n++;
				
				// scroll to last item
				//elm_genlist_item_show(itm, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
			}
		}

		if(seq->atom.size > sizeof(LV2_Atom_Sequence_Body))
			_clear_update(ui, n); // only update if there where any events
	}
}

const LV2UI_Descriptor osc_inspector_eo = {
	.URI						= SHERLOCK_OSC_INSPECTOR_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL
};
