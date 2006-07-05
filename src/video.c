/***************************************************************************

    video.c

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "png.h"
#include "debugger.h"
#include "vidhrdw/vector.h"
#ifdef USE_SCALE_EFFECTS
#include "osdscale.h"
#endif /* USE_SCALE_EFFECTS */

#ifdef NEW_RENDER
#include "render.h"
#else
#include "artwork.h"
#endif

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
#include "mamedbg.h"
#endif


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG_PARTIAL_UPDATES(x)		/* logerror x */

#define FRAMES_PER_FPS_UPDATE		12

/* VERY IMPORTANT: bitmap_alloc must allocate also a "safety area" 16 pixels wide all
   around the bitmap. This is required because, for performance reasons, some graphic
   routines don't clip at boundaries of the bitmap. */
#define BITMAP_SAFETY				16



/***************************************************************************
    GLOBALS
***************************************************************************/

/* handy globals for other parts of the system */
int vector_updates = 0;

/* main bitmap to render to */
#ifdef NEW_RENDER
static int skipping_this_frame;
static render_texture *scrtexture[MAX_SCREENS];
static int scrformat[MAX_SCREENS];
static int scrchanged[MAX_SCREENS];
static
#endif
mame_bitmap *scrbitmap[MAX_SCREENS][2];
static int curbitmap[MAX_SCREENS];

#ifdef USE_SCALE_EFFECTS
mame_bitmap *scalebitmap[MAX_SCREENS][2];
mame_bitmap *workbitmap[MAX_SCREENS][2];
static int scale_xsize;
static int scale_ysize;
static int scale_depth;
static int use_workbitmap;
#endif /* USE_SCALE_EFFECTS */

/* the active video display */
#ifndef NEW_RENDER
static mame_display current_display;
static UINT8 visible_area_changed;
static UINT8 refresh_rate_changed;
static UINT8 full_refresh_pending;
#endif

/* video updating */
static int last_partial_scanline[MAX_SCREENS];

/* speed computation */
static cycles_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static performance_info performance;

/* movie file */
static mame_file *movie_file = NULL;
static int movie_frame = 0;

/* misc other statics */
static UINT32 leds_status;

/* artwork callbacks */
#ifndef NEW_RENDER
#ifndef MESS
static artwork_callbacks mame_artwork_callbacks =
{
	NULL,
	artwork_load_artwork_file
};
#endif
#endif



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void video_pause(int pause);
static void video_exit(void);
static int allocate_graphics(const gfx_decode *gfxdecodeinfo);
static void decode_graphics(const gfx_decode *gfxdecodeinfo);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);
static void recompute_fps(int skipped_it);
#ifdef USE_SCALE_EFFECTS
static void alloc_scalebitmap(void);
static void free_scalebitmap(void);
static void texture_set_scalebitmap(int scrnum, int curbank, rectangle *visarea);
#endif /* USE_SCALE_EFFECTS */



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    video_init - start up the video system
-------------------------------------------------*/

int video_init(void)
{
	movie_file = NULL;
	movie_frame = 0;

#ifndef NEW_RENDER
	add_pause_callback(video_pause);
#endif
	add_exit_callback(video_exit);

	/* first allocate the necessary palette structures */
	if (palette_start())
		return 1;

#ifndef NEW_RENDER
{
	int bmwidth = Machine->drv->screen[0].maxwidth;
	int bmheight = Machine->drv->screen[0].maxheight;
	artwork_callbacks *artcallbacks;
	osd_create_params params;

	/* if we're a vector game, override the screen width and height */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* compute the visible area for raster games */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		params.width = Machine->drv->screen[0].default_visible_area.max_x - Machine->drv->screen[0].default_visible_area.min_x + 1;
		params.height = Machine->drv->screen[0].default_visible_area.max_y - Machine->drv->screen[0].default_visible_area.min_y + 1;
	}
	else
	{
		params.width = bmwidth;
		params.height = bmheight;
	}

	/* fill in the rest of the display parameters */
	params.aspect_x = 1333;
	params.aspect_y = 1000;
	params.depth = Machine->color_depth;
	params.colors = palette_get_total_colors_with_ui();
	params.fps = Machine->drv->screen[0].refresh_rate;
	params.video_attributes = Machine->drv->video_attributes;

#ifdef MESS
	artcallbacks = &mess_artwork_callbacks;
#else
	artcallbacks = &mame_artwork_callbacks;
#endif

	/* initialize the display through the artwork (and eventually the OSD) layer */
	if (artwork_create_display(&params, direct_rgb_components, artcallbacks))
		return 1;

	/* the create display process may update the vector width/height, so recompute */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* now allocate the screen bitmap */
	scrbitmap[0][0] = auto_bitmap_alloc_depth(bmwidth, bmheight, Machine->color_depth);
	if (!scrbitmap[0][0])
		return 1;

	/* set the default refresh rate */
	set_refresh_rate(0, Machine->drv->screen[0].refresh_rate);

	/* set the default visible area */
	set_visible_area(0, 0,1,0,1);	// make sure everything is recalculated on multiple runs
	set_visible_area(0,
			Machine->drv->screen[0].default_visible_area.min_x,
			Machine->drv->screen[0].default_visible_area.max_x,
			Machine->drv->screen[0].default_visible_area.min_y,
			Machine->drv->screen[0].default_visible_area.max_y);
}
#else
{
	int scrnum;

	/* loop over screens and allocate bitmaps */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			/* allocate bitmaps */
			scrbitmap[scrnum][0] = auto_bitmap_alloc_depth(Machine->drv->screen[scrnum].maxwidth, Machine->drv->screen[scrnum].maxheight, Machine->color_depth);
			scrbitmap[scrnum][1] = auto_bitmap_alloc_depth(Machine->drv->screen[scrnum].maxwidth, Machine->drv->screen[scrnum].maxheight, Machine->color_depth);

			/* choose the texture format */
			if (Machine->color_depth == 16)
				scrformat[scrnum] = TEXFORMAT_PALETTE16;
			else if (Machine->color_depth == 15)
				scrformat[scrnum] = TEXFORMAT_RGB15;
			else
				scrformat[scrnum] = TEXFORMAT_RGB32;

			/* allocate a texture per screen */
			scrtexture[scrnum] = render_texture_alloc(scrbitmap[scrnum][0], &Machine->visible_area[scrnum], &adjusted_palette[Machine->drv->screen[scrnum].palette_base], scrformat[scrnum], NULL, NULL);

			/* set the default refresh rate */
			set_refresh_rate(scrnum, Machine->drv->screen[scrnum].refresh_rate);

			/* set the default visible area */
			set_visible_area(scrnum, 0,1,0,1);	// make sure everything is recalculated on multiple runs
			set_visible_area(scrnum,
					Machine->drv->screen[scrnum].default_visible_area.min_x,
					Machine->drv->screen[scrnum].default_visible_area.max_x,
					Machine->drv->screen[scrnum].default_visible_area.min_y,
					Machine->drv->screen[scrnum].default_visible_area.max_y);
		}
}
#endif

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			return 1;

