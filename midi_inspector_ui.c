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

#include <lv2_eo_ui.h>

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

	Evas_Object *table;
	Evas_Object *list;
	Evas_Object *clear;
	Evas_Object *autoclear;
	Evas_Object *autoblock;
	Evas_Object *popup;

	Elm_Genlist_Item_Class *itc_midi;
	Elm_Genlist_Item_Class *itc_group;

	char string_buf [STRING_BUF_SIZE];
	char *logo_path;
};

typedef struct _midi_msg_t midi_msg_t;

struct _midi_msg_t {
	uint8_t type;
	const char *key;
};

#define COMMANDS_NUM 18
static const midi_msg_t commands [COMMANDS_NUM] = {
	{ LV2_MIDI_MSG_NOTE_OFF							, "NoteOff" },
	{ LV2_MIDI_MSG_NOTE_ON							, "NoteOn" },
	{ LV2_MIDI_MSG_NOTE_PRESSURE				, "NotePressure" },
	{ LV2_MIDI_MSG_CONTROLLER						, "Controller" },
	{ LV2_MIDI_MSG_PGM_CHANGE						, "ProgramChange" },
	{ LV2_MIDI_MSG_CHANNEL_PRESSURE			, "ChannelPressure" },
	{ LV2_MIDI_MSG_BENDER								, "Bender" },
	{ LV2_MIDI_MSG_SYSTEM_EXCLUSIVE			, "SystemExclusive" },
	{ LV2_MIDI_MSG_MTC_QUARTER					, "QuarterFrame" },
	{ LV2_MIDI_MSG_SONG_POS							, "SongPosition" },
	{ LV2_MIDI_MSG_SONG_SELECT					, "SongSelect" },
	{ LV2_MIDI_MSG_TUNE_REQUEST					, "TuneRequest" },
	{ LV2_MIDI_MSG_CLOCK								, "Clock" },
	{ LV2_MIDI_MSG_START								, "Start" },
	{ LV2_MIDI_MSG_CONTINUE							, "Continue" },
	{ LV2_MIDI_MSG_STOP									, "Stop" },
	{ LV2_MIDI_MSG_ACTIVE_SENSE					, "ActiveSense" },
	{ LV2_MIDI_MSG_RESET								, "Reset" },
};

