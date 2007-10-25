/*************************************************************************

    Driver for Midway V-Unit games

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "audio/williams.h"
#include "video/polynew.h"
#include "midvunit.h"


#define WATCH_RENDER		(0)
#define LOG_DMA				(0)


#define DMA_CLOCK			40000000


/* for when we implement DMA timing */
#define DMA_QUEUE_SIZE		273
#define TIME_PER_PIXEL		41e-9



UINT16 *midvunit_videoram;
UINT32 *midvunit_textureram;

static UINT16 video_regs[16];
static UINT16 dma_data[16];
static UINT8 dma_data_index;
static UINT16 page_control;
static UINT8 video_changed;

static mame_timer *scanline_timer;
static poly_manager *poly;

typedef struct _poly_extra_data poly_extra_data;
struct _poly_extra_data
{
	UINT8 *		texbase;
	UINT16 		pixdata;
	UINT8		dither;
};



/*************************************
 *
 *  Video system start
 *
 *************************************/

static TIMER_CALLBACK( scanline_timer_cb )
{
	int scanline = param;

	cpunum_set_input_line(0, 0, ASSERT_LINE);
	mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, scanline + 1, 0), scanline, time_zero);
}


static void midvunit_exit(running_machine *machine)
{
	poly_free(poly);
}


VIDEO_START( midvunit )
{
	scanline_timer = mame_timer_alloc(scanline_timer_cb);
	poly = poly_alloc(4000, sizeof(poly_extra_data), 0);
	add_exit_callback(machine, midvunit_exit);
}



/*************************************
 *
 *  Generic flat quad renderer
 *
 *************************************/

static void render_flat(void *destbase, INT32 scanline, INT32 startx, INT32 stopx, const poly_params *poly, int threadid)
{
	poly_extra_data *extra = poly->extra;
	UINT16 *dest = (UINT16 *)destbase + scanline * 512;
	UINT16 pixdata = extra->pixdata;
	int xstep = extra->dither + 1;
	int x;

	/* if dithering, ensure that we start on an appropriate pixel */
	startx += (scanline ^ startx) & extra->dither;

	/* non-dithered 0 pixels can use a memset */
	if (pixdata == 0 && xstep == 1)
		memset(&dest[startx], 0, 2 * (stopx - startx + 1));

	/* otherwise, we fill manually */
	else
	{
		for (x = startx; x < stopx; x += xstep)
			dest[x] = pixdata;
	}
}



/*************************************
 *
 *  Generic textured quad renderers
 *
 *************************************/

static void render_tex(void *destbase, INT32 scanline, INT32 startx, INT32 stopx, const poly_params *poly, int threadid)
{
	poly_extra_data *extra = poly->extra;
	UINT16 *dest = (UINT16 *)destbase + scanline * 512;
	UINT16 pixdata = extra->pixdata & 0xff00;
	const UINT8 *texbase = extra->texbase;
	INT32 u = poly_param_value(startx, scanline, 0, poly) * 65536.0f;
	INT32 v = poly_param_value(startx, scanline, 1, poly) * 65536.0f;
	INT32 dudx = poly->param[0].dpdx * 65536.0f;
	INT32 dvdx = poly->param[1].dpdx * 65536.0f;
	int xstep = extra->dither + 1;
	int x;

	/* if dithering, we advance by 2x; also ensure that we start on an appropriate pixel */
	if (xstep == 2)
	{
		if ((scanline ^ startx) & 1)
		{
			startx++;
			u += dudx;
			v += dvdx;
		}
		dudx *= 2;
		dvdx *= 2;
	}

	/* general case; render every pixel */
	for (x = startx; x < stopx; x += xstep)
	{
		dest[x] = pixdata | texbase[((v >> 8) & 0xff00) + (u >> 16)];
		u += dudx;
		v += dvdx;
	}
}


static void render_textrans(void *destbase, INT32 scanline, INT32 startx, INT32 stopx, const poly_params *poly, int threadid)
{
	poly_extra_data *extra = poly->extra;
	UINT16 *dest = (UINT16 *)destbase + scanline * 512;
	UINT16 pixdata = extra->pixdata & 0xff00;
	const UINT8 *texbase = extra->texbase;
	INT32 u = poly_param_value(startx, scanline, 0, poly) * 65536.0f;
	INT32 v = poly_param_value(startx, scanline, 1, poly) * 65536.0f;
	INT32 dudx = poly->param[0].dpdx * 65536.0f;
	INT32 dvdx = poly->param[1].dpdx * 65536.0f;
	int xstep = extra->dither + 1;
	int x;

	/* if dithering, we advance by 2x; also ensure that we start on an appropriate pixel */
	if (xstep == 2)
	{
		if ((scanline ^ startx) & 1)
		{
			startx++;
			u += dudx;
			v += dvdx;
		}
		dudx *= 2;
		dvdx *= 2;
	}

	/* general case; render every non-zero texel */
	for (x = startx; x < stopx; x += xstep)
	{
		UINT8 pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
		if (pix != 0)
			dest[x] = pixdata | pix;
		u += dudx;
		v += dvdx;
	}
}


