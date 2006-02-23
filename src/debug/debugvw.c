/*********************************************************************

    debugvw.c

    Debugger view engine.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "debugvw.h"
#include "debugcmd.h"
#include "debugcpu.h"
#include "debugcon.h"
#include "express.h"
#include <ctype.h>



/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

#define DEFAULT_DASM_LINES	(1000)
#define DEFAULT_DASM_WIDTH	(50)
#define DASM_MAX_BYTES		(16)



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/

/* debug_view_callbacks contains calbacks specific to a given view */
struct _debug_view_callbacks
{
	int				(*alloc)(debug_view *);		/* allocate memory */
	void			(*free)(debug_view *);		/* free memory */
	void			(*update)(debug_view *);	/* update contents */
	void			(*getprop)(debug_view *, UINT32, debug_property_info *value); /* get property */
	void			(*setprop)(debug_view *, UINT32, debug_property_info value); /* set property */
};
typedef struct _debug_view_callbacks debug_view_callbacks;


/* debug_view describes a single text-based view */
struct _debug_view
{
	debug_view *	next;						/* link to the next view */
	UINT8			type;						/* type of view */
	void *			extra_data;					/* extra view-specific data */
	debug_view_callbacks cb;					/* callback for this view */
	void *			osd_private_data;			/* OSD-managed private data */

	/* visibility info */
	UINT32			visible_rows;				/* number of currently visible rows */
	UINT32			visible_cols;				/* number of currently visible columns */
	UINT32			total_rows;					/* total number of rows */
	UINT32			total_cols;					/* total number of columns */
	UINT32			top_row;					/* current top row */
	UINT32			left_col;					/* current left column */
	UINT32			supports_cursor;			/* does this view support a cursor? */
	UINT32			cursor_visible;				/* is the cursor visible? */
	UINT32			cursor_row;					/* cursor row */
	UINT32			cursor_col;					/* cursor column */

	/* update info */
	UINT8			update_level;				/* update level; updates when this hits 0 */
	UINT8			update_pending;				/* true if there is a pending update */
	void			(*update_func)(debug_view *);/* callback for the update */
	debug_view_char *viewdata;					/* current array of view data */
	int				viewdata_size;				/* number of elements of the viewdata array */
};
/* typedef struct _debug_view debug_view -- defined in debugvw.h */


/* debug_view_registers contains data specific to a register view */
struct _debug_view_register
{
	UINT64			lastval;					/* last value */
	UINT64			currval;					/* current value */
	UINT32			regnum;						/* index */
	UINT8			tagstart;					/* starting tag char */
	UINT8			taglen;						/* number of tag chars */
	UINT8			valstart;					/* starting value char */
	UINT8			vallen;						/* number of value chars */
};
typedef struct _debug_view_register debug_view_register;


struct _debug_view_registers
{
	UINT8			recompute;					/* do we need to recompute the layout the next change? */
	UINT8			cpunum;						/* target CPU number */
	int				divider;					/* dividing column */
	UINT32			last_update;				/* execution counter at last update */
	debug_view_register reg[MAX_REGS];			/* register data */
};
typedef struct _debug_view_registers debug_view_registers;


/* debug_view_disasm contains data specific to a disassembly view */
struct _debug_view_disasm
{
	UINT8			recompute;					/* do we need to recompute the layout the next change? */
	UINT8			cpunum;						/* target CPU number */
	UINT8			display_raw;				/* display raw opcodes? */
	UINT8			display_raw_encrypted;		/* display encrypted opcodes? */
	UINT32			backwards_steps;			/* number of backwards steps */
	UINT32			dasm_width;					/* width of the disassembly area */
	UINT8 *			last_opcode_base;			/* last opcode base */
	UINT8 *			last_opcode_arg_base;		/* last opcode arg base */
	int				divider1, divider2;			/* left and right divider columns */
	UINT8			live_tracking;				/* track the value of the live expression? */
	UINT64			last_result;				/* last result from the expression */
	parsed_expression *expression;				/* expression to compute */
	char *			expression_string;			/* copy of the expression string */
	UINT8			expression_dirty;			/* true if the expression needs to be re-evaluated */
	UINT32			allocated_rows;				/* allocated rows */
	UINT32			allocated_cols;				/* allocated columns */
	offs_t *		address;					/* addresses of the instructions */
	char *			dasm;						/* disassembled instructions */
};
typedef struct _debug_view_disasm debug_view_disasm;


/* debug_view_memory contains data specific to a memory view */
struct _debug_view_memory
{
	UINT8			cpunum;						/* target CPU number */
	int				divider1, divider2;			/* left and right divider columns */
	UINT8			spacenum;					/* target address space */
	UINT8			bytes_per_chunk;			/* bytes per unit */
	UINT8			reverse_view;				/* reverse-endian view? */
	UINT8			ascii_view;					/* display ASCII characters? */
	UINT8			live_tracking;				/* track the value of the live expression? */
	UINT8			byte_offset;				/* byte offset within each row */
	UINT64			last_result;				/* last result from the expression */
	parsed_expression *expression;				/* expression to compute */
	char *			expression_string;			/* copy of the expression string */
	UINT8			expression_dirty;			/* true if the expression needs to be re-evaluated */
};
typedef struct _debug_view_memory debug_view_memory;



/*###################################################################################################
**  LOCAL VARIABLES
**#################################################################################################*/

static debug_view *first_view;



/*###################################################################################################
**  MACROS
**#################################################################################################*/



/*###################################################################################################
**  PROTOTYPES
**#################################################################################################*/

static void console_update(debug_view *view);

static int registers_alloc(debug_view *view);
static void registers_free(debug_view *view);
static void registers_update(debug_view *view);
static void	registers_getprop(debug_view *view, UINT32 property, debug_property_info *value);
static void	registers_setprop(debug_view *view, UINT32 property, debug_property_info value);

static int disasm_alloc(debug_view *view);
static void disasm_free(debug_view *view);
static void disasm_update(debug_view *view);
static void	disasm_getprop(debug_view *view, UINT32 property, debug_property_info *value);
static void	disasm_setprop(debug_view *view, UINT32 property, debug_property_info value);

static int memory_alloc(debug_view *view);
static void memory_free(debug_view *view);
static void memory_update(debug_view *view);
static void	memory_getprop(debug_view *view, UINT32 property, debug_property_info *value);
static void	memory_setprop(debug_view *view, UINT32 property, debug_property_info value);

static const debug_view_callbacks callback_table[] =
{
	{	NULL,				NULL,				NULL,				NULL,				NULL },
	{	NULL,				NULL,				console_update,		NULL,				NULL },
	{	registers_alloc,	registers_free,		registers_update,	registers_getprop,	registers_setprop },
	{	disasm_alloc,		disasm_free,		disasm_update,		disasm_getprop,		disasm_setprop },
	{	memory_alloc,		memory_free,		memory_update,		memory_getprop,		memory_setprop }
};



/*###################################################################################################
**  INITIALIZATION
**#################################################################################################*/

/*-------------------------------------------------
    debug_view_init - initializes the view system
-------------------------------------------------*/

void debug_view_init(void)
{
	/* reset the initial list */
	first_view = NULL;
	add_exit_callback(debug_view_exit);
}


/*-------------------------------------------------
    debug_view_exit - exits the view system
-------------------------------------------------*/

void debug_view_exit(void)
{
	/* kill all the views */
	while (first_view != NULL)
		debug_view_free(first_view);
}



/*###################################################################################################
**  VIEW CREATION/DELETION
**#################################################################################################*/

/*-------------------------------------------------
    debug_view_alloc - allocate a new debug
    view
-------------------------------------------------*/

debug_view *debug_view_alloc(int type)
{
	debug_view *view;

	/* allocate memory for the view */
	view = malloc(sizeof(*view));
	if (!view)
		return NULL;
	memset(view, 0, sizeof(*view));

	/* set the view type information */
	view->type = type;
	view->cb = callback_table[type];

	/* set up some reasonable defaults */
	view->visible_rows = 10;
	view->visible_cols = 10;
	view->total_rows = 10;
	view->total_cols = 10;

	/* allocate memory for the buffer */
	view->viewdata_size = view->visible_rows*view->visible_cols;
	view->viewdata = malloc(sizeof(view->viewdata[0]) * view->viewdata_size);
	if (!view->viewdata)
	{
		free(view);
		return NULL;
	}

	/* allocate extra memory */
	if (view->cb.alloc && !(*view->cb.alloc)(view))
	{
		free(view->viewdata);
		free(view);
		return NULL;
	}

	/* link it in */
	view->next = first_view;
	first_view = view;

	return view;
}