#define CONTROLLERS_NUM 72
static const midi_msg_t controllers [CONTROLLERS_NUM] = {
	{ LV2_MIDI_CTL_MSB_BANK             , "BankSelection_MSB" },
	{ LV2_MIDI_CTL_MSB_MODWHEEL         , "Modulation_MSB" },
	{ LV2_MIDI_CTL_MSB_BREATH           , "Breath_MSB" },
	{ LV2_MIDI_CTL_MSB_FOOT             , "Foot_MSB" },
	{ LV2_MIDI_CTL_MSB_PORTAMENTO_TIME  , "PortamentoTime_MSB" },
	{ LV2_MIDI_CTL_MSB_DATA_ENTRY       , "DataEntry_MSB" },
	{ LV2_MIDI_CTL_MSB_MAIN_VOLUME      , "MainVolume_MSB" },
	{ LV2_MIDI_CTL_MSB_BALANCE          , "Balance_MSB" },
	{ LV2_MIDI_CTL_MSB_PAN              , "Panpot_MSB" },
	{ LV2_MIDI_CTL_MSB_EXPRESSION       , "Expression_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT1          , "Effect1_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT2          , "Effect2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE1 , "GeneralPurpose1_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE2 , "GeneralPurpose2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE3 , "GeneralPurpose3_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE4 , "GeneralPurpose4_MSB" },

	{ LV2_MIDI_CTL_LSB_BANK             , "BankSelection_LSB" },
	{ LV2_MIDI_CTL_LSB_MODWHEEL         , "Modulation_LSB" },
	{ LV2_MIDI_CTL_LSB_BREATH           , "Breath_LSB" },
	{ LV2_MIDI_CTL_LSB_FOOT             , "Foot_LSB" },
	{ LV2_MIDI_CTL_LSB_PORTAMENTO_TIME  , "PortamentoTime_LSB" },
	{ LV2_MIDI_CTL_LSB_DATA_ENTRY       , "DataEntry_LSB" },
	{ LV2_MIDI_CTL_LSB_MAIN_VOLUME      , "MainVolume_LSB" },
	{ LV2_MIDI_CTL_LSB_BALANCE          , "Balance_LSB" },
	{ LV2_MIDI_CTL_LSB_PAN              , "Panpot_LSB" },
	{ LV2_MIDI_CTL_LSB_EXPRESSION       , "Expression_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT1          , "Effect1_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT2          , "Effect2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE1 , "GeneralPurpose1_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE2 , "GeneralPurpose2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE3 , "GeneralPurpose3_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE4 , "GeneralPurpose4_LSB" },

	{ LV2_MIDI_CTL_SUSTAIN              , "SustainPedal" },
	{ LV2_MIDI_CTL_PORTAMENTO           , "Portamento" },
	{ LV2_MIDI_CTL_SOSTENUTO            , "Sostenuto" },
	{ LV2_MIDI_CTL_SOFT_PEDAL           , "SoftPedal" },
	{ LV2_MIDI_CTL_LEGATO_FOOTSWITCH    , "LegatoFootSwitch" },
	{ LV2_MIDI_CTL_HOLD2                , "Hold2" },

	{ LV2_MIDI_CTL_SC1_SOUND_VARIATION  , "SC1_SoundVariation" },
	{ LV2_MIDI_CTL_SC2_TIMBRE           , "SC2_Timbre" },
	{ LV2_MIDI_CTL_SC3_RELEASE_TIME     , "SC3_ReleaseTime" },
	{ LV2_MIDI_CTL_SC4_ATTACK_TIME      , "SC4_AttackTime" },
	{ LV2_MIDI_CTL_SC5_BRIGHTNESS       , "SC5_Brightness" },
	{ LV2_MIDI_CTL_SC6                  , "SC6" },
	{ LV2_MIDI_CTL_SC7                  , "SC7" },
	{ LV2_MIDI_CTL_SC8                  , "SC8" },
	{ LV2_MIDI_CTL_SC9                  , "SC9" },
	{ LV2_MIDI_CTL_SC10                 , "SC10" },

	{ LV2_MIDI_CTL_GENERAL_PURPOSE5     , "GeneralPurpose5" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE6     , "GeneralPurpose6" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE7     , "GeneralPurpose7" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE8     , "GeneralPurpose8" },
	{ LV2_MIDI_CTL_PORTAMENTO_CONTROL   , "PortamentoControl" },

	{ LV2_MIDI_CTL_E1_REVERB_DEPTH      , "E1_ReverbDepth" },
	{ LV2_MIDI_CTL_E2_TREMOLO_DEPTH     , "E2_TremoloDepth" },
	{ LV2_MIDI_CTL_E3_CHORUS_DEPTH      , "E3_ChorusDepth" },
	{ LV2_MIDI_CTL_E4_DETUNE_DEPTH      , "E4_DetuneDepth" },
	{ LV2_MIDI_CTL_E5_PHASER_DEPTH      , "E5_PhaserDepth" },

	{ LV2_MIDI_CTL_DATA_INCREMENT       , "DataIncrement" },
	{ LV2_MIDI_CTL_DATA_DECREMENT       , "DataDecrement" },

	{ LV2_MIDI_CTL_NRPN_LSB             , "NRPN_LSB" },
	{ LV2_MIDI_CTL_NRPN_MSB             , "NRPN_MSB" },

	{ LV2_MIDI_CTL_RPN_LSB              , "RPN_LSB" },
	{ LV2_MIDI_CTL_RPN_MSB              , "RPN_MSB" },

	{ LV2_MIDI_CTL_ALL_SOUNDS_OFF       , "AllSoundsOff" },
	{ LV2_MIDI_CTL_RESET_CONTROLLERS    , "ResetControllers" },
	{ LV2_MIDI_CTL_LOCAL_CONTROL_SWITCH , "LocalControlSwitch" },
	{ LV2_MIDI_CTL_ALL_NOTES_OFF        , "AllNotesOff" },
	{ LV2_MIDI_CTL_OMNI_OFF             , "OmniOff" },
	{ LV2_MIDI_CTL_OMNI_ON              , "OmniOn" },
	{ LV2_MIDI_CTL_MONO1                , "Mono1" },
	{ LV2_MIDI_CTL_MONO2                , "Mono2" },
};

static int
_cmp_search(const void *itm1, const void *itm2)
{
	const midi_msg_t *msg1 = itm1;
	const midi_msg_t *msg2 = itm2;

	if(msg1->type < msg2->type)
		return -1;
	else if(msg1->type > msg2->type)
		return 1;

	return 0;
}

