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

#include <Elementary.h>

#include <lv2_eo_ui.h>

#define COUNT_MAX 2048 // maximal amount of events shown
#define STRING_BUF_SIZE 2048
#define STRING_MAX 256
#define STRING_OFF (STRING_MAX - 4)

// Disable deprecation warnings for Blank and Resource
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	struct {
		LV2_URID midi_MidiEvent;

		LV2_URID event_transfer;

		LV2_URID time_position;
		LV2_URID time_barBeat;
		LV2_URID time_bar;
		LV2_URID time_beat;
		LV2_URID time_beatUnit;
		LV2_URID time_beatsPerBar;
		LV2_URID time_beatsPerMinute;
		LV2_URID time_frame;
		LV2_URID time_framesPerSecond;
		LV2_URID time_speed;
	} uris;
	
	LV2_Atom_Forge forge;

	Evas_Object *table;
	Evas_Object *list;
	Evas_Object *clear;
	Evas_Object *autoclear;
	Evas_Object *autoblock;
	Evas_Object *popup;

	Elm_Genlist_Item_Class *itc_sherlock;
	Elm_Genlist_Item_Class *itc_seq;
	Elm_Genlist_Item_Class *itc_prop;
	Elm_Genlist_Item_Class *itc_vec;
	Elm_Genlist_Item_Class *itc_atom;

	char string_buf [STRING_BUF_SIZE];
	char *logo_path;

	Eina_Hash *urids;
};

static inline int
_is_expandable(UI *ui, const uint32_t type)
{
	return lv2_atom_forge_is_object_type(&ui->forge, type)
		|| (type == ui->forge.Tuple)
		|| (type == ui->forge.Vector)
		|| (type == ui->forge.Sequence);
}

#define CODE_PRE "<style=shadow,bottom>"
#define CODE_POST "</style>"

#define HIL_PRE(VAL) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#b00 font=Mono>")
#define HIL_POST ("</color>")