#ifndef NEW_RENDER
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* if the debugger is enabled, initialize its bitmap and font */
	if (Machine->debug_mode)
	{
		int depth = options.debug_depth ? options.debug_depth : Machine->color_depth;

		/* first allocate the debugger bitmap */
		Machine->debug_bitmap = auto_bitmap_alloc_depth(options.debug_width, options.debug_height, depth);
		if (!Machine->debug_bitmap)
			return 1;

		/* then create the debugger font */
		Machine->debugger_font = build_debugger_font();
		if (Machine->debugger_font == NULL)
			return 1;
	}
#endif
#endif

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo)
		if (allocate_graphics(Machine->drv->gfxdecodeinfo))
			return 1;

	/* initialize the palette - must be done after osd_create_display() */
	if (palette_init())
		return 1;

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* actually decode the graphics */
	if (Machine->drv->gfxdecodeinfo)
		decode_graphics(Machine->drv->gfxdecodeinfo);

	/* reset performance data */
	last_fps_time = osd_cycles();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = Machine->refresh_rate[0];
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	/* initialize tilemaps */
	if (tilemap_init() != 0)
		fatalerror("tilemap_init failed");

#ifdef USE_SCALE_EFFECTS
	use_workbitmap = (scrformat[0] == TEXFORMAT_PALETTE16);
	scale_depth = (scrformat[0] == TEXFORMAT_RGB15) ? 15 : 32;

	if (scale_init())
	{
		logerror("WARNING: scale_effect \"%s\" has any problem\n", scale_effect.name);
		logerror("WARNING: scale effect is disabled\n");
		scale_effect.effect = 0;
	}

	if (scale_effect.effect)
	{
		if (scale_check(scale_depth))
		{
			logerror("WARNING: scale_effect \"%s\" does not support depth %d\n", scale_effect.name, scale_depth);
			use_workbitmap = 1;
			scale_depth = (scale_depth == 15) ? 32 : 15;
			if (scale_check(scale_depth))
			{
				logerror("WARNING: scale_effect \"%s\" does not support both depth 15 and 32\n", scale_effect.name);
				logerror("WARNING: scale effect is disabled\n");

				scale_exit();
				scale_effect.effect = 0;
				scale_init();
			}
		}
	}
#endif /* USE_SCALE_EFFECTS */

	return 0;
}


/*-------------------------------------------------
    video_pause - pause the video system
-------------------------------------------------*/

static void video_pause(int pause)
{
	palette_set_global_brightness_adjust(pause ? options.pause_bright : 1.00);
	schedule_full_refresh();
}


/*-------------------------------------------------
    video_exit - close down the video system
-------------------------------------------------*/

static void video_exit(void)
{
	int i;

	/* stop recording any movie */
	record_movie_stop();

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

#ifndef NEW_RENDER
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* free the font elements */
	if (Machine->debugger_font)
	{
		freegfx(Machine->debugger_font);
		Machine->debugger_font = NULL;
	}
#endif

	/* close down the OSD layer's display */
	osd_close_display();
#else
{
	int scrnum;

	/* free all the render textures */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL && scrtexture[scrnum] != NULL)
			render_texture_free(scrtexture[scrnum]);

#ifdef USE_SCALE_EFFECTS
	free_scalebitmap();
	scale_exit();
#endif /* USE_SCALE_EFFECTS */
}
#endif
}