static inline const midi_msg_t *
_search_command(uint8_t type)
{
	return bsearch(&type, commands, COMMANDS_NUM, sizeof(midi_msg_t), _cmp_search);
}

static inline const midi_msg_t *
_search_controller(uint8_t type)
{
	return bsearch(&type, controllers, CONTROLLERS_NUM, sizeof(midi_msg_t), _cmp_search);
}

#define CODE_PRE "<font=Mono style=shadow,bottom>"
#define CODE_POST "</font>"

#define RAW_PRE "<color=#b0b><b>"
#define RAW_POST "</b></color>"

#define SYSTEM(TYP) "<color=#888><b>"TYP"</b></color>"
#define CHANNEL(TYP, VAL) "<color=#888><b>"TYP"</b></color><color=#fff>"VAL"</color>"
#define COMMAND(VAL) "<color=#0b0>"VAL"</color>"
#define CONTROLLER(VAL) "<color=#b00>"VAL"</color>"
#define PUNKT(VAL) "<color=#00b>"VAL"</color>"

static const char *keys [12] = {
	"C", "#C",
	"D", "#D",
	"E",
	"F", "#F",
	"G", "#G",
	"A", "#A",
	"H"
};

static inline const char *
_note(uint8_t val, uint8_t *octave)
{
	*octave = val / 12;

	return keys[val % 12];
}

static inline char *
_atom_stringify(UI *ui, char *ptr, char *end, const LV2_Atom *atom)
{
	//FIXME check for buffer overflows!!!

	const uint8_t *midi = LV2_ATOM_BODY_CONST(atom);

	sprintf(ptr, CODE_PRE RAW_PRE);
	ptr += strlen(ptr);

	char *barrier = ptr + STRING_OFF;
	unsigned i;
	for(i=0; (i<atom->size) && (ptr<barrier); i++, ptr += 3)
		sprintf(ptr, " %02"PRIX8, midi[i]);

	for( ; (i<3) && (ptr<barrier); i++, ptr += 3)
		sprintf(ptr, "   ");

	sprintf(ptr, RAW_POST);
	ptr += strlen(ptr);

	if( (midi[0] & 0xf0) == 0xf0) // system messages
	{
		const midi_msg_t *command_msg = _search_command(midi[0]); 
		const char *command_str = command_msg
			? command_msg->key
			: "Unknown";

		if(midi[0] == LV2_MIDI_MSG_SYSTEM_EXCLUSIVE) // sysex message
		{
			for( ; (i<atom->size) && ptr<barrier; i++, ptr += 3)
				sprintf(ptr, " %02"PRIX8, midi[i]);
		}
		else if(midi[0] == LV2_MIDI_MSG_SONG_POS)
		{
			uint16_t pos = (((uint16_t)midi[2] << 7) | midi[1]);
			sprintf(ptr,
				PUNKT(" [") SYSTEM("Syst.")
				PUNKT(", ") COMMAND("%s")
				PUNKT(", ") "%"PRIu16
				PUNKT("]"),
				command_str, pos);
		}
		else // other system message
		{
			if(atom->size == 2)
			{
				sprintf(ptr,
					PUNKT(" [") SYSTEM("Syst.")
					PUNKT(", ") COMMAND("%s")
					PUNKT(", ") "%"PRIi8
					PUNKT("]"),
					command_str, midi[1]);
			}
			else if(atom->size == 3)
			{
				sprintf(ptr,
					PUNKT(" [") SYSTEM("Syst.")
					PUNKT(", ") COMMAND("%s")
					PUNKT(", ") "%"PRIi8
					PUNKT(", ") "%"PRIi8
					PUNKT("]"),
					command_str, midi[1], midi[2]);
			}
			else // assume atom->size == 1, aka no data
			{
				sprintf(ptr,
					PUNKT(" [") SYSTEM("Syst.")
					PUNKT(", ") COMMAND("%s")
					PUNKT("]"),
					command_str);
			}
		}
	}
	else // channel messages
	{
		const uint8_t command = midi[0] & 0xf0;
		const uint8_t channel = midi[0] & 0x0f;

		const midi_msg_t *command_msg = _search_command(command); 
		const char *command_str = command_msg
			? command_msg->key
			: "Unknown";

		if( (command == LV2_MIDI_MSG_NOTE_ON)
			|| (command == LV2_MIDI_MSG_NOTE_OFF)
			|| (command == LV2_MIDI_MSG_NOTE_PRESSURE))
		{
			uint8_t octave;
			const char *note = _note(midi[1], &octave);
			sprintf(ptr,
				PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
				PUNKT(", ") COMMAND("%s")
				PUNKT(", ") "%s-%"PRIu8
				PUNKT(", ") "%"PRIi8
				PUNKT("]"),
				channel, command_str, note, octave, midi[2]);
		}
		else if(command == LV2_MIDI_MSG_CONTROLLER)
		{
			const midi_msg_t *controller_msg = _search_controller(midi[1]);
			const char *controller_str = controller_msg
				? controller_msg->key
				: "Unknown";

			if(atom->size == 3)
			{
				sprintf(ptr,
					PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
					PUNKT(", ") COMMAND("%s")
					PUNKT(", ") CONTROLLER("%s")
					PUNKT(", ") "%"PRIi8 
					PUNKT("]"),
					channel, command_str, controller_str, midi[2]);
			}
			else if(atom->size == 2)
			{
				sprintf(ptr,
					PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
					PUNKT(", ") COMMAND("%s")
					PUNKT(", ") CONTROLLER("%s")
					PUNKT("]"),
					channel, command_str, controller_str);
			}
		}
		else if(command == LV2_MIDI_MSG_BENDER)
		{
			int16_t bender = (((int16_t)midi[2] << 7) | midi[1]) - 0x2000;
			sprintf(ptr,
				PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
				PUNKT(", ") COMMAND("%s")
				PUNKT(", ") "%"PRIi16
				PUNKT("]"),
				channel, command_str, bender);
		}
		else if(atom->size == 3)
		{
			sprintf(ptr,
				PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
				PUNKT(", ") COMMAND("%s")
				PUNKT(", ") "%"PRIi8
				PUNKT(", ") "%"PRIi8
				PUNKT("]"),
				channel, command_str, midi[1], midi[2]);
		}
		else if(atom->size == 2)
		{
			sprintf(ptr,
				PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
				PUNKT(", ") COMMAND("%s")
				PUNKT(", ") "%"PRIi8
				PUNKT("]"),
				channel, command_str, midi[1]);
		}
		else // fall-back
		{
			sprintf(ptr,
				PUNKT(" [") CHANNEL("Ch ", "%02"PRIX8)
				PUNKT(", ") COMMAND("%s")
				PUNKT("]"),
				channel, command_str);
		}
	}
	ptr += strlen(ptr);

	if(ptr >= barrier) // there would be more to print
	{
		ptr = barrier;
		sprintf(ptr, "...");
		ptr += 4;
	}

	sprintf(ptr, CODE_POST);

	return ptr + strlen(ptr);
}

