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

%{
#include <stdio.h>
#include <string.h>

#include <common.h>

typedef enum _markup_type_t markup_type_t;
typedef struct _markup_item_t markup_item_t;

enum _markup_type_t {
	MARKUP_CODE,
	MARKUP_PREFIX,
	MARKUP_SUBJECT,
	MARKUP_PREDICATE,
	MARKUP_NUMBER,
	MARKUP_STRING,
	MARKUP_URI
};

struct _markup_item_t {
	const char *begin;
	const char *end;
};

static const markup_item_t markup_items [] = {
	[MARKUP_CODE] =				{"font=Mono style=Plain color=#ffffff", "font"},
	[MARKUP_PREFIX] =			{"color=#cc00cc", "color"},
	[MARKUP_SUBJECT] =		{"color=#00cccc", "color"},
	[MARKUP_PREDICATE] =	{"color=#00cc00", "color"},
	[MARKUP_NUMBER] =			{"color=#0000cc", "color"},
	[MARKUP_STRING] =			{"color=#cc0000", "color"},
	[MARKUP_URI] =				{"color=#cccc00", "color"}
};

static void
_add_plain(const char *content)
{
	encoder->append(content, encoder->data);
}

static void 
_add_singleton(const char *key)
{
	char buf [64];
	sprintf(buf, "<%s/>", key);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_begin(markup_type_t type)
{
	char buf [64];
	sprintf(buf, "<%s>", markup_items[type].begin);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_end(markup_type_t type)
{
	char buf [64];
	sprintf(buf, "</%s>", markup_items[type].end);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup(markup_type_t type, const char *content)
{
	char buf [64];
	sprintf(buf, "<%s>", markup_items[type].begin);
	encoder->append(buf, encoder->data);
	encoder->append(content, encoder->data);
	sprintf(buf, "</%s>", markup_items[type].end);
	encoder->append(buf, encoder->data);
}

enum {
	TK_NONE,
	TK_PREFIX,
	TK_SUBJECT,
	TK_PREDICATE,
	TK_NUMBER,
	TK_URI_IN,
	TK_URI_OUT,
	TK_URI_ERR,
	TK_STRING_IN,
	TK_STRING_OUT,
	TK_STRING_ERR,
	TK_WHITESPACE,
	TK_RAW,
	TK_TAB,
	TK_NEWLINE,
	TK_NAME,
	TK_BADCHAR
};

%}

%option reentrant noyywrap

w              [ \v\a]+
name           [_a-zA-Z@][_a-zA-Z0-9\.]*
n              [0-9]+
exp            [Ee][+-]?{n}
number         ({n}|{n}[.]{n}){exp}?
eol            [\n\r]

%x XSTRING
%x XURI

%%

{w}            return TK_WHITESPACE;
"\t"           return TK_TAB;
{eol}          return TK_NEWLINE;
"<"            BEGIN(XURI); return TK_URI_IN;
\"             BEGIN(XSTRING); return TK_STRING_IN;
{name}:        return TK_SUBJECT;
"@prefix"      return TK_PREFIX;
"a"            return TK_PREFIX;
{name}         return TK_PREDICATE;
{number}       return TK_NUMBER;
.              return TK_RAW;

<XURI>
{
   ">"         BEGIN(0); return TK_URI_OUT;
   {eol}	     BEGIN(0); return TK_URI_ERR;
   .           return TK_RAW;
}

<XSTRING>
{
   \\\"	       return TK_RAW;
   \"          BEGIN(0); return TK_STRING_OUT;
   {eol}	     BEGIN(0); return TK_STRING_ERR;
   .           return TK_RAW;
}

%%

void
ttl_to_markup(const char *utf8, FILE *f)
{
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

	enclex_init(&scanner);
	if(utf8)
	{
		buf = enc_scan_string(utf8, scanner);
	}
	else if(f)
	{
		encset_in(f, scanner);
		buf = enc_create_buffer(NULL, YY_BUF_SIZE, scanner);
	}
	else
	{
		enclex_destroy(scanner);
		return;
	}
				
	encoder->begin(encoder->data);
	_add_markup_begin(MARKUP_CODE);

	for(int tok=enclex(scanner); tok; tok=enclex(scanner))
	{
		const char *txt = encget_text(scanner);
		switch(tok)
		{
			case TK_PREFIX:
				_add_markup(MARKUP_PREFIX, txt);
				break;

			case TK_NUMBER:
				_add_markup(MARKUP_NUMBER, txt);
				break;

			case TK_URI_IN:
				_add_markup_begin(MARKUP_URI);
				_add_plain("&lt;");
				break;
			case TK_URI_OUT:
				_add_plain("&gt;");
				_add_markup_end(MARKUP_URI);
				break;
			case TK_URI_ERR:
				_add_markup_end(MARKUP_URI);
				_add_singleton("br");
				break;

			case TK_STRING_IN:
				_add_markup_begin(MARKUP_STRING);
				_add_plain("\"");
				break;
			case TK_STRING_OUT:
				_add_plain("\"");
				_add_markup_end(MARKUP_STRING);
				break;
			case TK_STRING_ERR:
				_add_markup_end(MARKUP_STRING);
				_add_singleton("br");
				break;

			case TK_NEWLINE:
				_add_singleton("br");
				break;
			
			case TK_BADCHAR:
				break;
			
			case TK_TAB:
				_add_plain("  ");
				break;

			case TK_SUBJECT:
			  _add_markup(MARKUP_SUBJECT, txt);
				break;
			case TK_PREDICATE:
			  _add_markup(MARKUP_PREDICATE, txt);
				break;

			case TK_NAME:
			case TK_WHITESPACE:
			case TK_RAW:
			default:
				_add_plain(txt);
				break;
		}
	}
				
	_add_markup_end(MARKUP_CODE);
	encoder->end(encoder->data);

	enc_delete_buffer(buf, scanner);
	enclex_destroy(scanner);
}