#define URI(VAL,TYP) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#fff font=Default>"TYP"</color>")
#define HIL(VAL,TYP) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#b00 font=Mono>"TYP"</color>")

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
_atom_stringify(UI *ui, char *ptr, char *end, int newline, const LV2_Atom *atom)
{
	//FIXME check for buffer overflows!!!
	sprintf(ptr, CODE_PRE);
	ptr += strlen(ptr);

	const char *type = _hash_get(ui, atom->type);
	sprintf(ptr, URI("type    ", "%s (%"PRIu32")"), type, atom->type);
	ptr += strlen(ptr);

	if(lv2_atom_forge_is_object_type(&ui->forge, atom->type))
	{
		const LV2_Atom_Object *atom_object = (const LV2_Atom_Object *)atom;
		const char *id = atom_object->body.id
			? _hash_get(ui, atom_object->body.id)
			: NULL;
		const char *otype = _hash_get(ui, atom_object->body.otype);

		if(id)
		{
			sprintf(ptr, URI("</br>id      ", "%s (%"PRIu32")"), id, atom_object->body.id);
			ptr += strlen(ptr);
		
			if(newline)
				sprintf(ptr, "</br>");
			else
				sprintf(ptr, "</tab>");
		}
		else // !id
		{
			sprintf(ptr, "</br>");
			ptr += strlen(ptr);

			if(newline)
				sprintf(ptr, "</br>");
		}
		ptr += strlen(ptr);

		sprintf(ptr, URI("otype   ", "%s (%"PRIu32")"), otype, atom_object->body.otype);
	}
	else if(atom->type == ui->forge.Tuple)
	{
		const LV2_Atom_Tuple *atom_tuple = (const LV2_Atom_Tuple *)atom;
		
		// do nothing
		sprintf(ptr, "</br>");
	}
	else if(atom->type == ui->forge.Vector)
	{
		const LV2_Atom_Vector *atom_vector = (const LV2_Atom_Vector *)atom;
		const char *ctype = _hash_get(ui, atom_vector->body.child_type);

		sprintf(ptr, URI("</br>ctype   ", "%s (%"PRIu32")"), ctype, atom_vector->body.child_type);
		ptr += strlen(ptr);
			
		if(newline)
			sprintf(ptr, "</br>");
		else
			sprintf(ptr, "</tab>");
		ptr += strlen(ptr);

		sprintf(ptr, URI("csize   ", "%"PRIu32), atom_vector->body.child_size);
	}
	else if(atom->type == ui->forge.Sequence)
	{
		const LV2_Atom_Sequence *atom_seq = (const LV2_Atom_Sequence *)atom;
		
		// do nothing
		sprintf(ptr, "</br>");
	}
	else if(atom->type == ui->forge.Int)
	{
		const LV2_Atom_Int *atom_int = (const LV2_Atom_Int *)atom;

		sprintf(ptr, HIL("</br>value   ", "%"PRIi32), atom_int->body);
	}
	else if(atom->type == ui->forge.Long)
	{
		const LV2_Atom_Long *atom_long = (const LV2_Atom_Long *)atom;

		sprintf(ptr, HIL("</br>value   ", "%"PRIi64), atom_long->body);
	}
	else if(atom->type == ui->forge.Float)
	{
		const LV2_Atom_Float *atom_float = (const LV2_Atom_Float *)atom;

		sprintf(ptr, HIL("</br>value   ", "%f"), atom_float->body);
	}
	else if(atom->type == ui->forge.Double)
	{
		const LV2_Atom_Double *atom_double = (const LV2_Atom_Double *)atom;

		sprintf(ptr, HIL("</br>value   ", "%lf"), atom_double->body);
	}
	else if(atom->type == ui->forge.Bool)
	{
		const LV2_Atom_Int *atom_int = (const LV2_Atom_Int *)atom;

		sprintf(ptr, HIL("</br>value   ", "%s"), atom_int->body ? "true" : "false");
	}
	else if(atom->type == ui->forge.URID)
	{
		const LV2_Atom_URID *atom_urid = (const LV2_Atom_URID *)atom;
		const char *uri = _hash_get(ui, atom_urid->body);

		sprintf(ptr, HIL("</br>value   ", "%"PRIu32" (%s)"), atom_urid->body, uri);
	}
	else if(atom->type == ui->forge.String)
	{
		char *str = LV2_ATOM_CONTENTS(LV2_Atom_String, atom);

		// truncate
		char tmp[4] = {'\0'};
		if(atom->size > STRING_MAX)
		{
			memcpy(tmp, &str[STRING_OFF], 4);
			strcpy(&str[STRING_OFF], "...");
		}

		sprintf(ptr, HIL("</br>value   ", "%s"), str);

		// restore
		if(atom->size > STRING_MAX)
			memcpy(&str[STRING_OFF], tmp, 4);
	}
	else if(atom->type == ui->forge.Path)
	{
		char *str = LV2_ATOM_CONTENTS(LV2_Atom_String, atom);

		// truncate
		char tmp[4] = {'\0'};
		if(atom->size > STRING_MAX)
		{
			memcpy(tmp, &str[STRING_OFF], 4);
			strcpy(&str[STRING_OFF], "...");
		}

		sprintf(ptr, HIL("</br>value   ", "%s"), str);

		// restore
		if(atom->size > STRING_MAX)
			memcpy(&str[STRING_OFF], tmp, 4);
	}
	else if(atom->type == ui->forge.Literal)
	{
		const LV2_Atom_Literal *atom_lit = (const LV2_Atom_Literal *)atom;

		char *str = LV2_ATOM_CONTENTS(LV2_Atom_Literal, atom);
		const char *datatype = _hash_get(ui, atom_lit->body.datatype);
		const char *lang = _hash_get(ui, atom_lit->body.lang);

		// truncate
		char tmp[4] = {'\0'};
		if(atom->size > STRING_MAX)
		{
			memcpy(tmp, &str[STRING_OFF], 4);
			strcpy(&str[STRING_OFF], "...");
		}

		sprintf(ptr, HIL("</br>value   ", "%s"), str);
		ptr += strlen(ptr);

		// restore
		if(atom->size > STRING_MAX)
			memcpy(&str[STRING_OFF], tmp, 4);
			
		if(newline)
			sprintf(ptr, "</br>");
		else
			sprintf(ptr, "</tab>");
		ptr += strlen(ptr);

		sprintf(ptr, URI("datatype", "%s (%"PRIu32")"), datatype, atom_lit->body.datatype);
		ptr += strlen(ptr);

		sprintf(ptr, URI("</tab>lang    ", "%s (%"PRIu32")"), lang, atom_lit->body.lang);
	}
	else if(atom->type == ui->forge.URI)
	{
		char *str = LV2_ATOM_CONTENTS(LV2_Atom_String, atom);
		LV2_URID urid = ui->map->map(ui->map->handle, str); //TODO add hashing

		// truncate
		char tmp[4] = {'\0'};
		if(atom->size > STRING_MAX)
		{
			memcpy(tmp, &str[STRING_OFF], 4);
			strcpy(&str[STRING_OFF], "...");
		}

		sprintf(ptr, HIL("</br>value   ", "%s (%"PRIu32")"), str, urid);

		// restore
		if(atom->size > STRING_MAX)
			memcpy(&str[STRING_OFF], tmp, 4);
	}
	else if(atom->type == ui->uris.midi_MidiEvent)
	{
		const uint8_t *midi = LV2_ATOM_BODY_CONST(atom);

		sprintf(ptr, HIL_PRE("</br>value   "));
		ptr += strlen(ptr);

		char *barrier = ptr + STRING_OFF;
		for(unsigned i=0; (i<atom->size) && (ptr<barrier); i++, ptr += 3)
			sprintf(ptr, "%02X ", midi[i]);

		if(ptr >= barrier) // there would be more to print
		{
			ptr = barrier;
			sprintf(ptr, "...");
			ptr += 4;
		}

		sprintf(ptr, HIL_POST);
	}
	else if(atom->type == ui->forge.Chunk)
	{
		const uint8_t *chunk = LV2_ATOM_BODY_CONST(atom);

		sprintf(ptr, HIL_PRE("</br>value   "));
		ptr += strlen(ptr);
			
		char *barrier = ptr + STRING_OFF;
		for(unsigned i=0; (i<atom->size) && (ptr<barrier); i++, ptr += 3)
			sprintf(ptr, "%02X ", chunk[i]);

		if(ptr >= barrier) // there would be more to print
		{
			ptr = barrier;
			sprintf(ptr, "...");
			ptr += 4;
		}

		sprintf(ptr, HIL_POST);
	}

	if(  newline
		&& !lv2_atom_forge_is_object_type(&ui->forge, atom->type)
		&& !(atom->type == ui->forge.Literal)
		&& !(atom->type == ui->forge.Vector) )
	{
		ptr += strlen(ptr);

		sprintf(ptr, "</br>");
	}
	ptr += strlen(ptr);

	sprintf(ptr, CODE_POST);

	return ptr + strlen(ptr);
}

