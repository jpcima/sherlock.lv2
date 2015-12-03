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
#include <lv2_osc.h>

#define COUNT_MAX 2048 // maximal amount of events shown
#define STRING_BUF_SIZE 2048
#define STRING_MAX 256
#define STRING_OFF (STRING_MAX - 4)

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	
	LV2_URID_Map *map;
	LV2_Atom_Forge forge;
	LV2_URID event_transfer;
	osc_forge_t oforge;

	Evas_Object *table;
	Evas_Object *list;
	Evas_Object *clear;
	Evas_Object *popup;

	Elm_Genlist_Item_Class *itc_osc;

	char string_buf [STRING_BUF_SIZE];
	char *logo_path;
};

#define HIL_PRE(VAL) ("<color=#bbb font=Mono style=plain><b>"VAL"</b></color> <color=#b00 font=Mono style=plain>")
#define HIL_POST ("</color>")

#define URI(VAL,TYP) ("<color=#bbb font=Mono style=plain><b>"VAL"</b></color> <color=#fff style=plain>"TYP"</color>")
#define HIL(VAL,TYP) ("<color=#bbb font=Mono style=plain><b>"VAL"</b></color> <color=#b00 font=Mono style=plain>"TYP"</color>")

static char *
_atom_stringify(UI *ui, const LV2_Atom *atom)
{
	const LV2_Atom_Object *obj = (const LV2_Atom_Object *)atom;

	if(osc_atom_is_message(&ui->oforge, obj))
	{
		const LV2_Atom_String *path = NULL;
		const LV2_Atom_String *fmt = NULL;
		const LV2_Atom_Tuple *tup = NULL;
		osc_atom_message_unpack(&ui->oforge, obj, &path, &fmt, &tup);

		char *osc = NULL;
		asprintf(&osc, " %s ,%s", LV2_ATOM_BODY_CONST(path), LV2_ATOM_BODY_CONST(fmt));
		//FIXME
		return osc;
	}
	else if(osc_atom_is_bundle(&ui->oforge, obj))
	{
		//FIXME
	}

	return NULL;
}

static char *
_osc_label_get(void *data, Evas_Object *obj, const char *part)
{
	UI *ui = evas_object_data_get(obj, "ui");
	const LV2_Atom_Event *ev = data;

	if(!ui)
		return NULL;

	if(!strcmp(part, "elm.text"))
	{
		return _atom_stringify(ui, &ev->body);
	}

	return NULL;
}

static Evas_Object * 
_osc_content_get(void *data, Evas_Object *obj, const char *part)
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
_osc_del(void *data, Evas_Object *obj)
{
	free(data);
}

static void
_clear_update(UI *ui, int count)
{
	if(!ui->clear)
		return;

	char *buf = ui->string_buf;
	sprintf(buf, "Clear (%i of %i)", count, COUNT_MAX);
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
			evas_object_size_hint_weight_set(ui->list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(ui->list, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->list);
			elm_table_pack(ui->table, ui->list, 0, 0, 2, 1);
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

		Evas_Object *info = elm_button_add(ui->table);
		if(info)
		{
			evas_object_smart_callback_add(info, "clicked", _info_clicked, ui);
			evas_object_size_hint_weight_set(info, 0.f, 0.f);
			evas_object_size_hint_align_set(info, 1.f, EVAS_HINT_FILL);
			evas_object_show(info);
			elm_table_pack(ui->table, info, 1, 1, 1, 1);
				
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

	eo_ui_driver_t driver;
	if(descriptor == &osc_inspector_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &osc_inspector_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &osc_inspector_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &osc_inspector_kx)
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
  }

	if(!ui->map)
	{
		fprintf(stderr, "LV2 URID extension not supported\n");
		free(ui);
		return NULL;
	}
	
	ui->event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);
	lv2_atom_forge_init(&ui->forge, ui->map);
	osc_forge_init(&ui->oforge, ui->map);

	ui->itc_osc = elm_genlist_item_class_new();
	if(ui->itc_osc)
	{
		ui->itc_osc->item_style = "default_style";
		ui->itc_osc->func.text_get = _osc_label_get;
		ui->itc_osc->func.content_get = _osc_content_get;
		ui->itc_osc->func.state_get = NULL;
		ui->itc_osc->func.del = _osc_del;
	}

	sprintf(ui->string_buf, "%s/omk_logo_256x256.png", bundle_path);
	ui->logo_path = strdup(ui->string_buf);

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

	if(ui->logo_path)
		free(ui->logo_path);

	if(ui->itc_osc)
		elm_genlist_item_class_free(ui->itc_osc);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	UI *ui = handle;
			
	if( (i == 2) && (urid == ui->event_transfer) && ui->list)
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
			if(n++ >= COUNT_MAX)
				break;
		
			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_osc,
				ev, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			elm_genlist_item_select_mode_set(itm2, ELM_OBJECT_SELECT_MODE_DEFAULT);
			
			// scroll to last item
			//elm_genlist_item_show(itm, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
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
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor osc_inspector_ui = {
	.URI						= SHERLOCK_OSC_INSPECTOR_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor osc_inspector_x11 = {
	.URI						= SHERLOCK_OSC_INSPECTOR_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor osc_inspector_kx = {
	.URI						= SHERLOCK_OSC_INSPECTOR_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