/*-------------------------------------------------
    allocate_graphics - allocate memory for the
    graphics
-------------------------------------------------*/

static int allocate_graphics(const gfx_decode *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT32 extxoffs[MAX_ABS_GFX_SIZE], extyoffs[MAX_ABS_GFX_SIZE];
		gfx_layout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;
		if (glcopy.extxoffs)
		{
			memcpy(extxoffs, glcopy.extxoffs, glcopy.width * sizeof(extxoffs[0]));
			glcopy.extxoffs = extxoffs;
		}
		if (glcopy.extyoffs)
		{
			memcpy(extyoffs, glcopy.extyoffs, glcopy.height * sizeof(extyoffs[0]));
			glcopy.extyoffs = extyoffs;
		}

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* for non-raw graphics, decode the X and Y offsets */
		if (glcopy.planeoffset[0] != GFX_RAW)
		{
			UINT32 *xoffset = glcopy.extxoffs ? extxoffs : glcopy.xoffset;
			UINT32 *yoffset = glcopy.extyoffs ? extyoffs : glcopy.yoffset;

			/* loop over all the planes, converting fractions */
			for (j = 0; j < glcopy.planes; j++)
			{
				UINT32 value = glcopy.planeoffset[j];
				if (IS_FRAC(value))
					glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			/* loop over all the X/Y offsets, converting fractions */
			for (j = 0; j < glcopy.width; j++)
			{
				UINT32 value = xoffset[j];
				if (IS_FRAC(value))
					xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			for (j = 0; j < glcopy.height; j++)
			{
				UINT32 value = yoffset[j];
				if (IS_FRAC(value))
					yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}
		}

		/* otherwise, just use yoffset[0] as the line modulo */
		else
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * glcopy.yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* allocate the graphics */
		Machine->gfx[i] = allocgfx(&glcopy);

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}


/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static void decode_graphics(const gfx_decode *gfxdecodeinfo)
{
	int totalgfx = 0, curgfx = 0;
	int i;

	/* count total graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
			totalgfx += Machine->gfx[i]->total_elements;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
		{
			/* if we have a valid region, decode it now */
			if (gfxdecodeinfo[i].memory_region > REGION_INVALID)
			{
				UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
				gfx_element *gfx = Machine->gfx[i];
				int j;

				/* now decode the actual graphics */
				for (j = 0; j < gfx->total_elements; j += 1024)
				{
					int num_to_decode = (j + 1024 < gfx->total_elements) ? 1024 : (gfx->total_elements - j);
					decodegfx(gfx, region_base + gfxdecodeinfo[i].start, j, num_to_decode);
					curgfx += num_to_decode;
		/*          ui_display_decoding(artwork_get_ui_bitmap(), curgfx * 100 / totalgfx);*/
				}
			}

			/* otherwise, clear the target region */
			else
				memset(Machine->gfx[i]->gfxdata, 0, Machine->gfx[i]->char_modulo * Machine->gfx[i]->total_elements);
		}
}


#ifndef NEW_RENDER
/*-------------------------------------------------
    scale_vectorgames - scale the vector games
    to a given resolution
-------------------------------------------------*/

static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height)
{
	double x_scale, y_scale, scale;

	/* compute the scale values */
	x_scale = (double)gfx_width  / *width;
	y_scale = (double)gfx_height / *height;

	/* pick the smaller scale factor */
	scale = (x_scale < y_scale) ? x_scale : y_scale;

	/* compute the new size */
	*width  = *width  * scale + 0.5;
	*height = *height * scale + 0.5;

	/* round to the nearest 4 pixel value */
	*width  &= ~3;
	*height &= ~3;
}
#endif


/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		logerror("video_init():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);

	/* register for saving it */
	state_save_register_global_pointer(buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);

		/* register for saving it */
		state_save_register_global_pointer(buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (UINT16 *)buffered_spriteram;
	buffered_spriteram32 = (UINT32 *)buffered_spriteram;
	buffered_spriteram16_2 = (UINT16 *)buffered_spriteram_2;
	buffered_spriteram32_2 = (UINT32 *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************

    Screen rendering and management.

***************************************************************************/

/*-------------------------------------------------
    set_visible_area - adjusts the visible portion
    of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int scrnum, int min_x, int max_x, int min_y, int max_y)
{
#ifndef NEW_RENDER
	if (       Machine->visible_area[0].min_x == min_x
			&& Machine->visible_area[0].max_x == max_x
			&& Machine->visible_area[0].min_y == min_y
			&& Machine->visible_area[0].max_y == max_y)
		return;

	/* "dirty" the area for the next display update */
	visible_area_changed = 1;

	/* bounds check */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) && scrbitmap[0][0])
		if ((min_x < 0) || (min_y < 0) || (max_x >= scrbitmap[0][0]->width) || (max_y >= scrbitmap[0][0]->height))
		{
			fatalerror("set_visible_area(%d,%d,%d,%d) out of bounds; bitmap dimensions are (%d,%d)",
				min_x, min_y, max_x, max_y,
				scrbitmap[0][0]->width, scrbitmap[0][0]->height);
		}

	/* set the new values in the Machine struct */
	Machine->visible_area[0].min_x = min_x;
	Machine->visible_area[0].max_x = max_x;
	Machine->visible_area[0].min_y = min_y;
	Machine->visible_area[0].max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		Machine->absolute_visible_area.min_x = 0;
		Machine->absolute_visible_area.max_x = scrbitmap[0][0]->width - 1;
		Machine->absolute_visible_area.min_y = 0;
		Machine->absolute_visible_area.max_y = scrbitmap[0][0]->height - 1;
	}

	/* raster games need to use the visible area */
	else
		Machine->absolute_visible_area = Machine->visible_area[0];

	/* recompute scanline timing */
	cpu_compute_scanline_timing();

	/* set UI visible area */
	ui_set_visible_area(Machine->absolute_visible_area.min_x,
						Machine->absolute_visible_area.min_y,
						Machine->absolute_visible_area.max_x,
						Machine->absolute_visible_area.max_y);