static char *
_atom_item_label_get(void *data, Evas_Object *obj, const char *part)
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

		ptr = _atom_stringify(ui, ptr, end, 1, atom);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}

static char * 
_prop_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Property_Body *prop = data;

	if(!ui)
		return NULL;
	
	if(!strcmp(part, "elm.text"))
	{
		char *buf = ui->string_buf; 
		char *ptr = buf;
		char *end = buf + STRING_BUF_SIZE;

		const char *key = _hash_get(ui, prop->key);
		const char *context = _hash_get(ui, prop->context);

		sprintf(ptr, URI("key     ", "%s (%"PRIu32")"), key, prop->key);
		ptr += strlen(ptr);

		if(context)
		{
			sprintf(ptr, URI("</tab>context ", "%s (%"PRIu32")"), context, prop->context);
			ptr += strlen(ptr);
		}
		
		sprintf(ptr, "</br>");
		ptr += strlen(ptr);

		ptr = _atom_stringify(ui, ptr, end, 0, &prop->value);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}

static char * 
_seq_item_label_get(void *data, Evas_Object *obj, const char *part)
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

		ptr = _atom_stringify(ui, ptr, end, 1, atom);

		return ptr
			? strdup(buf)
			: NULL;
	}

	return NULL;
}


static Evas_Object * 
_atom_item_content_get(void *data, Evas_Object *obj, const char *part)
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

