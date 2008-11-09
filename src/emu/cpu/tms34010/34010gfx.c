/***************************************************************************

    TMS34010: Portable Texas Instruments TMS34010 emulator

    Copyright Alex Pasadyn/Zsolt Vasvari
    Parts based on code by Aaron Giles

***************************************************************************/

#ifndef RECURSIVE_INCLUDE

#define LOG_GFX_OPS 0
#define LOGGFX(x) do { if (LOG_GFX_OPS && input_code_pressed(KEYCODE_L)) logerror x; } while (0)


/* Graphics Instructions */

static void line(void)
{
	if (!P_FLAG)
	{
		if (WINDOW_CHECKING != 0 && WINDOW_CHECKING != 3)
			logerror("LINE XY  %08X - Window Checking Mode %d not supported\n", PC, WINDOW_CHECKING);

		tms.st |= STBIT_P;
		TEMP = (tms.op & 0x80) ? 1 : 0;  /* boundary value depends on the algorithm */
		LOGGFX(("%08X(%3d):LINE (%d,%d)-(%d,%d)\n", PC, video_screen_get_vpos(tms.screen), DADDR_X, DADDR_Y, DADDR_X + DYDX_X, DADDR_Y + DYDX_Y));
	}

	if (COUNT > 0)
	{
		INT16 x1,y1;

		COUNT--;
		if (WINDOW_CHECKING != 3 ||
			(DADDR_X >= WSTART_X && DADDR_X <= WEND_X &&
			 DADDR_Y >= WSTART_Y && DADDR_Y <= WEND_Y))
			WPIXEL(DXYTOL(DADDR_XY),COLOR1);

		if (SADDR >= TEMP)
		{
			SADDR += DYDX_Y*2 - DYDX_X*2;
			x1 = INC1_X;
			y1 = INC1_Y;
		}
		else
		{
			SADDR += DYDX_Y*2;
			x1 = INC2_X;
			y1 = INC2_Y;
		}
		DADDR_X += x1;
		DADDR_Y += y1;

		COUNT_UNKNOWN_CYCLES(2);
		PC -= 0x10;  /* not done yet, check for interrupts and restart instruction */
		return;
	}
	tms.st &= ~STBIT_P;
}


/*
cases:
* window modes (0,1,2,3)
* boolean/arithmetic ops (16+6)
* transparency (on/off)
* plane masking
* directions (left->right/right->left, top->bottom/bottom->top)
*/

static int apply_window(const char *inst_name,int srcbpp, UINT32 *srcaddr, XY *dst, int *dx, int *dy)
{
	/* apply the window */
	if (WINDOW_CHECKING == 0)
		return 0;
	else
	{
		int sx = dst->x;
		int sy = dst->y;
		int ex = sx + *dx - 1;
		int ey = sy + *dy - 1;
		int diff, cycles = 3;

		if (WINDOW_CHECKING == 2)
			logerror("%08x: %s apply_window window mode %d not supported!\n", activecpu_get_pc(), inst_name, WINDOW_CHECKING);

		CLR_V;
		if (WINDOW_CHECKING == 1)
			SET_V_LOG(1);

		/* clip X */
		diff = WSTART_X - sx;
		if (diff > 0)
		{
			if (srcaddr)
				*srcaddr += diff * srcbpp;
			sx += diff;
			SET_V_LOG(1);
		}
		diff = ex - WEND_X;
		if (diff > 0)
		{
			ex -= diff;
			SET_V_LOG(1);
		}

		/* clip Y */
		diff = WSTART_Y - sy;
		if (diff > 0)
		{
			if (srcaddr)
				*srcaddr += diff * SPTCH;
			sy += diff;
			SET_V_LOG(1);
		}
		diff = ey - WEND_Y;
		if (diff > 0)
		{
			ey -= diff;
			SET_V_LOG(1);
		}

		/* compute cycles */
		if (*dx != ex - sx + 1 || *dy != ey - sy + 1)
		{
			if (dst->x != sx || dst->y != sy)
				cycles += 11;
			else
				cycles += 3;
		}
		else if (dst->x != sx || dst->y != sy)
			cycles += 7;

		/* update the values */
		dst->x = sx;
		dst->y = sy;
		*dx = ex - sx + 1;
		*dy = ey - sy + 1;
		return cycles;
	}
}


/*******************************************************************

    About the timing of gfx operations:

    The 34010 manual lists a fairly intricate and accurate way of
    computing cycle timings for graphics ops. However, there are
    enough typos and misleading statements to make the reliability
    of the timing info questionable.

    So, to address this, here is a simplified approximate version
    of the timing.

        timing = setup + (srcwords * 2 + dstwords * gfxop) * rows

    Each read access takes 2 cycles. Each gfx operation has
    its own timing as specified in the 34010 manual. So, it's 2
    cycles per read plus gfxop cycles per operation. Pretty
    simple, no?

*******************************************************************/

static int compute_fill_cycles(int left_partials, int right_partials, int full_words, int op_timing)
{
	int dstwords;

	if (left_partials) full_words += 1;
	if (right_partials) full_words += 1;
	dstwords = full_words;

	return (dstwords * op_timing);
}

static int compute_pixblt_cycles(int left_partials, int right_partials, int full_words, int op_timing)
{
	int srcwords, dstwords;

	if (left_partials) full_words += 1;
	if (right_partials) full_words += 1;
	srcwords = full_words;
	dstwords = full_words;

	return (dstwords * op_timing + srcwords * 2) + 2;
}

static int compute_pixblt_b_cycles(int left_partials, int right_partials, int full_words, int rows, int op_timing, int bpp)
{
	int srcwords, dstwords;

	if (left_partials) full_words += 1;
	if (right_partials) full_words += 1;
	srcwords = full_words * bpp / 16;
	dstwords = full_words;

	return (dstwords * op_timing + srcwords * 2) * rows + 2;
}


/* Shift register handling */
static void shiftreg_w(offs_t offset,UINT16 data)
{
	if (tms.config->from_shiftreg)
		(*tms.config->from_shiftreg)((UINT32)(offset << 3) & ~15, &tms.shiftreg[0]);
	else
		logerror("From ShiftReg function not set. PC = %08X\n", PC);
}

static UINT16 shiftreg_r(offs_t offset)
{
	if (tms.config->to_shiftreg)
		(*tms.config->to_shiftreg)((UINT32)(offset << 3) & ~15, &tms.shiftreg[0]);
	else
		logerror("To ShiftReg function not set. PC = %08X\n", PC);
	return tms.shiftreg[0];
}