static char *
_midi_label_get(void *data, Evas_Object *obj, const char *part)
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
_midi_content_get(void *data, Evas_Object *obj, const char *part)
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
						"Sherlock - MIDI Inspector"
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
	if(strcmp(plugin_uri, SHERLOCK_MIDI_INSPECTOR_URI))
		return NULL;

	eo_ui_driver_t driver;
	if(descriptor == &midi_inspector_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &midi_inspector_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &midi_inspector_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &midi_inspector_kx)
		driver = EO_UI_DRIVER_KX;
	else
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 600,
	eoui->h = 800;

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

	ui->itc_midi = elm_genlist_item_class_new();
	if(ui->itc_midi)
	{
		ui->itc_midi->item_style = "default_style";
		ui->itc_midi->func.text_get = _midi_label_get;
		ui->itc_midi->func.content_get = _midi_content_get;
		ui->itc_midi->func.state_get = NULL;
		ui->itc_midi->func.del = _del;
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

	if(ui->itc_midi)
		elm_genlist_item_class_free(ui->itc_midi);

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
			if(pos)
			{
				pos->offset = offset->body;
				pos->nsamples = nsamples->body;

				itm = elm_genlist_item_append(ui->list, ui->itc_group,
					pos, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
				elm_genlist_item_select_mode_set(itm, ELM_OBJECT_SELECT_MODE_NONE);
			}
		}

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
		
			Elm_Object_Item *itm2 = elm_genlist_item_append(ui->list, ui->itc_midi,
				ev, itm, ELM_GENLIST_ITEM_NONE, NULL, NULL);
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

const LV2UI_Descriptor midi_inspector_eo = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor midi_inspector_ui = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor midi_inspector_x11 = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor midi_inspector_kx = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