static Evas_Object * 
_prop_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Property_Body *prop = data;
	const LV2_Atom *atom = &prop->value;

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
_item_del(void *data, Evas_Object *obj)
{
	free(data);
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
_atom_expand(UI *ui, const LV2_Atom *atom, Evas_Object *obj, Elm_Object_Item *itm)
{
	if(lv2_atom_forge_is_object_type(&ui->forge, atom->type))
	{
		const LV2_Atom_Object *atom_object = (const LV2_Atom_Object *)atom;

		LV2_ATOM_OBJECT_FOREACH(atom_object, prop)
		{
			Elm_Genlist_Item_Type type = _is_expandable(ui, prop->value.type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;

			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_prop,
				prop, itm, type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
		}
	}
	else if(atom->type == ui->forge.Tuple)
	{
		const LV2_Atom_Tuple *atom_tuple = (const LV2_Atom_Tuple *)atom;

		for(LV2_Atom *iter = lv2_atom_tuple_begin(atom_tuple);
			!lv2_atom_tuple_is_end(LV2_ATOM_BODY(atom_tuple), atom_tuple->atom.size, iter);
			iter = lv2_atom_tuple_next(iter))
		{
			Elm_Genlist_Item_Type type = _is_expandable(ui, iter->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;

			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_atom,
				iter, itm, type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
		}
	}
	else if(atom->type == ui->forge.Vector)
	{
		const LV2_Atom_Vector *atom_vector = (const LV2_Atom_Vector *)atom;

		Elm_Genlist_Item_Type type = _is_expandable(ui, atom_vector->body.child_type)
			? ELM_GENLIST_ITEM_TREE
			: ELM_GENLIST_ITEM_NONE;

		int num = (atom_vector->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ atom_vector->body.child_size;
		const uint8_t *body = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, atom_vector);
		for(int i=0; i<num; i++)
		{
			LV2_Atom *child = malloc(sizeof(LV2_Atom) + atom_vector->body.child_size);
			if(child)
			{
				child->size = atom_vector->body.child_size;
				child->type = atom_vector->body.child_type;
				memcpy(LV2_ATOM_BODY(child), body + i*child->size, child->size);

				Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_vec,
					child, itm, type, NULL, NULL);
				elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
				elm_genlist_item_expanded_set(itm2, EINA_FALSE);
			}
		}
	}
	else if(atom->type == ui->forge.Sequence)
	{
		const LV2_Atom_Sequence *atom_seq = (const LV2_Atom_Sequence *)atom;

		LV2_ATOM_SEQUENCE_FOREACH(atom_seq, ev)
		{
			const LV2_Atom *child = &ev->body;

			Elm_Genlist_Item_Type type = _is_expandable(ui, child->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;

			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_seq,
				ev, itm, type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
		}
	}
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

	if( (class == ui->itc_sherlock) || (class == ui->itc_seq) )
	{
		const LV2_Atom_Event *ev = udata;
		const LV2_Atom *atom = &ev->body;
		_atom_expand(ui, atom, obj, itm);
	}
	else if(class == ui->itc_prop)
	{
		const LV2_Atom_Property_Body *prop = udata;
		const LV2_Atom *atom = &prop->value;

		_atom_expand(ui, atom, obj, itm);
	}
	else
	{
		const LV2_Atom *atom = udata;
		_atom_expand(ui, atom, obj, itm);
	}
}

static void
_item_contracted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	elm_genlist_item_subitems_clear(itm);
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

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);

	ui->table = elm_table_add(eoui->win);
	if(ui->table)
	{
		elm_table_homogeneous_set(ui->table, EINA_FALSE);
		elm_table_padding_set(ui->table, 0, 0);

		ui->list = elm_genlist_add(ui->table);
		if(ui->list)
		{
			elm_genlist_select_mode_set(ui->list, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_homogeneous_set(ui->list, EINA_FALSE); // TRUE for lazy-loading
			elm_genlist_mode_set(ui->list, ELM_LIST_SCROLL);
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

	eo_ui_driver_t driver;
	if(descriptor == &atom_inspector_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &atom_inspector_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &atom_inspector_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &atom_inspector_kx)
		driver = EO_UI_DRIVER_KX;
	else
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 1024,
	eoui->h = 480;

	ui->write_function = write_function;
	ui->controller = controller;
	
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			ui->unmap = (LV2_URID_Unmap *)features[i]->data;
  }

	if(!ui->map || !ui->unmap)
	{
		fprintf(stderr, "LV2 URID extension not supported\n");
		free(ui);
		return NULL;
	}
	
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->itc_sherlock = elm_genlist_item_class_new();
	if(ui->itc_sherlock)
	{
		ui->itc_sherlock->item_style = "default_style";
		ui->itc_sherlock->func.text_get = _seq_item_label_get;
		ui->itc_sherlock->func.content_get = _seq_item_content_get;
		ui->itc_sherlock->func.state_get = NULL;
		ui->itc_sherlock->func.del = _item_del;
	}

	ui->itc_seq = elm_genlist_item_class_new();
	if(ui->itc_seq)
	{
		ui->itc_seq->item_style = "default_style";
		ui->itc_seq->func.text_get = _seq_item_label_get;
		ui->itc_seq->func.content_get = _seq_item_content_get;
		ui->itc_seq->func.state_get = NULL;
		ui->itc_seq->func.del = NULL;
	}

	ui->itc_prop = elm_genlist_item_class_new();
	if(ui->itc_prop)
	{
		ui->itc_prop->item_style = "default_style";
		ui->itc_prop->func.text_get = _prop_item_label_get;
		ui->itc_prop->func.content_get = _prop_item_content_get;
		ui->itc_prop->func.state_get = NULL;
		ui->itc_prop->func.del = NULL;
	}
	
	ui->itc_vec = elm_genlist_item_class_new();
	if(ui->itc_vec)
	{
		ui->itc_vec->item_style = "default_style";
		ui->itc_vec->func.text_get = _atom_item_label_get;
		ui->itc_vec->func.content_get = _atom_item_content_get;
		ui->itc_vec->func.state_get = NULL;
		ui->itc_vec->func.del = _item_del;
	}
	
	ui->itc_atom = elm_genlist_item_class_new();
	if(ui->itc_atom)
	{
		ui->itc_atom->item_style = "default_style";
		ui->itc_atom->func.text_get = _atom_item_label_get;
		ui->itc_atom->func.content_get = _atom_item_content_get;
		ui->itc_atom->func.state_get = NULL;
		ui->itc_atom->func.del = NULL;
	}

	sprintf(ui->string_buf, "%s/omk_logo_256x256.png", bundle_path);
	ui->logo_path = strdup(ui->string_buf);

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}
	
	ui->uris.midi_MidiEvent = ui->map->map(ui->map->handle, LV2_MIDI__MidiEvent);

	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	ui->uris.time_position = ui->map->map(ui->map->handle, LV2_TIME__Position);
	ui->uris.time_barBeat = ui->map->map(ui->map->handle, LV2_TIME__barBeat);
	ui->uris.time_bar = ui->map->map(ui->map->handle, LV2_TIME__bar);
	ui->uris.time_beat = ui->map->map(ui->map->handle, LV2_TIME__beat);
	ui->uris.time_beatUnit = ui->map->map(ui->map->handle, LV2_TIME__beatUnit);
	ui->uris.time_beatsPerBar = ui->map->map(ui->map->handle, LV2_TIME__beatsPerBar);
	ui->uris.time_beatsPerMinute = ui->map->map(ui->map->handle, LV2_TIME__beatsPerMinute);
	ui->uris.time_frame = ui->map->map(ui->map->handle, LV2_TIME__frame);
	ui->uris.time_framesPerSecond = ui->map->map(ui->map->handle, LV2_TIME__framesPerSecond);
	ui->uris.time_speed = ui->map->map(ui->map->handle, LV2_TIME__speed);

	ui->urids = eina_hash_int32_new(_hash_del);

	// prepopulate hash table
	_hash_set(ui, ui->forge.Blank);
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
	_hash_set(ui, ui->forge.Resource);
	_hash_set(ui, ui->forge.Sequence);
	_hash_set(ui, ui->forge.String);
	_hash_set(ui, ui->forge.Tuple);
	_hash_set(ui, ui->forge.URI);
	_hash_set(ui, ui->forge.URID);
	_hash_set(ui, ui->forge.Vector);
	
	_hash_set(ui, ui->uris.midi_MidiEvent);
	
	_hash_set(ui, ui->uris.time_position);
	_hash_set(ui, ui->uris.time_barBeat);
	_hash_set(ui, ui->uris.time_bar);
	_hash_set(ui, ui->uris.time_beat);
	_hash_set(ui, ui->uris.time_beatUnit);
	_hash_set(ui, ui->uris.time_beatsPerBar);
	_hash_set(ui, ui->uris.time_beatsPerMinute);
	_hash_set(ui, ui->uris.time_frame);
	_hash_set(ui, ui->uris.time_framesPerSecond);
	_hash_set(ui, ui->uris.time_speed);

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	eoui_cleanup(&ui->eoui);

	if(ui->logo_path)
		free(ui->logo_path);

	eina_hash_free(ui->urids);

	if(ui->itc_atom)
		elm_genlist_item_class_free(ui->itc_atom);
	if(ui->itc_vec)
		elm_genlist_item_class_free(ui->itc_vec);
	if(ui->itc_prop)
		elm_genlist_item_class_free(ui->itc_prop);
	if(ui->itc_seq)
		elm_genlist_item_class_free(ui->itc_seq);
	if(ui->itc_sherlock)
		elm_genlist_item_class_free(ui->itc_sherlock);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;
			
	if( (i == 2) && (urid == ui->uris.event_transfer) && ui->list)
	{
		const LV2_Atom_Sequence *seq = buf;
		int n = elm_genlist_items_count(ui->list);

		LV2_ATOM_SEQUENCE_FOREACH(seq, elmnt)
		{
			size_t len = sizeof(LV2_Atom_Event) + elmnt->body.size;
			LV2_Atom_Event *ev = malloc(len);
			if(!ev)
				continue;

			memcpy(ev, elmnt, len);

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
					break;
				}
			}
			else if(elm_check_state_get(ui->autoblock))
			{
				break;
			}
			
			const LV2_Atom *atom = &elmnt->body;
			Elm_Genlist_Item_Type type = _is_expandable(ui, atom->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;

			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_sherlock,
				ev, NULL, type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
			n++;
			
			// scroll to last item
			//elm_genlist_item_show(itm, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
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
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor atom_inspector_ui = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor atom_inspector_x11 = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor atom_inspector_kx = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
