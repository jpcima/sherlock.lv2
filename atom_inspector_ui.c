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

#include <sherlock.h>

#include <Elementary.h>

#include <lv2_eo_ui.h>

#define COUNT_MAX 2018 // maximal amount of events shown

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
		LV2_URID atom_transfer;
	} uris;
	
	LV2_Atom_Forge forge;

	Evas_Object *vbox;
	Evas_Object *list;
	Evas_Object *clear;

	Elm_Genlist_Item_Class *itc_sherlock;
	Elm_Genlist_Item_Class *itc_prop;
	Elm_Genlist_Item_Class *itc_vec;
	Elm_Genlist_Item_Class *itc_atom;
};

static inline int
_is_expandable(UI *ui, const uint32_t type)
{
	return (type == ui->forge.Object)
		|| (type == ui->forge.Blank)
		|| (type == ui->forge.Resource)
		|| (type == ui->forge.Tuple)
		|| (type == ui->forge.Vector);
}

#define HIL_PRE(VAL) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#b00>")
#define HIL_POST ("</color></br></br>")

#define URI(VAL,TYP) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#fff>"TYP"</color>")
#define HIL(VAL,TYP) ("<color=#bbb font=Mono><b>"VAL"</b></color> <color=#b00>"TYP"</color>")

static char * 
_atom_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
	{
		char buf [1024];
		char *ptr = buf;
		char *end = buf + 1024;
		
		const char *type = ui->unmap->unmap(ui->unmap->handle, atom->type);
		
		sprintf(ptr, URI("type    ", "%s</br>"), type);
		ptr += strlen(ptr);

		if(  (atom->type == ui->forge.Object)
			|| (atom->type == ui->forge.Blank)
			|| (atom->type == ui->forge.Resource) )
		{
			const LV2_Atom_Object *atom_object = data;
			const char *id = ui->unmap->unmap(ui->unmap->handle,
				atom_object->body.id);
			const char *otype = ui->unmap->unmap(ui->unmap->handle,
				atom_object->body.otype);

			sprintf(ptr, URI("id      ", "%s</br>"), id);
			ptr += strlen(ptr);

			sprintf(ptr, URI("otype   ", "%s</br>"), otype);
		}
		else if(atom->type == ui->forge.Tuple)
		{
			const LV2_Atom_Tuple *atom_tuple = data;

			sprintf(ptr, "</br></br>");
		}
		else if(atom->type == ui->forge.Vector)
		{
			const LV2_Atom_Vector *atom_vector = data;
			const char *ctype = ui->unmap->unmap(ui->unmap->handle,
				atom_vector->body.child_type);

			sprintf(ptr, URI("type    ", "%s</br></br>"), ctype);
		}
		else if(atom->type == ui->forge.Int)
		{
			const LV2_Atom_Int *atom_int = data;

			sprintf(ptr, HIL("val     ", "%d</br></br>"), atom_int->body);
		}
		else if(atom->type == ui->forge.Long)
		{
			const LV2_Atom_Long *atom_long = data;

			sprintf(ptr, HIL("val     ", "%ld</br></br>"), atom_long->body);
		}
		else if(atom->type == ui->forge.Float)
		{
			const LV2_Atom_Float *atom_float = data;

			sprintf(ptr, HIL("val     ", "%f</br></br>"), atom_float->body);
		}
		else if(atom->type == ui->forge.Double)
		{
			const LV2_Atom_Double *atom_double = data;

			sprintf(ptr, HIL("val     ", "%lf</br></br>"), atom_double->body);
		}
		else if(atom->type == ui->forge.Bool)
		{
			const LV2_Atom_Int *atom_int = data;

			sprintf(ptr, HIL("val     ", "%s</br></br>"), atom_int->body ? "true" : "false");
		}
		else if(atom->type == ui->forge.URID)
		{
			const LV2_Atom_URID *atom_urid = data;

			sprintf(ptr, HIL("val     ", "%u</br></br>"), atom_urid->body);
		}
		else if(atom->type == ui->forge.String)
		{
			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom);

			sprintf(ptr, HIL("val     ", "%s</br></br>"), str);
		}
		else if(atom->type == ui->forge.Path)
		{
			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom);

			sprintf(ptr, HIL("val     ", "%s</br></br>"), str);
		}
		else if(atom->type == ui->forge.Literal)
		{
			const LV2_Atom_Literal *atom_lit = data;

			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, atom);
			const char *datatype = ui->unmap->unmap(ui->unmap->handle,
				atom_lit->body.datatype);
			const char *lang = ui->unmap->unmap(ui->unmap->handle,
				atom_lit->body.lang);

			sprintf(ptr, HIL("val     ", "%s</br>"), str);
			ptr += strlen(ptr);
			
			sprintf(ptr, URI("datatype", "%s</br>"), datatype);
			ptr += strlen(ptr);
			
			sprintf(ptr, URI("lang    ", "%s"), lang);
		}
		else if(atom->type == ui->forge.URI)
		{
			const char *str = LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom);

			sprintf(ptr, HIL("val     ", "%s</br></br>"), str);
		}
		else if(atom->type == ui->uris.midi_MidiEvent)
		{
			const uint8_t *midi = LV2_ATOM_BODY_CONST(atom);

			if(midi[0] == 0xf0)
			{
				sprintf(ptr, HIL("val     ", "%s</br></br>"), "Sysex");
			}
			else
			{
				sprintf(ptr, HIL_PRE("val     "));
				ptr += strlen(ptr);

				for(int i=0; (i<atom->size) && (ptr<end); i++, ptr += 3)
					sprintf(ptr, "%02X ", midi[i]);
				
				sprintf(ptr, HIL_POST);
			}
		}

		return strdup(buf);
	}
	else
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
		char buf [1024];
		char *ptr = buf;

		const char *key = ui->unmap->unmap(ui->unmap->handle, prop->key);
		const char *context = ui->unmap->unmap(ui->unmap->handle, prop->context);

		sprintf(ptr, URI("key     ", "%s</br>"), key);
		ptr += strlen(ptr);
		
		sprintf(ptr, URI("context ", "%s</br></br>"), context);
		ptr += strlen(ptr);

		return strdup(buf);
	}
	else
		return NULL;
}