#else
	if (       Machine->visible_area[scrnum].min_x == min_x
			&& Machine->visible_area[scrnum].max_x == max_x
			&& Machine->visible_area[scrnum].min_y == min_y
			&& Machine->visible_area[scrnum].max_y == max_y)
		return;

	/* bounds check */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) && scrbitmap[scrnum][0])
		if ((min_x < 0) || (min_y < 0) || (max_x >= scrbitmap[scrnum][0]->width) || (max_y >= scrbitmap[scrnum][0]->height))
		{
			fatalerror("set_visible_area(%d,%d,%d,%d) out of bounds; bitmap dimensions are (%d,%d)",
				min_x, min_y, max_x, max_y,
				scrbitmap[scrnum][0]->width, scrbitmap[scrnum][0]->height);
		}

	/* set the new values in the Machine struct */
	Machine->visible_area[scrnum].min_x = min_x;
	Machine->visible_area[scrnum].max_x = max_x;
	Machine->visible_area[scrnum].min_y = min_y;
	Machine->visible_area[scrnum].max_y = max_y;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
#endif
}


/*-------------------------------------------------
    set_refresh_rate - adjusts the refresh rate
    of the video mode dynamically
-------------------------------------------------*/

void set_refresh_rate(int scrnum, float fps)
{
	/* bail if already equal */
	if (Machine->refresh_rate[scrnum] == fps)
		return;

#ifndef NEW_RENDER
	/* "dirty" the rate for the next display update */
	refresh_rate_changed = 1;
#endif

	/* set the new values in the Machine struct */
	Machine->refresh_rate[scrnum] = fps;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}


/*-------------------------------------------------
    schedule_full_refresh - force a full erase
    and refresh the next frame
-------------------------------------------------*/

void schedule_full_refresh(void)
{
#ifndef NEW_RENDER
	full_refresh_pending = 1;
#endif
}


/*-------------------------------------------------
    force_partial_update - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scrnum, int scanline)
{
	rectangle clip = Machine->visible_area[scrnum];

	LOG_PARTIAL_UPDATES(("Partial: force_partial_update(%d,%d): ", scrnum, scanline));

	/* if skipping this frame, bail */
#ifndef NEW_RENDER
	if (osd_skip_this_frame())
#else
	if (skipping_this_frame)
#endif
	{
		LOG_PARTIAL_UPDATES(("skipped due to frameskipping\n"));
		return;
	}

	/* skip if less than the lowest so far */
	if (scanline < last_partial_scanline[scrnum])
	{
		LOG_PARTIAL_UPDATES(("skipped because less than previous\n"));
		return;
	}

#ifdef NEW_RENDER
	/* skip if this screen is not visible anywhere */
	if (!(render_get_live_screens_mask() & (1 << scrnum)))
	{
		LOG_PARTIAL_UPDATES(("skipped because screen not live\n"));
		return;
	}
#endif

#ifndef NEW_RENDER
	/* if there's a dirty bitmap and we didn't do any partial updates yet, handle it now */
	if (full_refresh_pending && last_partial_scanline[scrnum] == 0)
	{
		fillbitmap(scrbitmap[0][curbitmap[0]], get_black_pen(), NULL);
		full_refresh_pending = 0;
	}
#endif

	/* set the start/end scanlines */
	if (last_partial_scanline[scrnum] > clip.min_y)
		clip.min_y = last_partial_scanline[scrnum];
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		UINT32 flags;

		profiler_mark(PROFILER_VIDEO);
		LOG_PARTIAL_UPDATES(("updating %d-%d\n", clip.min_y, clip.max_y));
		flags = (*Machine->drv->video_update)(scrnum, scrbitmap[scrnum][curbitmap[scrnum]], &clip);
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);

#ifdef NEW_RENDER
		/* if we modified the bitmap, we have to commit */
		scrchanged[scrnum] |= (~flags & UPDATE_HAS_NOT_CHANGED);
#endif
	}

	/* remember where we left off */
	last_partial_scanline[scrnum] = scanline + 1;
}