static UINT16 dummy_shiftreg_r(offs_t offset)
{
	return tms.shiftreg[0];
}



/* Pixel operations */
static UINT32 pixel_op00(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return srcpix; }
static UINT32 pixel_op01(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return srcpix & dstpix; }
static UINT32 pixel_op02(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return srcpix & ~dstpix; }
static UINT32 pixel_op03(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return 0; }
static UINT32 pixel_op04(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (srcpix | ~dstpix) & mask; }
static UINT32 pixel_op05(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return ~(srcpix ^ dstpix) & mask; }
static UINT32 pixel_op06(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return ~dstpix & mask; }
static UINT32 pixel_op07(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return ~(srcpix | dstpix) & mask; }
static UINT32 pixel_op08(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (srcpix | dstpix) & mask; }
static UINT32 pixel_op09(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return dstpix & mask; }
static UINT32 pixel_op10(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (srcpix ^ dstpix) & mask; }
static UINT32 pixel_op11(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (~srcpix & dstpix) & mask; }
static UINT32 pixel_op12(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return mask; }
static UINT32 pixel_op13(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (~srcpix & dstpix) & mask; }
static UINT32 pixel_op14(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return ~(srcpix & dstpix) & mask; }
static UINT32 pixel_op15(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return srcpix ^ mask; }
static UINT32 pixel_op16(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (srcpix + dstpix) & mask; }
static UINT32 pixel_op17(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { INT32 tmp = srcpix + (dstpix & mask); return (tmp > mask) ? mask : tmp; }
static UINT32 pixel_op18(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { return (dstpix - srcpix) & mask; }
static UINT32 pixel_op19(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { INT32 tmp = srcpix - (dstpix & mask); return (tmp < 0) ? 0 : tmp; }
static UINT32 pixel_op20(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { dstpix &= mask; return (srcpix > dstpix) ? srcpix : dstpix; }
static UINT32 pixel_op21(UINT32 dstpix, UINT32 mask, UINT32 srcpix) { dstpix &= mask; return (srcpix < dstpix) ? srcpix : dstpix; }

static UINT32 (*const pixel_op_table[])(UINT32, UINT32, UINT32) =
{
	pixel_op00,	pixel_op01,	pixel_op02,	pixel_op03,	pixel_op04,	pixel_op05,	pixel_op06,	pixel_op07,
	pixel_op08,	pixel_op09,	pixel_op10,	pixel_op11,	pixel_op12,	pixel_op13,	pixel_op14,	pixel_op15,
	pixel_op16,	pixel_op17,	pixel_op18,	pixel_op19,	pixel_op20,	pixel_op21,	pixel_op00,	pixel_op00,
	pixel_op00,	pixel_op00,	pixel_op00,	pixel_op00,	pixel_op00,	pixel_op00,	pixel_op00,	pixel_op00
};
static const UINT8 pixel_op_timing_table[] =
{
	2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,6,5,5,2,2,2,2,2,2,2,2,2,2,2
};
static UINT32 (*pixel_op)(UINT32, UINT32, UINT32);
static UINT32 pixel_op_timing;


/* Blitters/fillers */
static void pixblt_1_op0(int src_is_linear, int dst_is_linear);
static void pixblt_2_op0(int src_is_linear, int dst_is_linear);
static void pixblt_4_op0(int src_is_linear, int dst_is_linear);
static void pixblt_8_op0(int src_is_linear, int dst_is_linear);
static void pixblt_16_op0(int src_is_linear, int dst_is_linear);
static void pixblt_r_1_op0(int src_is_linear, int dst_is_linear);
static void pixblt_r_2_op0(int src_is_linear, int dst_is_linear);
static void pixblt_r_4_op0(int src_is_linear, int dst_is_linear);
static void pixblt_r_8_op0(int src_is_linear, int dst_is_linear);
static void pixblt_r_16_op0(int src_is_linear, int dst_is_linear);
static void pixblt_b_1_op0(int dst_is_linear);
static void pixblt_b_2_op0(int dst_is_linear);
static void pixblt_b_4_op0(int dst_is_linear);
static void pixblt_b_8_op0(int dst_is_linear);
static void pixblt_b_16_op0(int dst_is_linear);
static void fill_1_op0(int dst_is_linear);
static void fill_2_op0(int dst_is_linear);
static void fill_4_op0(int dst_is_linear);
static void fill_8_op0(int dst_is_linear);
static void fill_16_op0(int dst_is_linear);

static void pixblt_1_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_2_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_4_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_8_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_16_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_1_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_2_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_4_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_8_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_16_op0_trans(int src_is_linear, int dst_is_linear);
static void pixblt_b_1_op0_trans(int dst_is_linear);
static void pixblt_b_2_op0_trans(int dst_is_linear);
static void pixblt_b_4_op0_trans(int dst_is_linear);
static void pixblt_b_8_op0_trans(int dst_is_linear);
static void pixblt_b_16_op0_trans(int dst_is_linear);
static void fill_1_op0_trans(int dst_is_linear);
static void fill_2_op0_trans(int dst_is_linear);
static void fill_4_op0_trans(int dst_is_linear);
static void fill_8_op0_trans(int dst_is_linear);
static void fill_16_op0_trans(int dst_is_linear);

static void pixblt_1_opx(int src_is_linear, int dst_is_linear);
static void pixblt_2_opx(int src_is_linear, int dst_is_linear);
static void pixblt_4_opx(int src_is_linear, int dst_is_linear);
static void pixblt_8_opx(int src_is_linear, int dst_is_linear);
static void pixblt_16_opx(int src_is_linear, int dst_is_linear);
static void pixblt_r_1_opx(int src_is_linear, int dst_is_linear);
static void pixblt_r_2_opx(int src_is_linear, int dst_is_linear);
static void pixblt_r_4_opx(int src_is_linear, int dst_is_linear);
static void pixblt_r_8_opx(int src_is_linear, int dst_is_linear);
static void pixblt_r_16_opx(int src_is_linear, int dst_is_linear);
static void pixblt_b_1_opx(int dst_is_linear);
static void pixblt_b_2_opx(int dst_is_linear);
static void pixblt_b_4_opx(int dst_is_linear);
static void pixblt_b_8_opx(int dst_is_linear);
static void pixblt_b_16_opx(int dst_is_linear);
static void fill_1_opx(int dst_is_linear);
static void fill_2_opx(int dst_is_linear);
static void fill_4_opx(int dst_is_linear);
static void fill_8_opx(int dst_is_linear);
static void fill_16_opx(int dst_is_linear);

static void pixblt_1_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_2_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_4_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_8_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_16_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_1_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_2_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_4_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_8_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_r_16_opx_trans(int src_is_linear, int dst_is_linear);
static void pixblt_b_1_opx_trans(int dst_is_linear);
static void pixblt_b_2_opx_trans(int dst_is_linear);
static void pixblt_b_4_opx_trans(int dst_is_linear);
static void pixblt_b_8_opx_trans(int dst_is_linear);
static void pixblt_b_16_opx_trans(int dst_is_linear);
static void fill_1_opx_trans(int dst_is_linear);
static void fill_2_opx_trans(int dst_is_linear);
static void fill_4_opx_trans(int dst_is_linear);
static void fill_8_opx_trans(int dst_is_linear);
static void fill_16_opx_trans(int dst_is_linear);


/* tables */
static void (*const pixblt_op_table[])(int, int) =
{
	pixblt_1_op0,	pixblt_1_op0_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,
	pixblt_1_opx,	pixblt_1_opx_trans,		pixblt_1_opx,	pixblt_1_opx_trans,

	pixblt_2_op0,	pixblt_2_op0_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,
	pixblt_2_opx,	pixblt_2_opx_trans,		pixblt_2_opx,	pixblt_2_opx_trans,

	pixblt_4_op0,	pixblt_4_op0_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,
	pixblt_4_opx,	pixblt_4_opx_trans,		pixblt_4_opx,	pixblt_4_opx_trans,

	pixblt_8_op0,	pixblt_8_op0_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,
	pixblt_8_opx,	pixblt_8_opx_trans,		pixblt_8_opx,	pixblt_8_opx_trans,

	pixblt_16_op0,	pixblt_16_op0_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans,
	pixblt_16_opx,	pixblt_16_opx_trans,	pixblt_16_opx,	pixblt_16_opx_trans
};

static void (*const pixblt_r_op_table[])(int, int) =
{
	pixblt_r_1_op0,	pixblt_r_1_op0_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,
	pixblt_r_1_opx,	pixblt_r_1_opx_trans,	pixblt_r_1_opx,	pixblt_r_1_opx_trans,

	pixblt_r_2_op0,	pixblt_r_2_op0_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,
	pixblt_r_2_opx,	pixblt_r_2_opx_trans,	pixblt_r_2_opx,	pixblt_r_2_opx_trans,

	pixblt_r_4_op0,	pixblt_r_4_op0_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,
	pixblt_r_4_opx,	pixblt_r_4_opx_trans,	pixblt_r_4_opx,	pixblt_r_4_opx_trans,

	pixblt_r_8_op0,	pixblt_r_8_op0_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,
	pixblt_r_8_opx,	pixblt_r_8_opx_trans,	pixblt_r_8_opx,	pixblt_r_8_opx_trans,

	pixblt_r_16_op0,pixblt_r_16_op0_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans,
	pixblt_r_16_opx,pixblt_r_16_opx_trans,	pixblt_r_16_opx,pixblt_r_16_opx_trans
};

static void (*const pixblt_b_op_table[])(int) =
{
	pixblt_b_1_op0,	pixblt_b_1_op0_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,
	pixblt_b_1_opx,	pixblt_b_1_opx_trans,	pixblt_b_1_opx,	pixblt_b_1_opx_trans,

	pixblt_b_2_op0,	pixblt_b_2_op0_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,
	pixblt_b_2_opx,	pixblt_b_2_opx_trans,	pixblt_b_2_opx,	pixblt_b_2_opx_trans,

	pixblt_b_4_op0,	pixblt_b_4_op0_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,
	pixblt_b_4_opx,	pixblt_b_4_opx_trans,	pixblt_b_4_opx,	pixblt_b_4_opx_trans,

	pixblt_b_8_op0,	pixblt_b_8_op0_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,
	pixblt_b_8_opx,	pixblt_b_8_opx_trans,	pixblt_b_8_opx,	pixblt_b_8_opx_trans,

	pixblt_b_16_op0,pixblt_b_16_op0_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans,
	pixblt_b_16_opx,pixblt_b_16_opx_trans,	pixblt_b_16_opx,pixblt_b_16_opx_trans
};

static void (*const fill_op_table[])(int) =
{
	fill_1_op0,		fill_1_op0_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,
	fill_1_opx,		fill_1_opx_trans,		fill_1_opx,		fill_1_opx_trans,

	fill_2_op0,		fill_2_op0_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,
	fill_2_opx,		fill_2_opx_trans,		fill_2_opx,		fill_2_opx_trans,

	fill_4_op0,		fill_4_op0_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,
	fill_4_opx,		fill_4_opx_trans,		fill_4_opx,		fill_4_opx_trans,

	fill_8_op0,		fill_8_op0_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,
	fill_8_opx,		fill_8_opx_trans,		fill_8_opx,		fill_8_opx_trans,

	fill_16_op0,	fill_16_op0_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans,
	fill_16_opx,	fill_16_opx_trans,		fill_16_opx,	fill_16_opx_trans
};


#define RECURSIVE_INCLUDE

/* non-transparent replace ops */
#define PIXEL_OP(src, mask, pixel) 		pixel = pixel
#define PIXEL_OP_TIMING			 		2
#define PIXEL_OP_REQUIRES_SOURCE 		0
#define TRANSPARENCY					0

	/* 1bpp cases */
	#define BITS_PER_PIXEL					1
	#define FUNCTION_NAME(base)				base##_1_op0
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 2bpp cases */
	#define BITS_PER_PIXEL					2
	#define FUNCTION_NAME(base)				base##_2_op0
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 4bpp cases */
	#define BITS_PER_PIXEL					4
	#define FUNCTION_NAME(base)				base##_4_op0
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 8bpp cases */
	#define BITS_PER_PIXEL					8
	#define FUNCTION_NAME(base)				base##_8_op0
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 16bpp cases */
	#define BITS_PER_PIXEL					16
	#define FUNCTION_NAME(base)				base##_16_op0
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

#undef TRANSPARENCY
#undef PIXEL_OP_REQUIRES_SOURCE
#undef PIXEL_OP_TIMING
#undef PIXEL_OP


#define PIXEL_OP(src, mask, pixel) 		pixel = (*pixel_op)(src, mask, pixel)
#define PIXEL_OP_TIMING			 		pixel_op_timing
#define PIXEL_OP_REQUIRES_SOURCE 		1
#define TRANSPARENCY					0

	/* 1bpp cases */
	#define BITS_PER_PIXEL					1
	#define FUNCTION_NAME(base)				base##_1_opx
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 2bpp cases */
	#define BITS_PER_PIXEL					2
	#define FUNCTION_NAME(base)				base##_2_opx
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 4bpp cases */
	#define BITS_PER_PIXEL					4
	#define FUNCTION_NAME(base)				base##_4_opx
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 8bpp cases */
	#define BITS_PER_PIXEL					8
	#define FUNCTION_NAME(base)				base##_8_opx
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 16bpp cases */
	#define BITS_PER_PIXEL					16
	#define FUNCTION_NAME(base)				base##_16_opx
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

#undef TRANSPARENCY
#undef PIXEL_OP_REQUIRES_SOURCE
#undef PIXEL_OP_TIMING
#undef PIXEL_OP


/* transparent replace ops */
#define PIXEL_OP(src, mask, pixel) 		pixel = pixel
#define PIXEL_OP_REQUIRES_SOURCE 		0
#define PIXEL_OP_TIMING			 		4
#define TRANSPARENCY					1

	/* 1bpp cases */
	#define BITS_PER_PIXEL					1
	#define FUNCTION_NAME(base)				base##_1_op0_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 2bpp cases */
	#define BITS_PER_PIXEL					2
	#define FUNCTION_NAME(base)				base##_2_op0_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 4bpp cases */
	#define BITS_PER_PIXEL					4
	#define FUNCTION_NAME(base)				base##_4_op0_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 8bpp cases */
	#define BITS_PER_PIXEL					8
	#define FUNCTION_NAME(base)				base##_8_op0_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 16bpp cases */
	#define BITS_PER_PIXEL					16
	#define FUNCTION_NAME(base)				base##_16_op0_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

#undef TRANSPARENCY
#undef PIXEL_OP_REQUIRES_SOURCE
#undef PIXEL_OP_TIMING
#undef PIXEL_OP


#define PIXEL_OP(src, mask, pixel) 		pixel = (*pixel_op)(src, mask, pixel)
#define PIXEL_OP_REQUIRES_SOURCE 		1
#define PIXEL_OP_TIMING					(2+pixel_op_timing)
#define TRANSPARENCY					1

	/* 1bpp cases */
	#define BITS_PER_PIXEL					1
	#define FUNCTION_NAME(base)				base##_1_opx_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 2bpp cases */
	#define BITS_PER_PIXEL					2
	#define FUNCTION_NAME(base)				base##_2_opx_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 4bpp cases */
	#define BITS_PER_PIXEL					4
	#define FUNCTION_NAME(base)				base##_4_opx_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 8bpp cases */
	#define BITS_PER_PIXEL					8
	#define FUNCTION_NAME(base)				base##_8_opx_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

	/* 16bpp cases */
	#define BITS_PER_PIXEL					16
	#define FUNCTION_NAME(base)				base##_16_opx_trans
	#include "34010gfx.c"
	#undef FUNCTION_NAME
	#undef BITS_PER_PIXEL

#undef TRANSPARENCY
#undef PIXEL_OP_REQUIRES_SOURCE
#undef PIXEL_OP_TIMING
#undef PIXEL_OP

static const UINT8 pixelsize_lookup[32] =
{
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
};


static void pixblt_b_l(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT B,L (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	(*pixblt_b_op_table[ix])(1);
}

static void pixblt_b_xy(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT B,XY (%d,%d) (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DADDR_X, DADDR_Y, DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	(*pixblt_b_op_table[ix])(0);
}

static void pixblt_l_l(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int pbh = (IOREG(REG_CONTROL) >> 8) & 1;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT L,L (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	if (!pbh)
		(*pixblt_op_table[ix])(1, 1);
	else
		(*pixblt_r_op_table[ix])(1, 1);
}

static void pixblt_l_xy(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int pbh = (IOREG(REG_CONTROL) >> 8) & 1;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT L,XY (%d,%d) (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DADDR_X, DADDR_Y, DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	if (!pbh)
		(*pixblt_op_table[ix])(1, 0);
	else
		(*pixblt_r_op_table[ix])(1, 0);
}

static void pixblt_xy_l(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int pbh = (IOREG(REG_CONTROL) >> 8) & 1;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT XY,L (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	if (!pbh)
		(*pixblt_op_table[ix])(0, 1);
	else
		(*pixblt_r_op_table[ix])(0, 1);
}

static void pixblt_xy_xy(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int pbh = (IOREG(REG_CONTROL) >> 8) & 1;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):PIXBLT XY,XY (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	if (!pbh)
		(*pixblt_op_table[ix])(0, 0);
	else
		(*pixblt_r_op_table[ix])(0, 0);
}

static void fill_l(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):FILL L (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	(*fill_op_table[ix])(1);
}

static void fill_xy(void)
{
	int psize = pixelsize_lookup[IOREG(REG_PSIZE) & 0x1f];
	int trans = (IOREG(REG_CONTROL) & 0x20) >> 5;
	int rop = (IOREG(REG_CONTROL) >> 10) & 0x1f;
	int ix = trans | (rop << 1) | (psize << 6);
	if (!P_FLAG) LOGGFX(("%08X(%3d):FILL XY (%d,%d) (%dx%d) depth=%d\n", PC, video_screen_get_vpos(tms.screen), DADDR_X, DADDR_Y, DYDX_X, DYDX_Y, IOREG(REG_PSIZE) ? IOREG(REG_PSIZE) : 32));
	pixel_op = pixel_op_table[rop];
	pixel_op_timing = pixel_op_timing_table[rop];
	(*fill_op_table[ix])(0);
}


#else


#undef PIXELS_PER_WORD
#undef PIXEL_MASK

#define PIXELS_PER_WORD (16 / BITS_PER_PIXEL)
#define PIXEL_MASK ((1 << BITS_PER_PIXEL) - 1)

static void FUNCTION_NAME(pixblt)(int src_is_linear, int dst_is_linear)
{
	/* if this is the first time through, perform the operation */
	if (!P_FLAG)
	{
		int dx, dy, x, y, /*words,*/ yreverse;
		void (*word_write)(offs_t address,UINT16 data);
		UINT16 (*word_read)(offs_t address);
		UINT32 readwrites = 0;
		UINT32 saddr, daddr;
		XY dstxy = { 0 };

		/* determine read/write functions */
		if (IOREG(REG_DPYCTL) & 0x0800)
		{
			word_write = shiftreg_w;
			word_read = shiftreg_r;
		}
		else
		{
			word_write = program_write_word_16le;
			word_read = program_read_word_16le;
		}

		/* compute the starting addresses */
		saddr = src_is_linear ? SADDR : SXYTOL(SADDR_XY);

		/* compute the bounds of the operation */
		dx = (INT16)DYDX_X;
		dy = (INT16)DYDX_Y;

		/* apply the window for non-linear destinations */
		tms.gfxcycles = 7 + (src_is_linear ? 0 : 2);
		if (!dst_is_linear)
		{
			dstxy = DADDR_XY;
			tms.gfxcycles += 2 + (!src_is_linear) + apply_window("PIXBLT", BITS_PER_PIXEL, &saddr, &dstxy, &dx, &dy);
			daddr = DXYTOL(dstxy);
		}
		else
			daddr = DADDR;
		daddr &= ~(BITS_PER_PIXEL - 1);
		LOGGFX(("  saddr=%08X daddr=%08X sptch=%08X dptch=%08X\n", saddr, daddr, SPTCH, DPTCH));

		/* bail if we're clipped */
		if (dx <= 0 || dy <= 0)
			return;

		/* window mode 1: just return and interrupt if we are within the window */
		if (WINDOW_CHECKING == 1 && !dst_is_linear)
		{
			CLR_V;
			DADDR_XY = dstxy;
			DYDX_X = dx;
			DYDX_Y = dy;
			IOREG(REG_INTPEND) |= TMS34010_WV;
			check_interrupt();
			return;
		}

		/* handle flipping the addresses */
		yreverse = (IOREG(REG_CONTROL) >> 9) & 1;
		if (!src_is_linear || !dst_is_linear)
			if (yreverse)
			{
				saddr += (dy - 1) * SPTCH;
				daddr += (dy - 1) * DPTCH;
			}

		tms.st |= STBIT_P;

		/* loop over rows */
		for (y = 0; y < dy; y++)
		{
			UINT32 srcwordaddr = saddr >> 4;
			UINT32 dstwordaddr = daddr >> 4;
			UINT8 srcbit = saddr & 15;
			UINT8 dstbit = daddr & 15;
			UINT32 srcword, dstword = 0;

			/* fetch the initial source word */
			srcword = (*word_read)(srcwordaddr++ << 1);
			readwrites++;

			/* fetch the initial dest word */
			if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY || (daddr & 0x0f) != 0)
			{
				dstword = (*word_read)(dstwordaddr << 1);
				readwrites++;
			}

			/* loop over pixels */
			for (x = 0; x < dx; x++)
			{
				UINT32 dstmask;
				UINT32 pixel;

				/* fetch more words if necessary */
				if (srcbit + BITS_PER_PIXEL > 16)
				{
					srcword |= (*word_read)(srcwordaddr++ << 1) << 16;
					readwrites++;
				}

				/* extract pixel from source */
				pixel = (srcword >> srcbit) & PIXEL_MASK;
				srcbit += BITS_PER_PIXEL;
				if (srcbit > 16)
				{
					srcbit -= 16;
					srcword >>= 16;
				}

				/* fetch additional destination word if necessary */
				if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY)
					if (dstbit + BITS_PER_PIXEL > 16)
					{
						dstword |= (*word_read)((dstwordaddr + 1) << 1) << 16;
						readwrites++;
					}

				/* apply pixel operations */
				pixel <<= dstbit;
				dstmask = PIXEL_MASK << dstbit;
				PIXEL_OP(dstword, dstmask, pixel);
				if (!TRANSPARENCY || pixel != 0)
					dstword = (dstword & ~dstmask) | pixel;

				/* flush destination words */
				dstbit += BITS_PER_PIXEL;
				if (dstbit > 16)
				{
					(*word_write)(dstwordaddr++ << 1, dstword);
					readwrites++;
					dstbit -= 16;
					dstword >>= 16;
				}
			}

			/* flush any remaining words */
			if (dstbit > 0)
			{
				/* if we're right-partial, read and mask the remaining bits */
				if (dstbit != 16)
				{
					UINT16 origdst = (*word_read)(dstwordaddr << 1);
					UINT16 mask = 0xffff << dstbit;
					dstword = (dstword & ~mask) | (origdst & mask);
					readwrites++;
				}

				(*word_write)(dstwordaddr++ << 1, dstword);
				readwrites++;
			}



#if 0
			int left_partials, right_partials, full_words, bitshift, bitshift_alt;
			UINT16 srcword, srcmask, dstword, dstmask, pixel;
			UINT32 swordaddr, dwordaddr;

			/* determine the bit shift to get from source to dest */
			bitshift = ((daddr & 15) - (saddr & 15)) & 15;
			bitshift_alt = (16 - bitshift) & 15;

			/* how many left and right partial pixels do we have? */
			left_partials = (PIXELS_PER_WORD - ((daddr & 15) / BITS_PER_PIXEL)) & (PIXELS_PER_WORD - 1);
			right_partials = ((daddr + dx * BITS_PER_PIXEL) & 15) / BITS_PER_PIXEL;
			full_words = dx - left_partials - right_partials;
			if (full_words < 0)
				left_partials = dx, right_partials = full_words = 0;
			else
				full_words /= PIXELS_PER_WORD;

			/* compute cycles */
			tms.gfxcycles += compute_pixblt_cycles(left_partials, right_partials, full_words, PIXEL_OP_TIMING);

			/* use word addresses each row */
			swordaddr = saddr >> 4;
			dwordaddr = daddr >> 4;

			/* fetch the initial source word */
			srcword = (*word_read)(swordaddr++ << 1);
			srcmask = PIXEL_MASK << (saddr & 15);

			/* handle the left partial word */
			if (left_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK << (daddr & 15);

				/* loop over partials */
				for (x = 0; x < left_partials; x++)
				{
					/* fetch another word if necessary */
					if (srcmask == 0)
					{
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = PIXEL_MASK;
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= BITS_PER_PIXEL;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* loop over full words */
			for (words = 0; words < full_words; words++)
			{
				/* fetch the destination word (if necessary) */
				if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY)
					dstword = (*word_read)(dwordaddr << 1);
				else
					dstword = 0;
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < PIXELS_PER_WORD; x++)
				{
					/* fetch another word if necessary */
					if (srcmask == 0)
					{
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = PIXEL_MASK;
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= BITS_PER_PIXEL;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* handle the right partial word */
			if (right_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < right_partials; x++)
				{
					/* fetch another word if necessary */
					if (srcmask == 0)
					{
		LOGGFX(("  right fetch @ %08x\n", swordaddr));
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = PIXEL_MASK;
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= BITS_PER_PIXEL;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}
#endif

			/* update for next row */
			if (!yreverse)
			{
				saddr += SPTCH;
				daddr += DPTCH;
			}
			else
			{
				saddr -= SPTCH;
				daddr -= DPTCH;
			}
		}

		tms.gfxcycles += readwrites * 2 + dx * dy * (PIXEL_OP_TIMING - 2);

		LOGGFX(("  (%d cycles)\n", tms.gfxcycles));
	}

	/* eat cycles */
	if (tms.gfxcycles > tms34010_ICount)
	{
		tms.gfxcycles -= tms34010_ICount;
		tms34010_ICount = 0;
		PC -= 0x10;
	}
	else
	{
		tms34010_ICount -= tms.gfxcycles;
		tms.st &= ~STBIT_P;
		if (src_is_linear && dst_is_linear)
			SADDR += DYDX_Y * SPTCH;
		else if (src_is_linear)
			SADDR += DYDX_Y * SPTCH;
		else
			SADDR_Y += DYDX_Y;
		if (dst_is_linear)
			DADDR += DYDX_Y * DPTCH;
		else
			DADDR_Y += DYDX_Y;
	}
}

static void FUNCTION_NAME(pixblt_r)(int src_is_linear, int dst_is_linear)
{
	/* if this is the first time through, perform the operation */
	if (!P_FLAG)
	{
		int dx, dy, x, y, words, yreverse;
		void (*word_write)(offs_t address,UINT16 data);
		UINT16 (*word_read)(offs_t address);
		UINT32 saddr, daddr;
		XY dstxy = { 0 };

		/* determine read/write functions */
		if (IOREG(REG_DPYCTL) & 0x0800)
		{
			word_write = shiftreg_w;
			word_read = shiftreg_r;
		}
		else
		{
			word_write = program_write_word_16le;
			word_read = program_read_word_16le;
		}

		/* compute the starting addresses */
		saddr = src_is_linear ? SADDR : SXYTOL(SADDR_XY);
if ((saddr & (BITS_PER_PIXEL - 1)) != 0) mame_printf_debug("PIXBLT_R%d with odd saddr\n", BITS_PER_PIXEL);
		saddr &= ~(BITS_PER_PIXEL - 1);

		/* compute the bounds of the operation */
		dx = (INT16)DYDX_X;
		dy = (INT16)DYDX_Y;

		/* apply the window for non-linear destinations */
		tms.gfxcycles = 7 + (src_is_linear ? 0 : 2);
		if (!dst_is_linear)
		{
			dstxy = DADDR_XY;
			tms.gfxcycles += 2 + (!src_is_linear) + apply_window("PIXBLT R", BITS_PER_PIXEL, &saddr, &dstxy, &dx, &dy);
			daddr = DXYTOL(dstxy);
		}
		else
			daddr = DADDR;
if ((daddr & (BITS_PER_PIXEL - 1)) != 0) mame_printf_debug("PIXBLT_R%d with odd daddr\n", BITS_PER_PIXEL);
		daddr &= ~(BITS_PER_PIXEL - 1);
		LOGGFX(("  saddr=%08X daddr=%08X sptch=%08X dptch=%08X\n", saddr, daddr, SPTCH, DPTCH));

		/* bail if we're clipped */
		if (dx <= 0 || dy <= 0)
			return;

		/* window mode 1: just return and interrupt if we are within the window */
		if (WINDOW_CHECKING == 1 && !dst_is_linear)
		{
			CLR_V;
			DADDR_XY = dstxy;
			DYDX_X = dx;
			DYDX_Y = dy;
			IOREG(REG_INTPEND) |= TMS34010_WV;
			check_interrupt();
			return;
		}

		/* handle flipping the addresses */
		yreverse = (IOREG(REG_CONTROL) >> 9) & 1;
		if (!src_is_linear || !dst_is_linear)
		{
			saddr += dx * BITS_PER_PIXEL;
			daddr += dx * BITS_PER_PIXEL;
			if (yreverse)
			{
				saddr += (dy - 1) * SPTCH;
				daddr += (dy - 1) * DPTCH;
			}
		}

		tms.st |= STBIT_P;

		/* loop over rows */
		for (y = 0; y < dy; y++)
		{
			int left_partials, right_partials, full_words, bitshift, bitshift_alt;
			UINT16 srcword, srcmask, dstword, dstmask, pixel;
			UINT32 swordaddr, dwordaddr;

			/* determine the bit shift to get from source to dest */
			bitshift = ((daddr & 15) - (saddr & 15)) & 15;
			bitshift_alt = (16 - bitshift) & 15;

			/* how many left and right partial pixels do we have? */
			left_partials = (PIXELS_PER_WORD - (((daddr - dx * BITS_PER_PIXEL) & 15) / BITS_PER_PIXEL)) & (PIXELS_PER_WORD - 1);
			right_partials = (daddr & 15) / BITS_PER_PIXEL;
			full_words = dx - left_partials - right_partials;
			if (full_words < 0)
				right_partials = dx, left_partials = full_words = 0;
			else
				full_words /= PIXELS_PER_WORD;

			/* compute cycles */
			tms.gfxcycles += compute_pixblt_cycles(left_partials, right_partials, full_words, PIXEL_OP_TIMING);

			/* use word addresses each row */
			swordaddr = (saddr + 15) >> 4;
			dwordaddr = (daddr + 15) >> 4;

			/* fetch the initial source word */
			srcword = (*word_read)(--swordaddr << 1);
			srcmask = PIXEL_MASK << ((saddr - BITS_PER_PIXEL) & 15);

			/* handle the right partial word */
			if (right_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(--dwordaddr << 1);
				dstmask = PIXEL_MASK << ((daddr - BITS_PER_PIXEL) & 15);

				/* loop over partials */
				for (x = 0; x < right_partials; x++)
				{
					/* fetch source pixel if necessary */
					if (srcmask == 0)
					{
						srcword = (*word_read)(--swordaddr << 1);
						srcmask = PIXEL_MASK << (16 - BITS_PER_PIXEL);
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask >>= BITS_PER_PIXEL;

					/* update the destination */
					dstmask >>= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr << 1, dstword);
			}

			/* loop over full words */
			for (words = 0; words < full_words; words++)
			{
				/* fetch the destination word (if necessary) */
				dwordaddr--;
				if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY)
					dstword = (*word_read)(dwordaddr << 1);
				else
					dstword = 0;
				dstmask = PIXEL_MASK << (16 - BITS_PER_PIXEL);

				/* loop over partials */
				for (x = 0; x < PIXELS_PER_WORD; x++)
				{
					/* fetch source pixel if necessary */
					if (srcmask == 0)
					{
						srcword = (*word_read)(--swordaddr << 1);
						srcmask = PIXEL_MASK << (16 - BITS_PER_PIXEL);
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask >>= BITS_PER_PIXEL;

					/* update the destination */
					dstmask >>= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr << 1, dstword);
			}

			/* handle the left partial word */
			if (left_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(--dwordaddr << 1);
				dstmask = PIXEL_MASK << (16 - BITS_PER_PIXEL);

				/* loop over partials */
				for (x = 0; x < left_partials; x++)
				{
					/* fetch the source pixel if necessary */
					if (srcmask == 0)
					{
						srcword = (*word_read)(--swordaddr << 1);
						srcmask = PIXEL_MASK << (16 - BITS_PER_PIXEL);
					}

					/* process the pixel */
					pixel = srcword & srcmask;
					if (dstmask > srcmask)
						pixel <<= bitshift;
					else
						pixel >>= bitshift_alt;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask >>= BITS_PER_PIXEL;

					/* update the destination */
					dstmask >>= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr << 1, dstword);
			}

			/* update for next row */
			if (!yreverse)
			{
				saddr += SPTCH;
				daddr += DPTCH;
			}
			else
			{
				saddr -= SPTCH;
				daddr -= DPTCH;
			}
		}
		LOGGFX(("  (%d cycles)\n", tms.gfxcycles));
	}

	/* eat cycles */
	if (tms.gfxcycles > tms34010_ICount)
	{
		tms.gfxcycles -= tms34010_ICount;
		tms34010_ICount = 0;
		PC -= 0x10;
	}
	else
	{
		tms34010_ICount -= tms.gfxcycles;
		tms.st &= ~STBIT_P;
		if (src_is_linear && dst_is_linear)
			SADDR += DYDX_Y * SPTCH;
		else if (src_is_linear)
			SADDR += DYDX_Y * SPTCH;
		else
			SADDR_Y += DYDX_Y;
		if (dst_is_linear)
			DADDR += DYDX_Y * DPTCH;
		else
			DADDR_Y += DYDX_Y;
	}
}

static void FUNCTION_NAME(pixblt_b)(int dst_is_linear)
{
	/* if this is the first time through, perform the operation */
	if (!P_FLAG)
	{
		int dx, dy, x, y, words, left_partials, right_partials, full_words;
		void (*word_write)(offs_t address,UINT16 data);
		UINT16 (*word_read)(offs_t address);
		UINT32 saddr, daddr;
		XY dstxy = { 0 };

		/* determine read/write functions */
		if (IOREG(REG_DPYCTL) & 0x0800)
		{
			word_write = shiftreg_w;
			word_read = shiftreg_r;
		}
		else
		{
			word_write = program_write_word_16le;
			word_read = program_read_word_16le;
		}

		/* compute the starting addresses */
		saddr = SADDR;

		/* compute the bounds of the operation */
		dx = (INT16)DYDX_X;
		dy = (INT16)DYDX_Y;

		/* apply the window for non-linear destinations */
		tms.gfxcycles = 4;
		if (!dst_is_linear)
		{
			dstxy = DADDR_XY;
			tms.gfxcycles += 2 + apply_window("PIXBLT B", 1, &saddr, &dstxy, &dx, &dy);
			daddr = DXYTOL(dstxy);
		}
		else
			daddr = DADDR;
		daddr &= ~(BITS_PER_PIXEL - 1);
		LOGGFX(("  saddr=%08X daddr=%08X sptch=%08X dptch=%08X\n", saddr, daddr, SPTCH, DPTCH));

		/* bail if we're clipped */
		if (dx <= 0 || dy <= 0)
			return;

		/* window mode 1: just return and interrupt if we are within the window */
		if (WINDOW_CHECKING == 1 && !dst_is_linear)
		{
			CLR_V;
			DADDR_XY = dstxy;
			DYDX_X = dx;
			DYDX_Y = dy;
			IOREG(REG_INTPEND) |= TMS34010_WV;
			check_interrupt();
			return;
		}

		/* how many left and right partial pixels do we have? */
		left_partials = (PIXELS_PER_WORD - ((daddr & 15) / BITS_PER_PIXEL)) & (PIXELS_PER_WORD - 1);
		right_partials = ((daddr + dx * BITS_PER_PIXEL) & 15) / BITS_PER_PIXEL;
		full_words = dx - left_partials - right_partials;
		if (full_words < 0)
			left_partials = dx, right_partials = full_words = 0;
		else
			full_words /= PIXELS_PER_WORD;

		/* compute cycles */
		tms.gfxcycles += compute_pixblt_b_cycles(left_partials, right_partials, full_words, dy, PIXEL_OP_TIMING, BITS_PER_PIXEL);
		tms.st |= STBIT_P;

		/* loop over rows */
		for (y = 0; y < dy; y++)
		{
			UINT16 srcword, srcmask, dstword, dstmask, pixel;
			UINT32 swordaddr, dwordaddr;

			/* use byte addresses each row */
			swordaddr = saddr >> 4;
			dwordaddr = daddr >> 4;

			/* fetch the initial source word */
			srcword = (*word_read)(swordaddr++ << 1);
			srcmask = 1 << (saddr & 15);

			/* handle the left partial word */
			if (left_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK << (daddr & 15);

				/* loop over partials */
				for (x = 0; x < left_partials; x++)
				{
					/* process the pixel */
					pixel = (srcword & srcmask) ? COLOR1 : COLOR0;
					pixel &= dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= 1;
					if (srcmask == 0)
					{
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = 0x0001;
					}

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* loop over full words */
			for (words = 0; words < full_words; words++)
			{
				/* fetch the destination word (if necessary) */
				if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY)
					dstword = (*word_read)(dwordaddr << 1);
				else
					dstword = 0;
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < PIXELS_PER_WORD; x++)
				{
					/* process the pixel */
					pixel = (srcword & srcmask) ? COLOR1 : COLOR0;
					pixel &= dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= 1;
					if (srcmask == 0)
					{
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = 0x0001;
					}

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* handle the right partial word */
			if (right_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < right_partials; x++)
				{
					/* process the pixel */
					pixel = (srcword & srcmask) ? COLOR1 : COLOR0;
					pixel &= dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the source */
					srcmask <<= 1;
					if (srcmask == 0)
					{
						srcword = (*word_read)(swordaddr++ << 1);
						srcmask = 0x0001;
					}

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* update for next row */
			saddr += SPTCH;
			daddr += DPTCH;
		}
		LOGGFX(("  (%d cycles)\n", tms.gfxcycles));
	}

	/* eat cycles */
	if (tms.gfxcycles > tms34010_ICount)
	{
		tms.gfxcycles -= tms34010_ICount;
		tms34010_ICount = 0;
		PC -= 0x10;
	}
	else
	{
		tms34010_ICount -= tms.gfxcycles;
		tms.st &= ~STBIT_P;
		SADDR += DYDX_Y * SPTCH;
		if (dst_is_linear)
			DADDR += DYDX_Y * DPTCH;
		else
			DADDR_Y += DYDX_Y;
	}
}

static void FUNCTION_NAME(fill)(int dst_is_linear)
{
	/* if this is the first time through, perform the operation */
	if (!P_FLAG)
	{
		int dx, dy, x, y, words, left_partials, right_partials, full_words;
		void (*word_write)(offs_t address,UINT16 data);
		UINT16 (*word_read)(offs_t address);
		UINT32 daddr;
		XY dstxy = { 0 };

		/* determine read/write functions */
		if (IOREG(REG_DPYCTL) & 0x0800)
		{
			word_write = shiftreg_w;
			word_read = dummy_shiftreg_r;
		}
		else
		{
			word_write = program_write_word_16le;
			word_read = program_read_word_16le;
		}

		/* compute the bounds of the operation */
		dx = (INT16)DYDX_X;
		dy = (INT16)DYDX_Y;

		/* apply the window for non-linear destinations */
		tms.gfxcycles = 4;
		if (!dst_is_linear)
		{
			dstxy = DADDR_XY;
			tms.gfxcycles += 2 + apply_window("FILL", 0, NULL, &dstxy, &dx, &dy);
			daddr = DXYTOL(dstxy);
		}
		else
			daddr = DADDR;
		daddr &= ~(BITS_PER_PIXEL - 1);
		LOGGFX(("  daddr=%08X\n", daddr));

		/* bail if we're clipped */
		if (dx <= 0 || dy <= 0)
			return;

		/* window mode 1: just return and interrupt if we are within the window */
		if (WINDOW_CHECKING == 1 && !dst_is_linear)
		{
			CLR_V;
			DADDR_XY = dstxy;
			DYDX_X = dx;
			DYDX_Y = dy;
			IOREG(REG_INTPEND) |= TMS34010_WV;
			check_interrupt();
			return;
		}

		/* how many left and right partial pixels do we have? */
		left_partials = (PIXELS_PER_WORD - ((daddr & 15) / BITS_PER_PIXEL)) & (PIXELS_PER_WORD - 1);
		right_partials = ((daddr + dx * BITS_PER_PIXEL) & 15) / BITS_PER_PIXEL;
		full_words = dx - left_partials - right_partials;
		if (full_words < 0)
			left_partials = dx, right_partials = full_words = 0;
		else
			full_words /= PIXELS_PER_WORD;

		/* compute cycles */
		tms.gfxcycles += 2;
		tms.st |= STBIT_P;

		/* loop over rows */
		for (y = 0; y < dy; y++)
		{
			UINT16 dstword, dstmask, pixel;
			UINT32 dwordaddr;

			/* use byte addresses each row */
			dwordaddr = daddr >> 4;

			/* compute cycles */
			tms.gfxcycles += compute_fill_cycles(left_partials, right_partials, full_words, PIXEL_OP_TIMING);

			/* handle the left partial word */
			if (left_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK << (daddr & 15);

				/* loop over partials */
				for (x = 0; x < left_partials; x++)
				{
					/* process the pixel */
					pixel = COLOR1 & dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* loop over full words */
			for (words = 0; words < full_words; words++)
			{
				/* fetch the destination word (if necessary) */
				if (PIXEL_OP_REQUIRES_SOURCE || TRANSPARENCY)
					dstword = (*word_read)(dwordaddr << 1);
				else
					dstword = 0;
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < PIXELS_PER_WORD; x++)
				{
					/* process the pixel */
					pixel = COLOR1 & dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* handle the right partial word */
			if (right_partials != 0)
			{
				/* fetch the destination word */
				dstword = (*word_read)(dwordaddr << 1);
				dstmask = PIXEL_MASK;

				/* loop over partials */
				for (x = 0; x < right_partials; x++)
				{
					/* process the pixel */
					pixel = COLOR1 & dstmask;
					PIXEL_OP(dstword, dstmask, pixel);
					if (!TRANSPARENCY || pixel != 0)
						dstword = (dstword & ~dstmask) | pixel;

					/* update the destination */
					dstmask <<= BITS_PER_PIXEL;
				}

				/* write the result */
				(*word_write)(dwordaddr++ << 1, dstword);
			}

			/* update for next row */
			daddr += DPTCH;
		}

		LOGGFX(("  (%d cycles)\n", tms.gfxcycles));
	}

	/* eat cycles */
	if (tms.gfxcycles > tms34010_ICount)
	{
		tms.gfxcycles -= tms34010_ICount;
		tms34010_ICount = 0;
		PC -= 0x10;
	}
	else
	{
		tms34010_ICount -= tms.gfxcycles;
		tms.st &= ~STBIT_P;
		if (dst_is_linear)
			DADDR += DYDX_Y * DPTCH;
		else
			DADDR_Y += DYDX_Y;
	}
}

#endif