static char * 
_sherlock_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
		return _atom_item_label_get((void *)atom, obj, part);
	else
		return NULL;
}


static Evas_Object * 
_atom_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom *atom = data;
	char buf [512];
	
	if(!ui)
		return NULL;
	
	if(!strcmp(part, "elm.swallow.icon"))
	{
		return NULL;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		sprintf(buf, "<color=#0bb font=Mono>%4u</color>", atom->size);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_size_hint_weight_set(label, 0.5, EVAS_HINT_EXPAND);
		evas_object_show(label);

		return label;
	}
	else
		return NULL;
}

static Evas_Object * 
_prop_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Property_Body *prop = data;
	char buf [512];

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		return NULL;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		return NULL;
	}
	else
		return NULL;
}

static Evas_Object * 
_sherlock_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;
	char buf [512];

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.swallow.icon"))
	{
		sprintf(buf, "<color=#bb0 font=Mono size=10>%4ld</color>", ev->time.frames);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_size_hint_weight_set(label, 0.5, EVAS_HINT_EXPAND);
		evas_object_show(label);

		return label;
	}
	else if(!strcmp(part, "elm.swallow.end"))
	{
		sprintf(buf, "<color=#0bb font=Mono size=10>%4u</color>", atom->size);

		Evas_Object *label = elm_label_add(obj);
		elm_object_part_text_set(label, "default", buf);
		evas_object_size_hint_weight_set(label, 0.5, EVAS_HINT_EXPAND);
		evas_object_show(label);

		return label;
	}
	else
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

#undef LV2_ATOM_TUPLE_FOREACH

#define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))