static void render_textransmask(void *destbase, INT32 scanline, INT32 startx, INT32 stopx, const poly_params *poly, int threadid)
{
	poly_extra_data *extra = poly->extra;
	UINT16 *dest = (UINT16 *)destbase + scanline * 512;
	UINT16 pixdata = extra->pixdata;
	const UINT8 *texbase = extra->texbase;
	INT32 u = poly_param_value(startx, scanline, 0, poly) * 65536.0f;
	INT32 v = poly_param_value(startx, scanline, 1, poly) * 65536.0f;
	INT32 dudx = poly->param[0].dpdx * 65536.0f;
	INT32 dvdx = poly->param[1].dpdx * 65536.0f;
	int xstep = extra->dither + 1;
	int x;

	/* if dithering, we advance by 2x; also ensure that we start on an appropriate pixel */
	if (xstep == 2)
	{
		if ((scanline ^ startx) & 1)
		{
			startx++;
			u += dudx;
			v += dvdx;
		}
		dudx *= 2;
		dvdx *= 2;
	}

	/* general case; every non-zero texel renders pixdata */
	for (x = startx; x < stopx; x += xstep)
	{
		UINT8 pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
		if (pix != 0)
			dest[x] = pixdata;
		u += dudx;
		v += dvdx;
	}
}



/*************************************
 *
 *  DMA queue processor
 *
 *************************************/

static void process_dma_queue(running_machine *machine)
{
	int textured = ((dma_data[0] & 0x300) == 0x100);
	poly_draw_scanline callback;
	poly_vertex vert[4];
	int i;

	/* if we're rendering to the same page we're viewing, it has changed */
	if ((((page_control >> 2) ^ page_control) & 1) == 0)
		video_changed = TRUE;

	/* fill in the vertex data */
	vert[0].x = (float)(INT16)dma_data[2];
	vert[0].y = (float)(INT16)dma_data[3];
	vert[1].x = (float)(INT16)dma_data[4];
	vert[1].y = (float)(INT16)dma_data[5];
	vert[2].x = (float)(INT16)dma_data[6];
	vert[2].y = (float)(INT16)dma_data[7];
	vert[3].x = (float)(INT16)dma_data[8];
	vert[3].y = (float)(INT16)dma_data[9];

	/* handle flat-shaded quads here */
	if (!textured)
		callback = render_flat;

	/* handle textured quads here */
	else
	{
		/* if textured, add the texture info */
		vert[0].p[0] = (float)(dma_data[10] & 0xff);
		vert[0].p[1] = (float)(dma_data[10] >> 8);
		vert[1].p[0] = (float)(dma_data[11] & 0xff);
		vert[1].p[1] = (float)(dma_data[11] >> 8);
		vert[2].p[0] = (float)(dma_data[12] & 0xff);
		vert[2].p[1] = (float)(dma_data[12] >> 8);
		vert[3].p[0] = (float)(dma_data[13] & 0xff);
		vert[3].p[1] = (float)(dma_data[13] >> 8);

		/* handle non-masked, non-transparent quads */
		if ((dma_data[0] & 0xc00) == 0x000)
			callback = render_tex;

		/* handle non-masked, transparent quads */
		else if ((dma_data[0] & 0xc00) == 0x800)
			callback = render_textrans;

		/* handle masked, transparent quads */
		else if ((dma_data[0] & 0xc00) == 0xc00)
			callback = render_textransmask;

		/* handle masked, non-transparent quads */
		else
			callback = render_flat;
	}

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		poly_extra_data *extra = poly_get_extra_data(poly);
		UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];

		/* set up the extra data for this triangle */
		extra->texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
		extra->pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
		extra->dither = ((dma_data[0] & 0x2000) != 0);

		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			poly_render_triangle(poly, dest, &machine->screen[0].visarea, callback, textured ? 2 : 0, &vert[0], &vert[1], &vert[2]);
		else
			poly_render_triangle(poly, dest, &machine->screen[0].visarea, callback, textured ? 2 : 0, &vert[0], &vert[3], &vert[2]);
	}
}



/*************************************
 *
 *  DMA pipe control control
 *
 *************************************/