/*-------------------------------------------------
    reset_partial_updates - reset partial updates
    at the start of each frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	/* reset partial updates */
	LOG_PARTIAL_UPDATES(("Partial: reset to 0\n"));
	memset(last_partial_scanline, 0, sizeof(last_partial_scanline));
	performance.partial_updates_this_frame = 0;
}


/*-------------------------------------------------
    update_video_and_audio - actually call the
    OSD layer to perform an update
-------------------------------------------------*/

void update_video_and_audio(void)
{
#ifndef NEW_RENDER
	int skipped_it = osd_skip_this_frame();
#else
	int skipped_it = skipping_this_frame;
#endif

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	debug_trace_delay = 0;
#endif

#ifndef NEW_RENDER
	/* fill in our portion of the display */
	current_display.changed_flags = 0;

	/* set the main game bitmap */
	current_display.game_bitmap = scrbitmap[0][curbitmap[0]];
	current_display.game_bitmap_update = Machine->absolute_visible_area;
	if (!skipped_it)
		current_display.changed_flags |= GAME_BITMAP_CHANGED;

	/* set the visible area */
	current_display.game_visible_area = Machine->absolute_visible_area;
	if (visible_area_changed)
		current_display.changed_flags |= GAME_VISIBLE_AREA_CHANGED;

	/* set the refresh rate */
	current_display.game_refresh_rate = Machine->refresh_rate[0];
	if (refresh_rate_changed)
		current_display.changed_flags |= GAME_REFRESH_RATE_CHANGED;

	/* set the vector dirty list */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (!full_refresh_pending && !ui_is_dirty() && !skipped_it)
		{
			current_display.vector_dirty_pixels = vector_dirty_list;
			current_display.changed_flags |= VECTOR_PIXELS_CHANGED;
		}

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* set the debugger bitmap */
	current_display.debug_bitmap = Machine->debug_bitmap;
	if (debugger_bitmap_changed)
		current_display.changed_flags |= DEBUG_BITMAP_CHANGED;
	debugger_bitmap_changed = 0;

	/* adjust the debugger focus */
	if (debugger_focus != current_display.debug_focus)
	{
		current_display.debug_focus = debugger_focus;
		current_display.changed_flags |= DEBUG_FOCUS_CHANGED;
	}
#endif

	/* set the LED status */
	if (leds_status != current_display.led_state)
	{
		current_display.led_state = leds_status;
		current_display.changed_flags |= LED_STATE_CHANGED;
	}

	/* update with data from other parts of the system */
	palette_update_display(&current_display);

	/* render */
	artwork_update_video_and_audio(&current_display);

	/* reset dirty flags */
	visible_area_changed = 0;
	refresh_rate_changed = 0;
#else
{
	int scrnum;

	/* call the OSD to update */
	skipping_this_frame = osd_update(mame_timer_get_time());

	/* empty the containers */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
			render_container_empty(render_container_get_screen(scrnum));
}
#endif

	/* update FPS */
	recompute_fps(skipped_it);
}


/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

static void recompute_fps(int skipped_it)
{
	/* increment the frame counters */
	frames_since_last_fps++;
	if (!skipped_it)
		rendered_frames_since_last_fps++;

	/* if we didn't skip this frame, we may be able to compute a new FPS */
	if (!skipped_it && frames_since_last_fps >= FRAMES_PER_FPS_UPDATE)
	{
		cycles_t cps = osd_cycles_per_second();
		cycles_t curr = osd_cycles();
		double seconds_elapsed = (double)(curr - last_fps_time) * (1.0 / (double)cps);
		double frames_per_sec = (double)frames_since_last_fps / seconds_elapsed;

		/* compute the performance data */
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->refresh_rate[0];
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->refresh_rate[0])
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->refresh_rate[0];
	}
}


#ifdef USE_SCALE_EFFECTS
static void alloc_scalebitmap(void)
{
	int scrnum;

	free_scalebitmap();

	scale_xsize = scale_effect.xsize;
	scale_ysize = scale_effect.ysize;

	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
	{
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			int bank;

			for (bank = 0; bank < 2; bank++)
			{
				scalebitmap[scrnum][bank] = bitmap_alloc_depth(
					Machine->drv->screen[scrnum].maxwidth * scale_xsize,
					Machine->drv->screen[scrnum].maxheight * scale_ysize,
					scale_depth);

				if (!use_workbitmap)
					continue;

				workbitmap[scrnum][bank] = bitmap_alloc_depth(
					Machine->drv->screen[scrnum].maxwidth,
					Machine->drv->screen[scrnum].maxheight,
					scale_depth);
			}
		}
	}
}

static void free_scalebitmap(void)
{
	int scrnum;

	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
	{
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			int bank;

			for (bank = 0; bank < 2; bank++)
			{
				if (scalebitmap[scrnum][bank])
					bitmap_free(scalebitmap[scrnum][bank]);
				if (workbitmap[scrnum][bank])
					bitmap_free(workbitmap[scrnum][bank]);
			}
		}
	}

	scale_xsize = 0;
	scale_ysize = 0;
}