static void
_atom_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom *atom = data;

	if(  (atom->type == ui->forge.Object)
		|| (atom->type == ui->forge.Resource)
		|| (atom->type == ui->forge.Blank) )
	{
		const LV2_Atom_Object *atom_object = (const LV2_Atom_Object *)atom;
		//const LV2_Atom_Property_Body *prop;

		LV2_ATOM_OBJECT_FOREACH(atom_object, prop)
		{
			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_prop, prop, itm,
				ELM_GENLIST_ITEM_TREE, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_TRUE);
		}
	}
	else if(atom->type == ui->forge.Tuple)
	{
		const LV2_Atom_Tuple *atom_tuple = (const LV2_Atom_Tuple *)atom;
		const LV2_Atom *elmnt;

		LV2_ATOM_TUPLE_FOREACH(atom_tuple, elmnt)
		//for(elmnt = lv2_atom_tuple_begin(atom_tuple);
		//	!lv2_atom_tuple_is_end(LV2_ATOM_BODY(atom_tuple), atom_tuple->atom.size, elmnt);
		//	elmnt = lv2_atom_tuple_next(elmnt))
		{
			Elm_Genlist_Item_Type type = _is_expandable(ui, elmnt->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;
			type = ELM_GENLIST_ITEM_TREE; //XXX

			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_atom, elmnt, itm,
				type, NULL, NULL);
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
		type = ELM_GENLIST_ITEM_TREE; //XXX

		int num = (atom_vector->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ atom_vector->body.child_size;
		const uint8_t *body = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, atom_vector);
		for(int i=0; i<num; i++)
		{
			LV2_Atom *atom = malloc(sizeof(LV2_Atom) + atom_vector->body.child_size);
			atom->size = atom_vector->body.child_size;
			atom->type = atom_vector->body.child_type;
			memcpy(LV2_ATOM_BODY(atom), body + i*atom->size, atom->size);

			Elm_Genlist_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_vec, atom, itm, type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
		}
	}
	else
		; // never reached
}

static void
_prop_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom_Property_Body *prop = data;
	const LV2_Atom *atom = &prop->value;

	Elm_Genlist_Item_Type type = _is_expandable(ui, atom->type)
		? ELM_GENLIST_ITEM_TREE
		: ELM_GENLIST_ITEM_NONE;
	type = ELM_GENLIST_ITEM_TREE; //XXX

	Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_atom, atom, itm, type, NULL, NULL);
	elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
	elm_genlist_item_expanded_set(itm2, EINA_FALSE);
}

static void
_sherlock_expand(UI *ui, const void *data, Evas_Object *obj, Elm_Object_Item *itm)
{
	const LV2_Atom_Event *ev = data;
	const LV2_Atom *atom = &ev->body;

	if(atom)
		_atom_expand(ui, atom, obj, itm);
}