/*-------------------------------------------------
    debug_view_free - free a debug view
-------------------------------------------------*/

void debug_view_free(debug_view *view)
{
	debug_view *curview, *prevview;

	/* find the view */
	for (prevview = NULL, curview = first_view; curview != NULL; prevview = curview, curview = curview->next)
		if (curview == view)
		{
			/* unlink */
			if (prevview != NULL)
				prevview->next = curview->next;
			else
				first_view = curview->next;

			/* free memory */
			if (view->cb.free)
				(*view->cb.free)(view);
			if (view->viewdata)
				free(view->viewdata);
			free(view);
			break;
		}
}



/*###################################################################################################
**  PROPERTY MANAGEMENT
**#################################################################################################*/

/*-------------------------------------------------
    debug_view_get_property - return the value
    of a given property
-------------------------------------------------*/

void debug_view_get_property(debug_view *view, int property, debug_property_info *value)
{
	switch (property)
	{
		case DVP_VISIBLE_ROWS:
			value->i = view->visible_rows;
			break;

		case DVP_VISIBLE_COLS:
			value->i = view->visible_cols;
			break;

		case DVP_TOTAL_ROWS:
			value->i = view->total_rows;
			break;

		case DVP_TOTAL_COLS:
			value->i = view->total_cols;
			break;

		case DVP_TOP_ROW:
			value->i = view->top_row;
			break;

		case DVP_LEFT_COL:
			value->i = view->left_col;
			break;

		case DVP_UPDATE_CALLBACK:
			value->f = (genf *) view->update_func;
			break;

		case DVP_VIEW_DATA:
			value->p = (void *) view->viewdata;
			break;

		case DVP_SUPPORTS_CURSOR:
			value->i = view->supports_cursor;
			break;

		case DVP_CURSOR_VISIBLE:
			value->i = view->cursor_visible;
			break;

		case DVP_CURSOR_ROW:
			value->i = view->cursor_row;
			break;

		case DVP_CURSOR_COL:
			value->i = view->cursor_col;
			break;

		case DVP_OSD_PRIVATE:
			value->p = view->osd_private_data;
			break;

		default:
			if (view->cb.getprop)
				(*view->cb.getprop)(view, property, value);
			else
				osd_die("Attempt to get invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}


/*-------------------------------------------------
    debug_view_set_property - set the value
    of a given property
-------------------------------------------------*/

void debug_view_set_property(debug_view *view, int property, debug_property_info value)
{
	switch (property)
	{
		case DVP_VISIBLE_ROWS:
			if (value.i != view->visible_rows)
			{
				debug_view_begin_update(view);
				view->visible_rows = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_VISIBLE_COLS:
			if (value.i != view->visible_cols)
			{
				debug_view_begin_update(view);
				view->visible_cols = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_TOTAL_ROWS:
			if (value.i != view->total_rows)
			{
				debug_view_begin_update(view);
				view->total_rows = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_TOTAL_COLS:
			if (value.i != view->total_cols)
			{
				debug_view_begin_update(view);
				view->total_cols = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_TOP_ROW:
			if (value.i != view->top_row)
			{
				debug_view_begin_update(view);
				view->top_row = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_LEFT_COL:
			if (value.i != view->left_col)
			{
				debug_view_begin_update(view);
				view->left_col = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_UPDATE_CALLBACK:
			debug_view_begin_update(view);
			view->update_func = (void (*)(debug_view *)) value.f;
			view->update_pending = TRUE;
			debug_view_end_update(view);
			break;

		case DVP_VIEW_DATA:
			/* read-only */
			break;

		case DVP_SUPPORTS_CURSOR:
			/* read-only */
			break;

		case DVP_CURSOR_VISIBLE:
			if (value.i != view->cursor_visible)
			{
				debug_view_begin_update(view);
				view->cursor_visible = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_CURSOR_ROW:
			if (value.i != view->cursor_row)
			{
				debug_view_begin_update(view);
				view->cursor_row = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_CURSOR_COL:
			if (value.i != view->cursor_col)
			{
				debug_view_begin_update(view);
				view->cursor_col = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_OSD_PRIVATE:
			view->osd_private_data = value.p;
			break;

		default:
			if (view->cb.setprop)
				(*view->cb.setprop)(view, property, value);
			else
				osd_die("Attempt to set invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}



/*###################################################################################################
**  UPDATE MANAGEMENT
**#################################################################################################*/

/*-------------------------------------------------
    debug_view_begin_update - bracket a sequence
    of changes so that only one update occurs
-------------------------------------------------*/

void debug_view_begin_update(debug_view *view)
{
	/* bump the level */
	view->update_level++;
}


/*-------------------------------------------------
    debug_view_end_update - bracket a sequence
    of changes so that only one update occurs
-------------------------------------------------*/

void debug_view_end_update(debug_view *view)
{
	/* if we hit zero, call the update function */
	if (view->update_level == 1)
	{
		while (view->update_pending)
		{
			int size;

			/* no longer pending */
			view->update_pending = 0;

			/* resize the viewdata if needed */
			size = view->visible_rows*view->visible_cols;
			if (size > view->viewdata_size)
			{
				view->viewdata_size = size;
				view->viewdata = realloc(view->viewdata, sizeof(view->viewdata[0]) * view->viewdata_size);
			}

			/* update the view */
			if (view->cb.update)
				(*view->cb.update)(view);

			/* update the owner */
			if (view->update_func)
				(*view->update_func)(view);
		}
	}

	/* decrement the level */
	view->update_level--;
}


/*-------------------------------------------------
    debug_view_update_all - force all views to
    refresh
-------------------------------------------------*/

void debug_view_update_all(void)
{
	debug_view *view;

	/* loop over each view and force an update */
	for (view = first_view; view != NULL; view = view->next)
	{
		debug_view_begin_update(view);
		view->update_pending = TRUE;
		debug_view_end_update(view);
	}
}


/*-------------------------------------------------
    debug_view_update_type - force all views of
    a given type to refresh
-------------------------------------------------*/

void debug_view_update_type(int type)
{
	debug_view *view;

	/* loop over each view and force an update */
	for (view = first_view; view != NULL; view = view->next)
		if (view->type == type)
		{
			debug_view_begin_update(view);
			view->update_pending = TRUE;
			debug_view_end_update(view);
		}
}



/*###################################################################################################
**  CONSOLE VIEW
**#################################################################################################*/

/*-------------------------------------------------
    console_update - update the console view
-------------------------------------------------*/

static void console_update(debug_view *view)
{
	debug_view_char *dest = view->viewdata;
	int total_rows, total_cols;
	UINT32 row;

	/* update the console info */
	debug_console_get_size(&total_rows, &total_cols);
	view->total_rows = total_rows;
	view->total_cols = total_cols;

	/* loop over visible rows */
	for (row = 0; row < view->visible_rows; row++)
	{
		UINT32 effrow = view->top_row + row;
		UINT32 col = 0;

		/* if this visible row is valid, add it to the buffer */
		if (effrow < view->total_rows)
		{
			const char *data = debug_console_get_line(effrow);
			UINT32 len = strlen(data);
			UINT32 effcol = view->left_col;

			/* copy data */
			while (col < view->visible_cols && effcol < len)
			{
				dest->byte = data[effcol++];
				dest->attrib = DCA_NORMAL;
				dest++;
				col++;
			}
		}

		/* fill the rest with blanks */
		while (col < view->visible_cols)
		{
			dest->byte = ' ';
			dest->attrib = DCA_NORMAL;
			dest++;
			col++;
		}
	}
}



/*###################################################################################################
**  REGISTERS VIEW
**#################################################################################################*/

/*-------------------------------------------------
    registers_alloc - allocate memory for the
    registers view
-------------------------------------------------*/

static int registers_alloc(debug_view *view)
{
	debug_view_registers *regdata;

	/* allocate memory */
	regdata = malloc(sizeof(*regdata));
	if (!regdata)
		return 0;
	memset(regdata, 0, sizeof(*regdata));

	/* initialize */
	regdata->recompute = TRUE;

	/* stash the extra data pointer */
	view->extra_data = regdata;
	return 1;
}


/*-------------------------------------------------
    registers_free - free memory for the
    registers view
-------------------------------------------------*/

static void registers_free(debug_view *view)
{
	debug_view_registers *regdata = view->extra_data;

	/* free any memory we callocated */
	if (regdata)
		free(regdata);
	view->extra_data = NULL;
}


/*-------------------------------------------------
    add_register - adds a register to the
    registers view
-------------------------------------------------*/

static void add_register(debug_view *view, int regnum, const char *str)
{
	debug_view_registers *regdata = view->extra_data;
	int tagstart, taglen, valstart, vallen;
	const char *colon;

	colon = strchr(str, ':');

	/* if no colon, mark everything as tag */
	if (!colon)
	{
		tagstart = 0;
		taglen = strlen(str);
		valstart = 0;
		vallen = 0;
	}

	/* otherwise, break the string at the colon */
	else
	{
		tagstart = 0;
		taglen = colon - str;
		valstart = (colon + 1) - str;
		vallen = strlen(colon + 1);
	}

	/* now trim spaces */
	while (isspace(str[tagstart]) && taglen > 0)
		tagstart++, taglen--;
	while (isspace(str[tagstart + taglen - 1]) && taglen > 0)
		taglen--;
	while (isspace(str[valstart]) && vallen > 0)
		valstart++, vallen--;
	while (isspace(str[valstart + vallen - 1]) && vallen > 0)
		vallen--;

	/* note the register number and info */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = cpunum_get_reg(regdata->cpunum, regnum);
	regdata->reg[view->total_rows].regnum   = regnum;
	regdata->reg[view->total_rows].tagstart = tagstart;
	regdata->reg[view->total_rows].taglen   = taglen;
	regdata->reg[view->total_rows].valstart = valstart;
	regdata->reg[view->total_rows].vallen   = vallen;
	view->total_rows++;

	/* adjust the divider and total cols, if necessary */
	regdata->divider = MAX(regdata->divider, 1 + taglen + 1);
	view->total_cols = MAX(view->total_cols, 1 + taglen + 2 + vallen + 1);
}


/*-------------------------------------------------
    registers_recompute - recompute all info
    for the registers view
-------------------------------------------------*/

static void registers_recompute(debug_view *view)
{
	debug_view_registers *regdata = view->extra_data;
	const int *list = cpunum_debug_register_list(regdata->cpunum);
	int regnum, maxtaglen, maxvallen;

	/* reset the view parameters */
	view->top_row = 0;
	view->left_col = 0;
	view->total_rows = 0;
	view->total_cols = 0;
	regdata->divider = 0;

	/* add a cycles entry: cycles:99999999 */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = 0;
	regdata->reg[view->total_rows].regnum   = MAX_REGS + 1;
	regdata->reg[view->total_rows].tagstart = 0;
	regdata->reg[view->total_rows].taglen   = 6;
	regdata->reg[view->total_rows].valstart = 7;
	regdata->reg[view->total_rows].vallen   = 8;
	maxtaglen = regdata->reg[view->total_rows].taglen;
	maxvallen = regdata->reg[view->total_rows].vallen;
	view->total_rows++;

	/* add a beam entry: beamx:123 */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = 0;
	regdata->reg[view->total_rows].regnum   = MAX_REGS + 2;
	regdata->reg[view->total_rows].tagstart = 0;
	regdata->reg[view->total_rows].taglen   = 5;
	regdata->reg[view->total_rows].valstart = 6;
	regdata->reg[view->total_rows].vallen   = 3;
	maxtaglen = MAX(maxtaglen, regdata->reg[view->total_rows].taglen);
	maxvallen = MAX(maxvallen, regdata->reg[view->total_rows].vallen);
	view->total_rows++;

	/* add a beam entry: beamy:456 */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = 0;
	regdata->reg[view->total_rows].regnum   = MAX_REGS + 3;
	regdata->reg[view->total_rows].tagstart = 0;
	regdata->reg[view->total_rows].taglen   = 5;
	regdata->reg[view->total_rows].valstart = 6;
	regdata->reg[view->total_rows].vallen   = 3;
	maxtaglen = MAX(maxtaglen, regdata->reg[view->total_rows].taglen);
	maxvallen = MAX(maxvallen, regdata->reg[view->total_rows].vallen);
	view->total_rows++;

	/* add a flags entry: flags:xxxxxxxx */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = 0;
	regdata->reg[view->total_rows].regnum   = MAX_REGS + 4;
	regdata->reg[view->total_rows].tagstart = 0;
	regdata->reg[view->total_rows].taglen   = 5;
	regdata->reg[view->total_rows].valstart = 6;
	regdata->reg[view->total_rows].vallen   = strlen(cpunum_flags(regdata->cpunum));
	maxtaglen = MAX(maxtaglen, regdata->reg[view->total_rows].taglen);
	maxvallen = MAX(maxvallen, regdata->reg[view->total_rows].vallen);
	view->total_rows++;

	/* add a divider entry */
	regdata->reg[view->total_rows].lastval  =
	regdata->reg[view->total_rows].currval  = 0;
	regdata->reg[view->total_rows].regnum   = MAX_REGS;
	view->total_rows++;

	/* set the current divider and total cols */
	regdata->divider = 1 + maxtaglen + 1;
	view->total_cols = 1 + maxtaglen + 2 + maxvallen + 1;

	/* add all registers into it */
	for (regnum = 0; regnum < MAX_REGS; regnum++)
	{
		const char *str = NULL;
		int regid;

		/* identify the register id */
		regid = list ? list[regnum] : regnum;
		if (regid < 0)
			break;

		/* retrieve the string for this register */
		str = cpunum_reg_string(regdata->cpunum, regnum);

		/* did we get a string? */
		if (str && str[0] != '~')
			add_register(view, regnum, str);
	}

	/* no longer need to recompute */
	regdata->recompute = FALSE;
}


/*-------------------------------------------------
    registers_update - update the contents of
    the register view
-------------------------------------------------*/

static void registers_update(debug_view *view)
{
	UINT32 execution_counter = debug_get_execution_counter();
	debug_view_registers *regdata = view->extra_data;
	debug_view_char *dest = view->viewdata;
	UINT32 row, i;

	/* if our assumptions changed, revisit them */
	if (regdata->recompute)
		registers_recompute(view);

	/* loop over visible rows */
	for (row = 0; row < view->visible_rows; row++)
	{
		UINT32 effrow = view->top_row + row;
		UINT32 col = 0;

		/* if this visible row is valid, add it to the buffer */
		if (effrow < view->total_rows)
		{
			debug_view_register *reg = &regdata->reg[effrow];
			UINT32 effcol = view->left_col;
			char temp[256], dummy[100];
			UINT8 attrib = DCA_NORMAL;
			UINT32 len = 0;
			char *data = dummy;

			/* get the effective string */
			dummy[0] = 0;
			if (reg->regnum >= MAX_REGS)
			{
				reg->lastval = reg->currval;
				switch (reg->regnum)
				{
					case MAX_REGS:
						reg->tagstart = reg->valstart = reg->vallen = 0;
						reg->taglen = view->total_cols;
						for (i = 0; i < view->total_cols; i++)
							dummy[i] = '-';
						dummy[i] = 0;
						break;

					case MAX_REGS + 1:
						sprintf(dummy, "cycles:%-8d", activecpu_get_icount());
						reg->currval = activecpu_get_icount();
						break;

					case MAX_REGS + 2:
						sprintf(dummy, "beamx:%3d", cpu_gethorzbeampos());
						break;

					case MAX_REGS + 3:
						sprintf(dummy, "beamy:%3d", cpu_getscanline());
						break;

					case MAX_REGS + 4:
						sprintf(dummy, "flags:%s", activecpu_flags());
						break;
				}
			}
			else
			{
				data = (char *)cpunum_reg_string(regdata->cpunum, reg->regnum);
				if (regdata->last_update != execution_counter)
					reg->lastval = reg->currval;
				reg->currval = cpunum_get_reg(regdata->cpunum, reg->regnum);
			}

			/* see if we changed */
			if (reg->lastval != reg->currval)
				attrib = DCA_CHANGED;

			/* build up a string */
			if (reg->taglen < regdata->divider - 1)
			{
				memset(&temp[len], ' ', regdata->divider - 1 - reg->taglen);
				len += regdata->divider - 1 - reg->taglen;
			}

			memcpy(&temp[len], &data[reg->tagstart], reg->taglen);
			len += reg->taglen;

			temp[len++] = ' ';
			temp[len++] = ' ';

			memcpy(&temp[len], &data[reg->valstart], reg->vallen);
			len += reg->vallen;

			temp[len++] = ' ';
			temp[len] = 0;

			/* copy data */
			while (col < view->visible_cols && effcol < len)
			{
				dest->byte = temp[effcol++];
				dest->attrib = attrib | ((effcol <= regdata->divider) ? DCA_ANCILLARY : DCA_NORMAL);
				dest++;
				col++;
			}
		}

		/* fill the rest with blanks */
		while (col < view->visible_cols)
		{
			dest->byte = ' ';
			dest->attrib = DCA_NORMAL;
			dest++;
			col++;
		}
	}

	/* remember the last update */
	regdata->last_update = execution_counter;
}


/*-------------------------------------------------
    registers_getprop - return the value
    of a given property
-------------------------------------------------*/

static void	registers_getprop(debug_view *view, UINT32 property, debug_property_info *value)
{
	debug_view_registers *regdata = view->extra_data;

	switch (property)
	{
		case DVP_REGS_CPUNUM:
			value->i = regdata->cpunum;
			break;

		default:
			osd_die("Attempt to get invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}


/*-------------------------------------------------
    registers_getprop - set the value
    of a given property
-------------------------------------------------*/

static void	registers_setprop(debug_view *view, UINT32 property, debug_property_info value)
{
	debug_view_registers *regdata = view->extra_data;

	switch (property)
	{
		case DVP_REGS_CPUNUM:
			if (value.i != regdata->cpunum)
			{
				debug_view_begin_update(view);
				regdata->cpunum = value.i;
				regdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		default:
			osd_die("Attempt to set invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}



/*###################################################################################################
**  DISASSEMBLY VIEW
**#################################################################################################*/

/*-------------------------------------------------
    disasm_alloc - allocate disasm for the
    disassembly view
-------------------------------------------------*/

static int disasm_alloc(debug_view *view)
{
	debug_view_disasm *dasmdata;

	/* allocate disasm */
	dasmdata = malloc(sizeof(*dasmdata));
	if (!dasmdata)
		return 0;
	memset(dasmdata, 0, sizeof(*dasmdata));

	/* initialize */
	dasmdata->recompute = TRUE;
	dasmdata->display_raw = TRUE;
	dasmdata->backwards_steps = 3;
	dasmdata->dasm_width = DEFAULT_DASM_WIDTH;

	/* stash the extra data pointer */
	view->total_rows = DEFAULT_DASM_LINES;
	view->extra_data = dasmdata;
	return 1;
}


/*-------------------------------------------------
    disasm_free - free disasm for the
    disassembly view
-------------------------------------------------*/

static void disasm_free(debug_view *view)
{
	debug_view_disasm *dasmdata = view->extra_data;

	/* free any disasm we callocated */
	if (dasmdata)
	{
		if (dasmdata->expression)
			expression_free(dasmdata->expression);
		if (dasmdata->expression_string)
			free(dasmdata->expression_string);
		if (dasmdata->address)
			free(dasmdata->address);
		if (dasmdata->dasm)
			free(dasmdata->dasm);
		free(dasmdata);
	}
	view->extra_data = NULL;
}


/*-------------------------------------------------
    disasm_back_up - back up the specified number
    of instructions from the given PC
-------------------------------------------------*/

static offs_t disasm_back_up(int cpunum, const debug_cpu_info *cpuinfo, offs_t startpc, int numinstrs)
{
	int minlen = BYTE2ADDR(activecpu_min_instruction_bytes(), cpuinfo, ADDRESS_SPACE_PROGRAM);
	int maxlen = BYTE2ADDR(activecpu_max_instruction_bytes(), cpuinfo, ADDRESS_SPACE_PROGRAM);
	int use_new_dasm = (activecpu_get_info_fct(CPUINFO_PTR_DISASSEMBLE_NEW) != NULL);
	UINT32 addrmask = cpuinfo->space[ADDRESS_SPACE_PROGRAM].logaddrmask;
	offs_t curpc, lastgoodpc = startpc, temppc;
	UINT8 opbuf[1024], argbuf[1024];
	char dasmbuffer[100];

	/* compute the increment */
	if (minlen == 0) minlen = 1;
	if (maxlen == 0) maxlen = 1;

	/* start off numinstrs back */
	curpc = startpc - minlen * numinstrs;
	if (curpc > startpc)
		curpc = 0;

	/* if using the new disassembler, prefetch the opcode bytes */
	if (use_new_dasm)
		for (temppc = curpc; temppc < startpc; temppc++)
		{
			opbuf[1000 + temppc - startpc] = debug_read_opcode(temppc, 1, FALSE);
			argbuf[1000 + temppc - startpc] = debug_read_opcode(temppc, 1, TRUE);
		}

	/* loop until we hit it */
	while (1)
	{
		offs_t testpc, nextcurpc, instlen, instcount = 0;

		/* loop until we get past the target instruction */
		for (testpc = curpc; testpc < startpc; testpc += instlen)
		{
			/* convert PC to a byte offset */
			offs_t pcbyte = ADDR2BYTE_MASKED(testpc, cpuinfo, ADDRESS_SPACE_PROGRAM);

			/* get the disassembly, but only if mapped */
			instlen = 1;
			if (!cpuinfo->translate || (*cpuinfo->translate)(ADDRESS_SPACE_PROGRAM, &pcbyte))
				if (memory_get_read_ptr(cpunum, ADDRESS_SPACE_PROGRAM, pcbyte) != NULL)
				{
					memory_set_opbase(pcbyte);
					if (use_new_dasm)
						instlen = activecpu_dasm_new(dasmbuffer, testpc & addrmask, &opbuf[1000 + testpc - startpc], &argbuf[1000 + testpc - startpc], startpc - testpc) & DASMFLAG_LENGTHMASK;
					else
						instlen = activecpu_dasm(dasmbuffer, testpc & addrmask) & DASMFLAG_LENGTHMASK;
				}

			/* count this one */
			instcount++;
		}

		/* if we ended up right on startpc, this is a good candidate */
		if (testpc == startpc && instcount <= numinstrs)
			lastgoodpc = curpc;

		/* we're also done if we go back too far */
		if (startpc - curpc >= numinstrs * maxlen)
			break;

		/* and if we hit 0, we're done */
		if (curpc == 0)
			break;

		/* back up one more and try again */
		nextcurpc = curpc - minlen;
		if (nextcurpc > startpc)
			nextcurpc = 0;

		/* if using the new disassembler, prefetch the opcode bytes */
		if (use_new_dasm)
			for (temppc = nextcurpc; temppc < curpc; temppc++)
			{
				opbuf[1000 + temppc - startpc] = debug_read_opcode(temppc, 1, FALSE);
				argbuf[1000 + temppc - startpc] = debug_read_opcode(temppc, 1, TRUE);
			}

		/* update curpc once we're done fetching */
		curpc = nextcurpc;
	}

	return lastgoodpc;
}


/*-------------------------------------------------
    disasm_generate_bytes - generate the opcode
    byte values
-------------------------------------------------*/

static void disasm_generate_bytes(offs_t pcbyte, int numbytes, const debug_cpu_info *cpuinfo, int minbytes, char *string, int maxchars, int encrypted)
{
	int byte, offset = 0;
	UINT64 val;

	switch (minbytes)
	{
		case 1:
			if (maxchars >= 2)
				offset = sprintf(string, "%02X", (UINT32)debug_read_opcode(pcbyte, 1, FALSE));
			for (byte = 1; byte < numbytes && offset + 3 < maxchars; byte++)
				offset += sprintf(&string[offset], " %02X", (UINT32)debug_read_opcode(pcbyte + byte, 1, encrypted));
			break;

		case 2:
			if (maxchars >= 4)
				offset = sprintf(string, "%04X", (UINT32)debug_read_opcode(pcbyte, 2, FALSE));
			for (byte = 2; byte < numbytes && offset + 5 < maxchars; byte += 2)
				offset += sprintf(&string[offset], " %04X", (UINT32)debug_read_opcode(pcbyte + byte, 2, encrypted));
			break;

		case 4:
			if (maxchars >= 8)
				offset = sprintf(string, "%08X", (UINT32)debug_read_opcode(pcbyte, 4, FALSE));
			for (byte = 4; byte < numbytes && offset + 9 < maxchars; byte += 4)
				offset += sprintf(&string[offset], " %08X", (UINT32)debug_read_opcode(pcbyte + byte, 4, encrypted));
			break;

		case 8:
			val = debug_read_opcode(pcbyte, 8, FALSE);
			if (maxchars >= 16)
				offset = sprintf(string, "%08X%08X", (UINT32)(val >> 32), (UINT32)val);
			for (byte = 8; byte < numbytes && offset + 17 < maxchars; byte += 8)
			{
				val = debug_read_opcode(pcbyte + byte, 8, encrypted);
				offset += sprintf(&string[offset], " %08X%08X", (UINT32)(val >> 32), (UINT32)val);
			}
			break;

		default:
			osd_die("disasm_generate_bytes: unknown size = %d\n", minbytes);
			break;
	}

	/* if we ran out of room, indicate more */
	string[maxchars - 1] = 0;
	if (byte < numbytes && maxchars > 3)
		string[maxchars - 2] = string[maxchars - 3] = string[maxchars - 4] = '.';
}


/*-------------------------------------------------
    disasm_recompute - recompute selected info
    for the disassembly view
-------------------------------------------------*/

static void disasm_recompute(debug_view *view, offs_t pc, int startline, int lines, int original_cpunum)
{
	debug_view_disasm *dasmdata = view->extra_data;
	const debug_cpu_info *cpuinfo = debug_get_cpu_info(dasmdata->cpunum);
	int chunksize, minbytes, maxbytes, maxbytes_clamped;
	int use_new_dasm;
	UINT32 addrmask;
	int line;

	/* switch to the context of the CPU in question */
	addrmask = cpuinfo->space[ADDRESS_SPACE_PROGRAM].logaddrmask;
	use_new_dasm = (activecpu_get_info_fct(CPUINFO_PTR_DISASSEMBLE_NEW) != NULL);

	/* determine how many characters we need for an address and set the divider */
	dasmdata->divider1 = 1 + cpuinfo->space[ADDRESS_SPACE_PROGRAM].logchars + 1;

	/* assume a fixed number of characters for the disassembly */
	dasmdata->divider2 = dasmdata->divider1 + 1 + dasmdata->dasm_width + 1;

	/* determine how many bytes we might need to display */
	minbytes = activecpu_min_instruction_bytes();
	maxbytes = activecpu_max_instruction_bytes();
	if (dasmdata->display_raw)
	{
		chunksize = activecpu_databus_width(ADDRESS_SPACE_PROGRAM) / 8;
		maxbytes_clamped = maxbytes;
		if (maxbytes_clamped > DASM_MAX_BYTES)
			maxbytes_clamped = DASM_MAX_BYTES;
		view->total_cols = dasmdata->divider2 + 1 + 2 * maxbytes_clamped + (maxbytes_clamped / minbytes - 1) + 1;
	}
	else
		view->total_cols = dasmdata->divider2 + 1;

	/* reallocate memory if we don't have enough */
	if (dasmdata->allocated_rows < view->total_rows || dasmdata->allocated_cols < view->total_cols)
	{
		/* update our values */
		dasmdata->allocated_rows = view->total_rows;
		dasmdata->allocated_cols = view->total_cols;

		/* allocate address array */
		if (dasmdata->address)
			free(dasmdata->address);
		dasmdata->address = malloc(sizeof(dasmdata->address[0]) * dasmdata->allocated_rows);

		/* allocate disassembly buffer */
		if (dasmdata->dasm)
			free(dasmdata->dasm);
		dasmdata->dasm = malloc(sizeof(dasmdata->dasm[0]) * dasmdata->allocated_rows * dasmdata->allocated_cols);
	}

	/* iterate over lines */
	for (line = 0; line < lines; line++)
	{
		offs_t pcbyte, tempaddr;
		UINT64 dummyreadop;
		char buffer[100];
		int numbytes;
		int instr = startline + line;
		char *destbuf = &dasmdata->dasm[instr * dasmdata->allocated_cols];

		/* convert PC to a byte offset */
		pcbyte = ADDR2BYTE_MASKED(pc, cpuinfo, ADDRESS_SPACE_PROGRAM);

		/* convert back and set the address of this instruction */
		dasmdata->address[instr] = pcbyte;
		sprintf(&destbuf[0], " %0*X  ", cpuinfo->space[ADDRESS_SPACE_PROGRAM].logchars, BYTE2ADDR(pcbyte, cpuinfo, ADDRESS_SPACE_PROGRAM));

		/* make sure we can translate the address */
		tempaddr = pcbyte;
		if (!cpuinfo->translate || (*cpuinfo->translate)(ADDRESS_SPACE_PROGRAM, &tempaddr))
		{
			/* if we can use new disassembly, do it */
			if (use_new_dasm)
			{
				UINT8 opbuf[64], argbuf[64];

				/* fetch the bytes up to the maximum */
				for (numbytes = 0; numbytes < maxbytes; numbytes++)
				{
					opbuf[numbytes] = debug_read_opcode(pcbyte + numbytes, 1, FALSE);
					argbuf[numbytes] = debug_read_opcode(pcbyte + numbytes, 1, TRUE);
				}

				/* disassemble the result */
				pc += numbytes = activecpu_dasm_new(buffer, pc & addrmask, opbuf, argbuf, maxbytes) & DASMFLAG_LENGTHMASK;
			}

			/* otherwise, we need to use the old, risky way */
			else
			{
				/* get the disassembly, but only if mapped */
				if (memory_get_op_ptr(dasmdata->cpunum, pcbyte, 0) != NULL || (cpuinfo->readop && (*cpuinfo->readop)(pcbyte, 1, &dummyreadop)))
				{
					memory_set_opbase(pcbyte);
					pc += numbytes = activecpu_dasm(buffer, pc & addrmask) & DASMFLAG_LENGTHMASK;
				}
				else
				{
					sprintf(buffer, "<unmapped>");
					numbytes = minbytes;
					pc += BYTE2ADDR(minbytes, cpuinfo, ADDRESS_SPACE_PROGRAM);
				}
			}
		}
		else
			sprintf(buffer, "<unmapped>");
		sprintf(&destbuf[dasmdata->divider1 + 1], "%-*s  ", dasmdata->dasm_width, buffer);

		/* get the bytes */
		if (dasmdata->display_raw)
		{
			numbytes = ADDR2BYTE(numbytes, cpuinfo, ADDRESS_SPACE_PROGRAM);
			disasm_generate_bytes(pcbyte, numbytes, cpuinfo, minbytes, &destbuf[dasmdata->divider2], dasmdata->allocated_cols - dasmdata->divider2, dasmdata->display_raw_encrypted);
		}
	}

	/* update opcode base information */
	dasmdata->last_opcode_base = opcode_base;
	dasmdata->last_opcode_arg_base = opcode_arg_base;

	/* reset the opcode base */
	if (dasmdata->cpunum == original_cpunum)
		memory_set_opbase(activecpu_get_physical_pc_byte());

	/* now longer need to recompute */
	dasmdata->recompute = FALSE;
}


/*-------------------------------------------------
    disasm_update - update the contents of
    the disassembly view
-------------------------------------------------*/

static void disasm_update(debug_view *view)
{
	debug_view_disasm *dasmdata = view->extra_data;
	const debug_cpu_info *cpuinfo = debug_get_cpu_info(dasmdata->cpunum);
	offs_t pc = cpunum_get_reg(dasmdata->cpunum, REG_PC);
	offs_t pcbyte = ADDR2BYTE_MASKED(pc, cpuinfo, ADDRESS_SPACE_PROGRAM);
	debug_view_char *dest = view->viewdata;
	int original_cpunum = cpu_getactivecpu();
	EXPRERR exprerr;
	UINT32 row;

	/* switch to the CPU's context */
	cpuintrf_push_context(dasmdata->cpunum);

	/* if our expression is dirty, fix it */
	if (dasmdata->expression_dirty && dasmdata->expression_string)
	{
		parsed_expression *expr;

		/* parse the new expression */
		exprerr = expression_parse(dasmdata->expression_string, debug_get_cpu_info(dasmdata->cpunum)->symtable, &expr);

		/* if it worked, update the expression */
		if (exprerr == EXPRERR_NONE)
		{
			if (dasmdata->expression)
				expression_free(dasmdata->expression);
			dasmdata->expression = expr;
		}
	}

	/* if we're tracking a value, make sure it is visible */
	if (dasmdata->expression && (dasmdata->live_tracking || dasmdata->expression_dirty))
	{
		UINT64 result;

		/* recompute the value of the expression */
		exprerr = expression_execute(dasmdata->expression, &result);
		if (exprerr == EXPRERR_NONE && result != dasmdata->last_result)
		{
			offs_t resultbyte = ADDR2BYTE_MASKED(result, cpuinfo, ADDRESS_SPACE_PROGRAM);

			/* update the result */
			dasmdata->last_result = result;

			/* see if the new result is an address we already have */
			for (row = 0; row < dasmdata->allocated_rows; row++)
				if (dasmdata->address[row] == resultbyte)
					break;

			/* if we didn't find it, or if it's really close to the bottom, recompute */
			if (row == dasmdata->allocated_rows || row >= view->total_rows - view->visible_rows)
				dasmdata->recompute = TRUE;

			/* otherwise, if it's not visible, adjust the view so it is */
			else if (row < view->top_row || row >= view->top_row + view->visible_rows - 2)
				view->top_row = (row > 3) ? row - 3 : 0;
		}

		/* no longer dirty */
		dasmdata->expression_dirty = FALSE;
	}

	/* if the opcode base has changed, rework things */
	if (opcode_base != dasmdata->last_opcode_base || opcode_arg_base != dasmdata->last_opcode_arg_base)
		dasmdata->recompute = TRUE;

	/* if we need to recompute, do it */
	if (dasmdata->recompute)
	{
		/* determine the addresses of what we will display */
		offs_t backpc = disasm_back_up(dasmdata->cpunum, cpuinfo, (UINT32)dasmdata->last_result, dasmdata->backwards_steps);

		/* put ourselves back in the top left */
		view->top_row = 0;
		view->left_col = 0;

		/* recompute the view */
		disasm_recompute(view, backpc, 0, view->total_rows, original_cpunum);
	}

	/* loop over visible rows */
	for (row = 0; row < view->visible_rows; row++)
	{
		UINT32 effrow = view->top_row + row;
		UINT8 attrib = DCA_NORMAL;
		debug_cpu_breakpoint *bp;
		UINT32 col = 0;

		/* if this visible row is valid, add it to the buffer */
		if (effrow < dasmdata->allocated_rows)
		{
			const char *data = &dasmdata->dasm[effrow * dasmdata->allocated_cols];
			UINT32 effcol = view->left_col;
			UINT32 len = 0;

			/* if we're on the line with the PC, recompute and hilight it */
			if (pcbyte == dasmdata->address[effrow])
			{
				attrib = DCA_CURRENT;
				disasm_recompute(view, pc, effrow, 1, original_cpunum);
			}

			/* if we're on a line with a breakpoint, tag it changed */
			else
			{
				for (bp = cpuinfo->first_bp; bp; bp = bp->next)
					if (dasmdata->address[effrow] == ADDR2BYTE_MASKED(bp->address, cpuinfo, ADDRESS_SPACE_PROGRAM))
						attrib = DCA_CHANGED;
			}

			/* get the effective string */
			len = strlen(data);

			/* copy data */
			while (col < view->visible_cols && effcol < len)
			{
				dest->byte = data[effcol++];
				dest->attrib = (effcol <= dasmdata->divider1 || effcol >= dasmdata->divider2) ? (attrib | DCA_ANCILLARY) : attrib;
				dest++;
				col++;
			}
		}

		/* fill the rest with blanks */
		while (col < view->visible_cols)
		{
			dest->byte = ' ';
			dest->attrib = (effrow < view->total_rows) ? (attrib | DCA_ANCILLARY) : attrib;
			dest++;
			col++;
		}
	}

	/* restore the original CPU context */
	cpuintrf_pop_context();
}


/*-------------------------------------------------
    disasm_getprop - return the value
    of a given property
-------------------------------------------------*/

static void	disasm_getprop(debug_view *view, UINT32 property, debug_property_info *value)
{
	debug_view_disasm *dasmdata = view->extra_data;

	switch (property)
	{
		case DVP_DASM_CPUNUM:
			value->i = dasmdata->cpunum;
			break;

		case DVP_DASM_EXPRESSION:
			value->s = dasmdata->expression_string;
			break;

		case DVP_DASM_TRACK_LIVE:
			value->i = dasmdata->live_tracking;
			break;

		case DVP_DASM_DISPLAY_RAW:
			value->i = dasmdata->display_raw;
			break;

		case DVP_DASM_DISPLAY_ENCRYPTED:
			value->i = dasmdata->display_raw_encrypted;
			break;

		case DVP_DASM_BACKWARD_STEPS:
			value->i = dasmdata->backwards_steps;
			break;

		case DVP_DASM_WIDTH:
			value->i = dasmdata->dasm_width;
			break;

		default:
			osd_die("Attempt to get invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}


/*-------------------------------------------------
    disasm_getprop - set the value
    of a given property
-------------------------------------------------*/

static void	disasm_setprop(debug_view *view, UINT32 property, debug_property_info value)
{
	debug_view_disasm *dasmdata = view->extra_data;

	switch (property)
	{
		case DVP_DASM_CPUNUM:
			if (value.i != dasmdata->cpunum)
			{
				debug_view_begin_update(view);
				dasmdata->cpunum = value.i;

				/* we need to recompute the expression in the context of the new CPU */
				dasmdata->expression_dirty = TRUE;
				dasmdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_DASM_EXPRESSION:
			debug_view_begin_update(view);

			/* free the old expression string and allocate a new one */
			if (dasmdata->expression_string)
				free(dasmdata->expression_string);
			dasmdata->expression_string = malloc(strlen(value.s) + 1);
			if (dasmdata->expression_string)
				strcpy(dasmdata->expression_string, value.s);

			/* update everything as a result */
			dasmdata->expression_dirty = TRUE;
			dasmdata->recompute = TRUE;
			view->update_pending = TRUE;
			debug_view_end_update(view);
			break;

		case DVP_DASM_TRACK_LIVE:
			if (value.i != dasmdata->live_tracking)
			{
				debug_view_begin_update(view);
				dasmdata->live_tracking = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_DASM_DISPLAY_RAW:
			if (value.i != dasmdata->display_raw)
			{
				debug_view_begin_update(view);
				dasmdata->display_raw = value.i;
				dasmdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_DASM_DISPLAY_ENCRYPTED:
			if (value.i != dasmdata->display_raw_encrypted)
			{
				debug_view_begin_update(view);
				dasmdata->display_raw_encrypted = value.i;
				dasmdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_DASM_BACKWARD_STEPS:
			if (value.i != dasmdata->backwards_steps)
			{
				debug_view_begin_update(view);
				dasmdata->backwards_steps = value.i;
				dasmdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_DASM_WIDTH:
			if (value.i != dasmdata->dasm_width)
			{
				debug_view_begin_update(view);
				dasmdata->dasm_width = value.i;
				dasmdata->recompute = TRUE;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		default:
			osd_die("Attempt to set invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}



/*###################################################################################################
**  MEMORY VIEW
**#################################################################################################*/

/*
00000000  00 11 22 33 44 55 66 77-88 99 aa bb cc dd ee ff  0123456789abcdef
00000000   0011  2233  4455  6677- 8899  aabb  ccdd  eeff  0123456789abcdef
00000000    00112233    44556677 -  8899aabb    ccddeeff   0123456789abcdef
*/

/*-------------------------------------------------
    memory_alloc - allocate memory for the
    memory view
-------------------------------------------------*/

static int memory_alloc(debug_view *view)
{
	debug_view_memory *memdata;

	/* allocate memory */
	memdata = malloc(sizeof(*memdata));
	if (!memdata)
		return 0;
	memset(memdata, 0, sizeof(*memdata));

	/* by default we track live */
	memdata->live_tracking = TRUE;
	memdata->ascii_view = TRUE;

	/* stash the extra data pointer */
	view->extra_data = memdata;

	/* we support cursors */
	view->supports_cursor = TRUE;
	return 1;
}


/*-------------------------------------------------
    memory_free - free memory for the
    memory view
-------------------------------------------------*/

static void memory_free(debug_view *view)
{
	debug_view_memory *memdata = view->extra_data;

	/* free any memory we callocated */
	if (memdata)
	{
		if (memdata->expression)
			expression_free(memdata->expression);
		if (memdata->expression_string)
			free(memdata->expression_string);
		free(memdata);
	}
	view->extra_data = NULL;
}


/*-------------------------------------------------
    memory_get_cursor_pos - return the cursor
    position as an address and a shift value
-------------------------------------------------*/

static int memory_get_cursor_pos(debug_view *view, offs_t *address, UINT8 *shift)
{
	debug_view_memory *memdata = view->extra_data;
	int curx = view->cursor_col, cury = view->cursor_row;
	int modval;

	/* if not in the middle section, punt */
	if (curx <= memdata->divider1)
		curx = memdata->divider1 + 1;
	if (curx >= memdata->divider2)
		curx = memdata->divider2 - 1;
	curx -= memdata->divider1;

	/* compute the base address */
	*address = 0x10 * cury + memdata->byte_offset;

	/* the rest depends on the current format */

	/* non-reverse case */
	if (!memdata->reverse_view)
	{
		switch (memdata->bytes_per_chunk)
		{
			default:
			case 1:
				modval = curx % 3;
				if (modval == 0) modval = 1;
				modval -= 1;
				*address += curx / 3;
				*shift = 4 - 4 * modval;
				break;

			case 2:
				modval = curx % 6;
				if (modval <= 1) modval = 2;
				modval -= 2;
				*address += 2 * (curx / 6);
				*shift = 12 - 4 * modval;
				break;

			case 4:
				modval = curx % 12;
				if (modval <= 2) modval = 3;
				if (modval == 11) modval = 10;
				modval -= 3;
				*address += 4 * (curx / 12);
				*shift = 28 - 4 * modval;
				break;
		}
	}
	else
	{
		switch (memdata->bytes_per_chunk)
		{
			default:
			case 1:
				modval = curx % 3;
				if (modval == 0) modval = 1;
				modval -= 1;
				*address += 15 - curx / 3;
				*shift = 4 - 4 * modval;
				break;

			case 2:
				modval = curx % 6;
				if (modval <= 1) modval = 2;
				modval -= 2;
				*address += 14 - 2 * (curx / 6);
				*shift = 12 - 4 * modval;
				break;

			case 4:
				modval = curx % 12;
				if (modval <= 2) modval = 3;
				if (modval == 11) modval = 10;
				modval -= 3;
				*address += 12 - 4 * (curx / 12);
				*shift = 28 - 4 * modval;
				break;
		}
	}
	return 1;
}


/*-------------------------------------------------
    memory_set_cursor_pos - set the cursor
    position as a function of an address and a
    shift value
-------------------------------------------------*/

static void memory_set_cursor_pos(debug_view *view, offs_t address, UINT8 shift)
{
	debug_view_memory *memdata = view->extra_data;
	int curx, cury;

	/* offset the address by the byte offset */
	address -= memdata->byte_offset;

	/* compute the y coordinate */
	cury = address / 0x10;

	/* the rest depends on the current format */

	/* non-reverse case */
	if (!memdata->reverse_view)
	{
		switch (memdata->bytes_per_chunk)
		{
			default:
			case 1:
				curx = memdata->divider1 + 1 + 3 * (address % 0x10) + (1 - (shift / 4));
				break;

			case 2:
				curx = memdata->divider1 + 2 + 6 * ((address % 0x10) / 2) + (3 - (shift / 4));
				break;

			case 4:
				curx = memdata->divider1 + 3 + 12 * ((address % 0x10) / 4) + (7 - (shift / 4));
				break;
		}
	}
	else
	{
		switch (memdata->bytes_per_chunk)
		{
			default:
			case 1:
				curx = memdata->divider1 + 1 + 3 * (15 - address % 0x10) + (1 - (shift / 4));
				break;

			case 2:
				curx = memdata->divider1 + 2 + 6 * (7 - (address % 0x10) / 2) + (3 - (shift / 4));
				break;

			case 4:
				curx = memdata->divider1 + 3 + 12 * (3 - (address % 0x10) / 4) + (7 - (shift / 4));
				break;
		}
	}

	/* set the position, clamping to the window bounds */
	view->cursor_col = (curx < 0) ? 0 : (curx >= view->total_cols) ? (view->total_cols - 1) : curx;
	view->cursor_row = (cury < 0) ? 0 : (cury >= view->total_rows) ? (view->total_rows - 1) : cury;

	/* scroll if out of range */
	if (view->cursor_row < view->top_row)
		view->top_row = view->cursor_row;
	if (view->cursor_row >= view->top_row + view->visible_rows)
		view->top_row = view->cursor_row - view->visible_rows + 1;
}


/*-------------------------------------------------
    memory_handle_char - handle a character typed
    within the current view
-------------------------------------------------*/

static void memory_handle_char(debug_view *view, char chval)
{
	debug_view_memory *memdata = view->extra_data;
	static const char *hexvals = "0123456789abcdef";
	char *hexchar = strchr(hexvals, tolower(chval));
	offs_t address;
	UINT8 shift;

	/* get the position */
	if (!memory_get_cursor_pos(view, &address, &shift))
		return;

	/* up/down work the same regardless */
	if (chval == DCH_UP && view->cursor_row > 0)
		address -= 0x10;
	if (chval == DCH_DOWN && view->cursor_row < view->total_rows - 1)
		address += 0x10;

	/* switch off of the current chunk size */
	cpuintrf_push_context(memdata->cpunum);
	switch (memdata->bytes_per_chunk)
	{
		default:
		case 1:
			if (hexchar)
				debug_write_byte(memdata->spacenum, address, (debug_read_byte(memdata->spacenum, address) & ~(0xf << shift)) | ((hexchar - hexvals) << shift));
			if (hexchar || chval == DCH_RIGHT)
			{
				if (shift == 0) { shift = 4; address++; }
				else shift -= 4;
			}
			else if (chval == DCH_LEFT)
			{
				if (shift == 4) { shift = 0; if (address != 0) address--; }
				else shift += 4;
			}
			break;

		case 2:
			if (hexchar)
				debug_write_word(memdata->spacenum, address, (debug_read_word(memdata->spacenum, address) & ~(0xf << shift)) | ((hexchar - hexvals) << shift));
			if (hexchar || chval == DCH_RIGHT)
			{
				if (shift == 0) { shift = 12; address += 2; }
				else shift -= 4;
			}
			else if (chval == DCH_LEFT)
			{
				if (shift == 12) { shift = 0; if (address != 0) address -= 2; }
				else shift += 4;
			}
			break;

		case 4:
			if (hexchar)
				debug_write_dword(memdata->spacenum, address, (debug_read_dword(memdata->spacenum, address) & ~(0xf << shift)) | ((hexchar - hexvals) << shift));
			if (hexchar || chval == DCH_RIGHT)
			{
				if (shift == 0) { shift = 28; address += 4; }
				else shift -= 4;
			}
			else if (chval == DCH_LEFT)
			{
				if (shift == 28) { shift = 0; if (address != 0) address -= 4; }
				else shift += 4;
			}
			break;
	}
	cpuintrf_pop_context();

	/* set a new position */
	memory_set_cursor_pos(view, address, shift);
}


/*-------------------------------------------------
    memory_update - update the contents of
    the register view
-------------------------------------------------*/

static void memory_update(debug_view *view)
{
	debug_view_memory *memdata = view->extra_data;
	const debug_cpu_info *cpuinfo = debug_get_cpu_info(memdata->cpunum);
	debug_view_char *dest = view->viewdata;
	char addrformat[16];
	EXPRERR exprerr;
	UINT32 row;

	/* switch to the CPU's context */
	cpuintrf_push_context(memdata->cpunum);

	/* determine how many characters we need for an address and set the divider */
	sprintf(addrformat, " %*s%%0%dX ", 8 - cpuinfo->space[memdata->spacenum].logchars, "", cpuinfo->space[memdata->spacenum].logchars);

	/* compute total rows and columns */
	view->total_rows = (cpuinfo->space[memdata->spacenum].logbytemask / 0x10) + 1;
	view->total_cols = memdata->ascii_view ? 77 : 59;

	/* set up the dividers */
	memdata->divider1 = 1 + 8 + 1;
	memdata->divider2 = memdata->divider1 + 1 + 16*3 + 1;
	if (memdata->reverse_view)
	{
		int temp = view->total_cols + 1 - memdata->divider2;
		memdata->divider2 = view->total_cols + 1 - memdata->divider1;
		memdata->divider1 = temp;
	}

	/* clamp the bytes per chunk */
	if (memdata->bytes_per_chunk < (1 << cpuinfo->space[memdata->spacenum].addr2byte_lshift))
		memdata->bytes_per_chunk = (1 << cpuinfo->space[memdata->spacenum].addr2byte_lshift);

	/* if our expression is dirty, fix it */
	if ((memdata->expression_dirty || memdata->cpunum != memdata->cpunum) && memdata->expression_string)
	{
		parsed_expression *expr;

		/* parse the new expression */
		exprerr = expression_parse(memdata->expression_string, debug_get_cpu_info(memdata->cpunum)->symtable, &expr);

		/* if it worked, update the expression */
		if (exprerr == EXPRERR_NONE)
		{
			if (memdata->expression)
				expression_free(memdata->expression);
			memdata->expression = expr;
			memdata->expression_dirty = 1;
		}
	}

	/* if we're tracking a value, make sure it is visible */
	if (memdata->expression && (memdata->live_tracking || memdata->expression_dirty || memdata->cpunum != memdata->cpunum))
	{
		UINT64 result;

		/* recompute the value of the expression */
		exprerr = expression_execute(memdata->expression, &result);

		/* reset the row number */
		if (result != memdata->last_result || memdata->expression_dirty || memdata->cpunum != memdata->cpunum)
		{
			memdata->last_result = result;
			view->top_row = ADDR2BYTE_MASKED(memdata->last_result, cpuinfo, memdata->spacenum) / 0x10;
			memdata->byte_offset = ADDR2BYTE_MASKED(memdata->last_result, cpuinfo, memdata->spacenum) % 0x10;
			view->cursor_row = view->top_row;
		}

		/* no longer dirty */
		memdata->expression_dirty = 0;
	}

	/* update our CPU number */
	memdata->cpunum = memdata->cpunum;

	/* loop over visible rows */
	for (row = 0; row < view->visible_rows; row++)
	{
		UINT32 effrow = view->top_row + row;
		offs_t addrbyte = effrow * 0x10 + memdata->byte_offset;
		UINT8 attrib = DCA_NORMAL;
		UINT32 col = 0;

		/* if this visible row is valid, add it to the buffer */
		if (effrow < view->total_rows)
		{
			UINT32 effcol = view->left_col;
			UINT32 len = 0;
			char data[100];
			int i;

			/* generate the string */
			if (!memdata->reverse_view)
			{
				len = sprintf(&data[len], addrformat, BYTE2ADDR(addrbyte & cpuinfo->space[memdata->spacenum].logbytemask, cpuinfo, memdata->spacenum));
				len += sprintf(&data[len], " ");
				switch (memdata->bytes_per_chunk)
				{
					default:
					case 1:
						for (i = 0; i < 16; i++)
						{
							offs_t curaddr = addrbyte + i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], "%02X ", debug_read_byte(memdata->spacenum, addrbyte + i));
							else
								len += sprintf(&data[len], "** ");
						}
						break;

					case 2:
						for (i = 0; i < 8; i++)
						{
							offs_t curaddr = addrbyte + 2 * i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], " %04X ", debug_read_word(memdata->spacenum, addrbyte + 2 * i));
							else
								len += sprintf(&data[len], " **** ");
						}
						break;

					case 4:
						for (i = 0; i < 4; i++)
						{
							offs_t curaddr = addrbyte + 4 * i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], "  %08X  ", debug_read_dword(memdata->spacenum, addrbyte + 4 * i));
							else
								len += sprintf(&data[len], "  ********  ");
						}
						break;
				}
				len += sprintf(&data[len], " ");
				if (memdata->ascii_view)
				{
					for (i = 0; i < 16; i++)
					{
						char c = debug_read_byte(memdata->spacenum, addrbyte + i);
						len += sprintf(&data[len], "%c", isprint(c) ? c : '.');
					}
					len += sprintf(&data[len], " ");
				}
			}
			else
			{
				len = sprintf(&data[len], " ");
				if (memdata->ascii_view)
				{
					for (i = 0; i < 16; i++)
					{
						char c = debug_read_byte(memdata->spacenum, addrbyte + i);
						len += sprintf(&data[len], "%c", isprint(c) ? c : '.');
					}
					len += sprintf(&data[len], " ");
				}
				len += sprintf(&data[len], " ");
				switch (memdata->bytes_per_chunk)
				{
					default:
					case 1:
						for (i = 15; i >= 0; i--)
						{
							offs_t curaddr = addrbyte + i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], "%02X ", debug_read_byte(memdata->spacenum, addrbyte + i));
							else
								len += sprintf(&data[len], "** ");
						}
						break;

					case 2:
						for (i = 7; i >= 0; i--)
						{
							offs_t curaddr = addrbyte + 2 * i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], " %04X ", debug_read_word(memdata->spacenum, addrbyte + 2 * i));
							else
								len += sprintf(&data[len], " **** ");
						}
						break;

					case 4:
						for (i = 3; i >= 0; i--)
						{
							offs_t curaddr = addrbyte + 4 * i;
							if (!cpuinfo->translate || (*cpuinfo->translate)(memdata->spacenum, &curaddr))
								len += sprintf(&data[len], "  %08X  ", debug_read_dword(memdata->spacenum, addrbyte + 4 * i));
							else
								len += sprintf(&data[len], "  ********  ");
						}
						break;
				}
				len += sprintf(&data[len], addrformat, BYTE2ADDR(addrbyte & cpuinfo->space[memdata->spacenum].logbytemask, cpuinfo, memdata->spacenum));
			}

			/* copy data */
			while (col < view->visible_cols && effcol < len)
			{
				dest->byte = data[effcol++];
				if (effcol <= memdata->divider1 || effcol >= memdata->divider2)
					dest->attrib = attrib | DCA_ANCILLARY;
				else if (view->cursor_visible && effcol - 1 == view->cursor_col && effrow == view->cursor_row && dest->byte != ' ')
					dest->attrib = attrib | DCA_SELECTED;
				else
					dest->attrib = attrib;
				dest++;
				col++;
			}
		}

		/* fill the rest with blanks */
		while (col < view->visible_cols)
		{
			dest->byte = ' ';
			dest->attrib = (effrow < view->total_rows) ? (attrib | DCA_ANCILLARY) : attrib;
			dest++;
			col++;
		}
	}

	/* restore the context */
	cpuintrf_pop_context();
}


/*-------------------------------------------------
    memory_getprop - return the value
    of a given property
-------------------------------------------------*/

static void	memory_getprop(debug_view *view, UINT32 property, debug_property_info *value)
{
	debug_view_memory *memdata = view->extra_data;

	switch (property)
	{
		case DVP_MEM_EXPRESSION:
			value->s = memdata->expression_string;
			break;

		case DVP_MEM_TRACK_LIVE:
			value->i = memdata->live_tracking;
			break;

		case DVP_MEM_CPUNUM:
			value->i = memdata->cpunum;
			break;

		case DVP_MEM_SPACENUM:
			value->i = memdata->spacenum;
			break;

		case DVP_MEM_BYTES_PER_CHUNK:
			value->i = memdata->bytes_per_chunk;
			break;

		case DVP_MEM_REVERSE_VIEW:
			value->i = memdata->reverse_view;
			break;

		case DVP_MEM_ASCII_VIEW:
			value->i = memdata->ascii_view;
			break;

		default:
			osd_die("Attempt to get invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}


/*-------------------------------------------------
    memory_getprop - set the value
    of a given property
-------------------------------------------------*/

static void	memory_setprop(debug_view *view, UINT32 property, debug_property_info value)
{
	debug_view_memory *memdata = view->extra_data;

	switch (property)
	{
		case DVP_MEM_EXPRESSION:
			debug_view_begin_update(view);

			/* free the old expression and allocate a new copy */
			if (memdata->expression_string)
				free(memdata->expression_string);
			memdata->expression_string = malloc(strlen(value.s) + 1);
			if (memdata->expression_string)
				strcpy(memdata->expression_string, value.s);

			memdata->expression_dirty = TRUE;
			view->update_pending = TRUE;
			debug_view_end_update(view);
			break;

		case DVP_MEM_TRACK_LIVE:
			if (value.i != memdata->live_tracking)
			{
				debug_view_begin_update(view);
				memdata->live_tracking = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_MEM_CPUNUM:
			if (value.i != memdata->cpunum)
			{
				debug_view_begin_update(view);
				memdata->cpunum = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_MEM_SPACENUM:
			if (value.i != memdata->spacenum)
			{
				debug_view_begin_update(view);
				memdata->spacenum = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_MEM_BYTES_PER_CHUNK:
			if (value.i != memdata->bytes_per_chunk)
			{
				debug_view_begin_update(view);
				memdata->bytes_per_chunk = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_MEM_REVERSE_VIEW:
			if (value.i != memdata->reverse_view)
			{
				debug_view_begin_update(view);
				memdata->reverse_view = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_MEM_ASCII_VIEW:
			if (value.i != memdata->ascii_view)
			{
				debug_view_begin_update(view);
				memdata->ascii_view = value.i;
				view->update_pending = TRUE;
				debug_view_end_update(view);
			}
			break;

		case DVP_CHARACTER:
			debug_view_begin_update(view);
			memory_handle_char(view, value.i);
			view->update_pending = TRUE;
			debug_view_end_update(view);
			break;

		default:
			osd_die("Attempt to set invalid property %d on debug view type %d\n", property, view->type);
			break;
	}
}