static void convert_palette_to_32(const mame_bitmap *src, mame_bitmap *dst, const rectangle *visarea, const rgb_t *palette)
{
	int x, y;

	for (y = visarea->min_x; y < visarea->max_y; y++)
	{
		UINT32 *dst32 = ((UINT32 *)dst->line[y]) + visarea->min_x;
		UINT16 *src16 = ((UINT16 *)src->line[y]) + visarea->min_x;

		for (x = visarea->min_x; x < visarea->max_x; x++)
			*dst32++ = palette[*src16++];
	}
}

static void convert_palette_to_15(const mame_bitmap *src, mame_bitmap *dst, const rectangle *visarea, const rgb_t *palette)
{
	int x, y;

	for (y = visarea->min_x; y < visarea->max_y; y++)
	{
		UINT16 *dst16 = ((UINT16 *)dst->line[y]) + visarea->min_x;
		UINT16 *src16 = ((UINT16 *)src->line[y]) + visarea->min_x;

		for (x = visarea->min_x; x < visarea->max_x; x++)
		{
			UINT32 color = palette[*src16++];
			*dst16++ = ((color >> 9) & 0x7c00) | ((color >> 6) & 0x03e0) | ((color >> 3) & 0x001f);
		}
	}
}

static void convert_15_to_32(const mame_bitmap *src, mame_bitmap *dst, const rectangle *visarea)
{
	int x, y;

	for (y = visarea->min_x; y < visarea->max_y; y++)
	{
		UINT32 *dst32 = ((UINT32 *)dst->line[y]) + visarea->min_x;
		UINT16 *src16 = ((UINT16 *)src->line[y]) + visarea->min_x;

		for (x = visarea->min_x; x < visarea->max_x; x++)
		{
			UINT16 pix = *src16++;
			UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
			*dst32++ = color | ((color >> 5) & 0x070707) | 0xff000000;
		}
	}
}

static void convert_32_to_15(mame_bitmap *src, mame_bitmap *dst, const const rectangle *visarea)
{
	int x, y;

	for (y = visarea->min_x; y < visarea->max_y; y++)
	{
		UINT16 *dst16 = ((UINT16 *)dst->line[y]) + visarea->min_x;
		UINT32 *src32 = ((UINT32 *)src->line[y]) + visarea->min_x;

		for (x = visarea->min_x; x < visarea->max_x; x++)
		{
			UINT32 color = *src32++;
			*dst16++ = ((color >> 9) & 0x7c00) | ((color >> 6) & 0x03e0) | ((color >> 3) & 0x001f);
		}
	}
}

static void texture_set_scalebitmap(int scrnum, int curbank, rectangle *visarea)
{
	mame_bitmap *target = scrbitmap[scrnum][curbank];
	const rgb_t *palette;
	mame_bitmap *dst;
	int width = visarea->max_x - visarea->min_x;
	int height = visarea->max_y - visarea->min_y;
	int update = 0;

	if (scale_xsize != scale_effect.xsize || scale_ysize != scale_effect.ysize)
	{
		alloc_scalebitmap();
		update = 1;
	}

	dst = scalebitmap[scrnum][curbank];

	switch (scrformat[scrnum])
	{
	case TEXFORMAT_PALETTE16:
		palette = &adjusted_palette[Machine->drv->screen[scrnum].palette_base];
		target = workbitmap[scrnum][curbank];

		if (scale_depth == 32)
			convert_palette_to_32(scrbitmap[scrnum][curbank], target, visarea, palette);
		else
			convert_palette_to_15(scrbitmap[scrnum][curbank], target, visarea, palette);

		break;

	case TEXFORMAT_RGB15:
		if (scale_depth == 15)
			break;

		target = workbitmap[scrnum][curbank];
		convert_15_to_32(scrbitmap[scrnum][curbank], target, visarea);
		break;

	case TEXFORMAT_RGB32:
		if (scale_depth == 32)
			break;

		convert_32_to_15(scrbitmap[scrnum][curbank], target, visarea);
		target = workbitmap[scrnum][curbank];
		break;
	}

	if (scale_depth == 32)
	{
		UINT32 *src32 = ((UINT32 *)target->line[visarea->min_y]) + visarea->min_x;
		UINT32 *dst32 = ((UINT32 *)dst->line[visarea->min_y]) + visarea->min_x;
		scale_perform_scale((UINT8 *)src32, (UINT8 *)dst32, target->rowbytes, dst->rowbytes, width, height, 32, update);
	}
	else
	{
		UINT16 *src16 = ((UINT16 *)target->line[visarea->min_y]) + visarea->min_x;
		UINT16 *dst16 = ((UINT16 *)dst->line[visarea->min_y]) + visarea->min_x;
		scale_perform_scale((UINT8 *)src16, (UINT8 *)dst16, target->rowbytes, dst->rowbytes, width, height, 15, update);
	}

	visarea->min_x *= scale_effect.xsize;
	visarea->max_x *= scale_effect.xsize;
	visarea->min_y *= scale_effect.ysize;
	visarea->max_y *= scale_effect.ysize;

	render_texture_set_bitmap(scrtexture[scrnum], dst, visarea, NULL, (scale_depth == 32) ? TEXFORMAT_RGB32 : TEXFORMAT_RGB15);
}
#endif /* USE_SCALE_EFFECTS */