static void
_item_expanded(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *itm = event_info;
	UI *ui = data;

	const Elm_Genlist_Item_Class *class = elm_genlist_item_item_class_get(itm);
	const void *udata = elm_object_item_data_get(itm);

	if(udata)
	{
		if(class  == ui->itc_sherlock)
			_sherlock_expand(ui, udata, obj, itm);
		else if(class == ui->itc_prop)
			_prop_expand(ui, udata, obj, itm);
		else if(class == ui->itc_vec)
			_atom_expand(ui, udata, obj, itm);
		else if(class == ui->itc_atom)
			_atom_expand(ui, udata, obj, itm);
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
_clear_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	elm_genlist_clear(ui->list);
}

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);

	ui->vbox = elm_box_add(eoui->win);
	elm_box_horizontal_set(ui->vbox, EINA_FALSE);
	elm_box_homogeneous_set(ui->vbox, EINA_FALSE);
	elm_box_padding_set(ui->vbox, 0, 10);

	ui->list = elm_genlist_add(ui->vbox);
	elm_genlist_select_mode_set(ui->list, ELM_OBJECT_SELECT_MODE_DEFAULT);
	elm_genlist_homogeneous_set(ui->list, EINA_TRUE); // TRUE for lazy-loading
	//elm_genlist_mode_set(ui->list, ELM_LIST_SCROLL);
	evas_object_data_set(ui->list, "ui", ui);
	//evas_object_smart_callback_add(ui->list, "selected", _item_selected, ui);
	evas_object_smart_callback_add(ui->list, "expand,request",
		_item_expand_request, ui);
	evas_object_smart_callback_add(ui->list, "contract,request",
		_item_contract_request, ui);
	evas_object_smart_callback_add(ui->list, "expanded", _item_expanded, ui);
	evas_object_smart_callback_add(ui->list, "contracted", _item_contracted, ui);
	evas_object_size_hint_weight_set(ui->list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->list);
	elm_box_pack_end(ui->vbox, ui->list);

	ui->clear = elm_button_add(ui->vbox);
	elm_object_part_text_set(ui->clear, "default", "Clear");
	evas_object_smart_callback_add(ui->clear, "clicked", _clear_clicked, ui);
	//evas_object_size_hint_weight_set(ui->clear, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->clear, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->clear);
	elm_box_pack_end(ui->vbox, ui->clear);

	return ui->vbox;
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
	eoui->w = 500,
	eoui->h = 500;

	ui->write_function = write_function;
	ui->controller = controller;
	
	int i, j;
	for(i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			ui->unmap = (LV2_URID_Unmap *)features[i]->data;
  }
	
	ui->uris.midi_MidiEvent = ui->map->map(ui->map->handle, LV2_MIDI__MidiEvent);
	ui->uris.atom_transfer = ui->map->map(ui->map->handle, LV2_ATOM__atomTransfer);
	
	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->itc_sherlock = elm_genlist_item_class_new();
	//ui->itc_sherlock->item_style = "double_label";
	ui->itc_sherlock->item_style = "default_style";
	ui->itc_sherlock->func.text_get = _sherlock_item_label_get;
	ui->itc_sherlock->func.content_get = _sherlock_item_content_get;
	ui->itc_sherlock->func.state_get = NULL;
	ui->itc_sherlock->func.del = _item_del;
	
	ui->itc_prop = elm_genlist_item_class_new();
	//ui->itc_prop->item_style = "double_label";
	ui->itc_prop->item_style = "default_style";
	ui->itc_prop->func.text_get = _prop_item_label_get;
	ui->itc_prop->func.content_get = _prop_item_content_get;
	ui->itc_prop->func.state_get = NULL;
	ui->itc_prop->func.del = NULL;
	
	ui->itc_vec = elm_genlist_item_class_new();
	//ui->itc_vec->item_style = "double_label";
	ui->itc_vec->item_style = "default_style";
	ui->itc_vec->func.text_get = _atom_item_label_get;
	ui->itc_vec->func.content_get = _atom_item_content_get;
	ui->itc_vec->func.state_get = NULL;
	ui->itc_vec->func.del = _item_del;
	
	ui->itc_atom = elm_genlist_item_class_new();
	//ui->itc_atom->item_style = "double_label";
	ui->itc_atom->item_style = "default_style";
	ui->itc_atom->func.text_get = _atom_item_label_get;
	ui->itc_atom->func.content_get = _atom_item_content_get;
	ui->itc_atom->func.state_get = NULL;
	ui->itc_atom->func.del = NULL;

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	eoui_cleanup(&ui->eoui);

	elm_genlist_item_class_free(ui->itc_atom);
	elm_genlist_item_class_free(ui->itc_vec);
	elm_genlist_item_class_free(ui->itc_prop);
	elm_genlist_item_class_free(ui->itc_sherlock);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;

	if( (i == 0) && (urid == ui->uris.atom_transfer) )
	{
		const LV2_Atom_Sequence *seq = buf;

		LV2_ATOM_SEQUENCE_FOREACH(seq, elmnt)
		{
			size_t len = sizeof(LV2_Atom_Event) + elmnt->body.size;
			LV2_Atom_Event *ev = malloc(len);
			memcpy(ev, elmnt, len);

			// check item count 
			if(elm_genlist_items_count(ui->list) >= COUNT_MAX)
				break;
			
			/* TODO would be correct
			const LV2_Atom *atom = &elmnt->body;
			Elm_Genlist_Item_Type type = _is_expandable(ui, atom->type)
				? ELM_GENLIST_ITEM_TREE
				: ELM_GENLIST_ITEM_NONE;
			*/
			Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_TREE; // TODO looks nicer

			Elm_Genlist_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_sherlock, ev, NULL,
				type, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			elm_genlist_item_expanded_set(itm2, EINA_FALSE);
			
			// scroll to last item
			//elm_genlist_item_show(itm, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
		}
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