WRITE32_HANDLER( midvunit_dma_queue_w )
{
	if (LOG_DMA && input_code_pressed(KEYCODE_L))
		logerror("%06X:queue(%X) = %08X\n", activecpu_get_pc(), dma_data_index, data);
	if (dma_data_index < 16)
		dma_data[dma_data_index++] = data;
}


READ32_HANDLER( midvunit_dma_queue_entries_r )
{
	/* always return 0 entries */
	return 0;
}


READ32_HANDLER( midvunit_dma_trigger_r )
{
	if (offset)
	{
		if (LOG_DMA && input_code_pressed(KEYCODE_L))
			logerror("%06X:trigger\n", activecpu_get_pc());
		process_dma_queue(Machine);
		dma_data_index = 0;
	}
	return 0;
}



/*************************************
 *
 *  Paging control
 *
 *************************************/

WRITE32_HANDLER( midvunit_page_control_w )
{
	/* watch for the display page to change */
	if ((page_control ^ data) & 1)
	{
		video_changed = TRUE;
		if (LOG_DMA && input_code_pressed(KEYCODE_L))
			logerror("##########################################################\n");
		video_screen_update_partial(0, video_screen_get_vpos(0) - 1);
	}
	page_control = data;
}


READ32_HANDLER( midvunit_page_control_r )
{
	return page_control;
}



/*************************************
 *
 *  Video control
 *
 *************************************/

WRITE32_HANDLER( midvunit_video_control_w )
{
	UINT16 old = video_regs[offset];

	/* update the data */
	COMBINE_DATA(&video_regs[offset]);

	/* update the scanline timer */
	if (offset == 0)
		mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, (data & 0x1ff) + 1, 0), data & 0x1ff, time_zero);

	/* if something changed, update our parameters */
	if (old != video_regs[offset] && video_regs[6] != 0 && video_regs[11] != 0)
	{
		rectangle visarea;

		/* derive visible area from blanking */
		visarea.min_x = 0;
		visarea.max_x = (video_regs[6] + video_regs[2] - video_regs[5]) % video_regs[6];
		visarea.min_y = 0;
		visarea.max_y = (video_regs[11] + video_regs[7] - video_regs[10]) % video_regs[11];
		video_screen_configure(0, video_regs[6], video_regs[11], &visarea, HZ_TO_SUBSECONDS(MIDVUNIT_VIDEO_CLOCK / 2) * video_regs[6] * video_regs[11]);
	}
}


READ32_HANDLER( midvunit_scanline_r )
{
	return video_screen_get_vpos(0);
}



/*************************************
 *
 *  Video RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_videoram_w )
{
	poly_wait(poly, "Video RAM write");
	if (!video_changed)
	{
		int visbase = (page_control & 1) ? 0x40000 : 0x00000;
		if ((offset & 0x40000) == visbase)
			video_changed = TRUE;
	}
	COMBINE_DATA(&midvunit_videoram[offset]);
}


READ32_HANDLER( midvunit_videoram_r )
{
	poly_wait(poly, "Video RAM read");
	return midvunit_videoram[offset];
}



/*************************************
 *
 *  Palette RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_paletteram_w )
{
	int newword;

	COMBINE_DATA(&paletteram32[offset]);
	newword = paletteram32[offset];
	palette_set_color_rgb(Machine, offset, pal5bit(newword >> 10), pal5bit(newword >> 5), pal5bit(newword >> 0));
}



/*************************************
 *
 *  Texture RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_textureram_w )
{
	UINT8 *base = (UINT8 *)midvunit_textureram;
	poly_wait(poly, "Texture RAM write");
	base[offset * 2] = data;
	base[offset * 2 + 1] = data >> 8;
}


READ32_HANDLER( midvunit_textureram_r )
{
	UINT8 *base = (UINT8 *)midvunit_textureram;
	return (base[offset * 2 + 1] << 8) | base[offset * 2];
}




/*************************************
 *
 *  Video system update
 *
 *************************************/

VIDEO_UPDATE( midvunit )
{
	int x, y, width, xoffs;
	UINT32 offset;

	poly_wait(poly, "Refresh Time");

	/* if the video didn't change, indicate as much */
	if (!video_changed)
		return UPDATE_HAS_NOT_CHANGED;
	video_changed = FALSE;

	/* determine the base of the videoram */
#if WATCH_RENDER
	offset = (page_control & 4) ? 0x40000 : 0x00000;
#else
	offset = (page_control & 1) ? 0x40000 : 0x00000;
#endif

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;

	/* adjust the offset */
	offset += xoffs;
	offset += 512 * (cliprect->min_y - machine->screen[0].visarea.min_y);

	/* loop over rows */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels + cliprect->min_x;
		for (x = 0; x < width; x++)
			*dest++ = midvunit_videoram[offset + x] & 0x7fff;
		offset += 512;
	}
	return 0;
}