/*-------------------------------------------------
    video_frame_update - handle frameskipping and UI,
    plus updating the screen during normal
    operations
-------------------------------------------------*/

void video_frame_update(void)
{
	int paused = mame_is_paused();
	int phase = mame_get_phase();
	int scrnum;

	/* only render sound and video if we're in the running phase */
	if (phase == MAME_PHASE_RUNNING)
	{
		/* update sound */
		sound_frame_update();

		/* finish updating the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (Machine->drv->screen[scrnum].tag != NULL)
				force_partial_update(scrnum, Machine->visible_area[scrnum].max_y);

		/* update our movie recording state */
		if (!paused)
			record_movie_frame(scrbitmap[0][curbitmap[0]]);

#ifdef NEW_RENDER
{
		int livemask = render_get_live_screens_mask();

		/* now add the quads for all the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (livemask & (1 << scrnum))
			{
				/* only update if empty and not a vector game; otherwise assume the driver did it directly */
				if (render_container_is_empty(render_container_get_screen(scrnum)) && !(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
				{
					if (!skipping_this_frame && scrchanged[scrnum])
					{
						rectangle visarea = Machine->visible_area[scrnum];
						visarea.max_x++;
						visarea.max_y++;
#ifdef USE_SCALE_EFFECTS
						if (scale_effect.effect)
							texture_set_scalebitmap(scrnum, curbitmap[scrnum], &visarea);
						else
#endif /* USE_SCALE_EFFECTS */
						render_texture_set_bitmap(scrtexture[scrnum], scrbitmap[scrnum][curbitmap[scrnum]], &visarea, &adjusted_palette[Machine->drv->screen[scrnum].palette_base], scrformat[scrnum]);
						curbitmap[scrnum] = 1 - curbitmap[scrnum];
					}
					render_screen_add_quad(scrnum, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), scrtexture[scrnum], PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_SCREENTEX(1));
				}
			}

		/* reset the screen changed flags */
		memset(scrchanged, 0, sizeof(scrchanged));
}
#endif
	}

	/* the user interface must be called between vh_update() and osd_update_video_and_audio(), */
	/* to allow it to overlay things on the game display. We must call it even */
	/* if the frame is skipped, to keep a consistent timing. */
#ifndef NEW_RENDER
	ui_update_and_render(artwork_get_ui_bitmap());
#else
	ui_update_and_render();
#endif

	/* blit to the screen */
	update_video_and_audio();

	/* call the end-of-frame callback */
	if (phase == MAME_PHASE_RUNNING)
	{
		if (Machine->drv->video_eof && !paused)
		{
			profiler_mark(PROFILER_VIDEO);
			(*Machine->drv->video_eof)();
			profiler_mark(PROFILER_END);
		}

		/* reset partial updates if we're paused or if the debugger is active */
		if (paused || mame_debug_is_active())
			reset_partial_updates();
	}
}


/*-------------------------------------------------
    skip_this_frame -
-------------------------------------------------*/

int skip_this_frame(void)
{
#ifndef NEW_RENDER
	return osd_skip_this_frame();
#else
	return skipping_this_frame;
#endif
}



/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const performance_info *mame_get_performance_info(void)
{
	return &performance;
}




/***************************************************************************

    Screen snapshot and movie recording code

***************************************************************************/

/*-------------------------------------------------
    save_frame_with - save a frame with a
    given handler for screenshots and movies
-------------------------------------------------*/

static void save_frame_with(mame_file *fp, mame_bitmap *bitmap, int (*write_handler)(mame_file *, mame_bitmap *))
{
	rectangle bounds;
	mame_bitmap *osdcopy;
	UINT32 saved_rgb_components[3];

	/* allow the artwork system to override certain parameters */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		bounds.min_x = 0;
		bounds.max_x = bitmap->width - 1;
		bounds.min_y = 0;
		bounds.max_y = bitmap->height - 1;
	}
	else
	{
		bounds = Machine->visible_area[0];
	}
	memcpy(saved_rgb_components, direct_rgb_components, sizeof(direct_rgb_components));
#ifndef NEW_RENDER
	artwork_override_screenshot_params(&bitmap, &bounds, direct_rgb_components);
#endif

	/* allow the OSD system to muck with the screenshot */
	osdcopy = osd_override_snapshot(bitmap, &bounds);
	if (osdcopy)
		bitmap = osdcopy;

	/* now do the actual work */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		write_handler(fp, bitmap);
	}
	else
	{
		mame_bitmap *copy;
		int sizex, sizey, scalex, scaley;

		sizex = bounds.max_x - bounds.min_x + 1;
		sizey = bounds.max_y - bounds.min_y + 1;

		scalex = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_2_1) ? 2 : 1;
		scaley = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) ? 2 : 1;

		if(Machine->gamedrv->flags & ORIENTATION_SWAP_XY)
		{
			int temp;

			temp = scalex;
			scalex = scaley;
			scaley = temp;
		}

		copy = bitmap_alloc_depth(sizex * scalex,sizey * scaley,bitmap->depth);
		if (copy)
		{
			int x,y,sx,sy;

			sx = bounds.min_x;
			sy = bounds.min_y;

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			write_handler(fp, copy);
			bitmap_free(copy);
		}
	}
	memcpy(direct_rgb_components, saved_rgb_components, sizeof(saved_rgb_components));

	/* if the OSD system allocated a bitmap; free it */
	if (osdcopy)
		bitmap_free(osdcopy);
}


 /*-------------------------------------------------
    save_screen_snapshot_as - save a snapshot to
    the given file handle
-------------------------------------------------*/

