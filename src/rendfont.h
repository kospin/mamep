/***************************************************************************

    rendfont.h

    Rendering system font management.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __RENDFONT_H__
#define __RENDFONT_H__

#include "render.h"



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

render_font *render_font_alloc(const char *filename);
void render_font_free(render_font *font);
render_texture *render_font_get_char_texture_and_bounds(render_font *font, float height, float aspect, UINT16 ch, render_bounds *bounds);
void render_font_get_scaled_bitmap_and_bounds(render_font *font, mame_bitmap *dest, float height, float aspect, UINT16 chnum, rectangle *bounds);
float render_font_get_char_width(render_font *font, float height, float aspect, UINT16 ch);
float render_font_get_string_width(render_font *font, float height, float aspect, const char *string);
float render_font_get_wstring_width(render_font *font, float height, float aspect, const UINT16 *wstring);

int render_font_get_pixel_height(const render_font *font);

int uifont_need_font_warning(void);
int uifont_decodechar(const UINT8 *s, UINT16 *code);

void convert_command_move(char *buf);
#endif	/* __RENDFONT_H__ */