void save_screen_snapshot_as(mame_file *fp, mame_bitmap *bitmap)
{
	save_frame_with(fp, bitmap, png_write_bitmap);
}


/*-------------------------------------------------
    open the next non-existing file of type
    filetype according to our numbering scheme
-------------------------------------------------*/

static mame_file *mame_fopen_next(int filetype)
{
	char name[FILENAME_MAX];
	int seq;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.xxx" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (mame_faccess(name, filetype))
	{
		seq = 0;
		do
		{
			/* otherwise use "nameNNNN.xxx" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name, seq++);
		} while (mame_faccess(name, filetype));
	}

    return (mame_fopen(Machine->gamedrv->name, name, filetype, 1));
}


/*-------------------------------------------------
    save_screen_snapshot - save a snapshot.
-------------------------------------------------*/

void save_screen_snapshot(mame_bitmap *bitmap)
{
	UINT32 screenmask = render_get_live_screens_mask();
	mame_file *fp;
	int scrnum;

	/* write one snapshot per visible screen */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (screenmask & (1 << scrnum))
			if ((fp = mame_fopen_next(FILETYPE_SCREENSHOT)) != NULL)
			{
				save_screen_snapshot_as(fp, scrbitmap[scrnum][curbitmap[scrnum]]);
				mame_fclose(fp);
			}
}


/*-------------------------------------------------
    record_movie - start, stop and update the
    recording of a MNG movie
-------------------------------------------------*/

void record_movie_start(const char *name)
{
	if (movie_file != NULL)
		mame_fclose(movie_file);

	if (name)
		movie_file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MOVIE, 1);
	else
		movie_file = mame_fopen_next(FILETYPE_MOVIE);

	movie_frame = 0;
}


void record_movie_stop(void)
{
	if (movie_file)
	{
		mng_capture_stop(movie_file);
		mame_fclose(movie_file);
		movie_file = NULL;
	}
}


void record_movie_toggle(void)
{
	if (movie_file == NULL)
	{
		record_movie_start(NULL);
		if (movie_file)
			ui_popup(_("REC START"));
	}
	else
	{
		record_movie_stop();
		ui_popup(_("REC STOP (%d frames)"), movie_frame);
	}
}


void record_movie_frame(mame_bitmap *bitmap)
{
	if (movie_file != NULL && bitmap != NULL)
	{
		profiler_mark(PROFILER_MOVIE_REC);

		if (movie_frame++ == 0)
			save_frame_with(movie_file, bitmap, mng_capture_start);
		save_frame_with(movie_file, bitmap, mng_capture_frame);

		profiler_mark(PROFILER_END);
	}
}



/***************************************************************************

    Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
    pp_* -- pixel plotting callbacks
-------------------------------------------------*/

static void pp_8 (mame_bitmap *b, int x, int y, pen_t p)  { ((UINT8 *)b->line[y])[x] = p; }
static void pp_16(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT16 *)b->line[y])[x] = p; }
static void pp_32(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT32 *)b->line[y])[x] = p; }


/*-------------------------------------------------
    rp_* -- pixel reading callbacks
-------------------------------------------------*/

static pen_t rp_8 (mame_bitmap *b, int x, int y)  { return ((UINT8 *)b->line[y])[x]; }
static pen_t rp_16(mame_bitmap *b, int x, int y)  { return ((UINT16 *)b->line[y])[x]; }
static pen_t rp_32(mame_bitmap *b, int x, int y)  { return ((UINT32 *)b->line[y])[x]; }


/*-------------------------------------------------
    pb_* -- box plotting callbacks
-------------------------------------------------*/

static void pb_8 (mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT8 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_16(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT16 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_32(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT32 *)b->line[y])[x] = p; x++; } y++; } }


/*-------------------------------------------------
    bitmap_alloc_core
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(mame_bitmap)) : malloc(sizeof(mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		UINT8 *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(UINT8 *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (UINT8 *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		if (pixelsize == 1)
		{
			bitmap->read = rp_8;
			bitmap->plot = pp_8;
			bitmap->plot_box = pb_8;
		}
		else if (pixelsize == 2)
		{
			bitmap->read = rp_16;
			bitmap->plot = pp_16;
			bitmap->plot_box = pb_16;
		}
		else
		{
			bitmap->read = rp_32;
			bitmap->plot = pp_32;
			bitmap->plot_box = pb_32;
		}
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, FALSE);
}


/*-------------------------------------------------
    auto_bitmap_alloc_depth - allocate a bitmap
    for a specific depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, TRUE);
}


/*-------------------------------------------------
    bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



