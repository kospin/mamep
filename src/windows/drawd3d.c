//============================================================
//
//  drawd3d.c - Win32 Direct3D implementation
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// Useful info:
//  Windows XP/2003 shipped with DirectX 8.1
//  Windows 2000 shipped with DirectX 7a
//  Windows 98SE shipped with DirectX 6.1a
//  Windows 98 shipped with DirectX 5
//  Windows NT shipped with DirectX 3.0a
//  Windows 95 shipped with DirectX 2

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <d3d9.h>
#include "d3dintf.h"

// standard C headers
#include <math.h>

// MAME headers
#include "render.h"
#include "options.h"

// OSD headers
#include "winmain.h"
#include "window.h"


// future caps to handle:
//    if (d3d->caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA)



//============================================================
//  DEBUGGING
//============================================================

#define DEBUG_MODE_SCORES	0

extern void mtlog_add(const char *event);



//============================================================
//  CONSTANTS
//============================================================

#define VERTEX_FORMAT		(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define VERTEX_BUFFER_SIZE	(2048*4)

enum
{
	TEXTURE_TYPE_PLAIN,
	TEXTURE_TYPE_DYNAMIC,
	TEXTURE_TYPE_SURFACE
};



//============================================================
//  MACROS
//============================================================

#define FSWAP(var1, var2) do { float temp = var1; var1 = var2; var2 = temp; } while (0)



//============================================================
//  TYPE DEFINITIONS
//============================================================

/* texture_info holds information about a texture */
typedef struct _texture_info texture_info;
struct _texture_info
{
	texture_info *			next;						// next texture in the list
	UINT32					hash;						// hash value for the texture
	UINT32					flags;						// rendering flags
	render_texinfo			texinfo;					// copy of the texture info
	float					ustart, ustop;				// beginning/ending U coordinates
	float					vstart, vstop;				// beginning/ending V coordinates
	int						rawwidth, rawheight;		// raw width/height of the texture
	int						type;						// what type of texture are we?
	int						borderpix;					// do we have a 1 pixel border?
	int						xprescale;					// what is our X prescale factor?
	int						yprescale;					// what is our Y prescale factor?
	d3d_texture *			d3dtex;						// Direct3D texture pointer
	d3d_surface *			d3dsurface;					// Direct3D offscreen plain surface pointer
	d3d_texture *			d3dfinaltex;				// Direct3D final (post-scaled) texture
};


/* poly_info holds information about a single polygon/d3d primitive */
typedef struct _poly_info poly_info;
struct _poly_info
{
	 D3DPRIMITIVETYPE		type;						// type of primitive
	 UINT32					count;						// total number of primitives
	 UINT32					numverts;					// total number of vertices
	 UINT32					flags;						// rendering flags
	 texture_info *			texture;					// pointer to texture info
};


/* d3d_vertex describes a single vertex */
typedef struct _d3d_vertex d3d_vertex;
struct _d3d_vertex
{
	float					x, y, z;					// X,Y,Z coordinates
	float					rhw;						// 1/W coordinate
	D3DCOLOR 				color;						// diffuse color
	float					u0, v0;						// texture stage 0 coordinates
};


/* d3d_info is the information about Direct3D for the current screen */
typedef struct _d3d_info d3d_info;
struct _d3d_info
{
	int						adapter;					// ordinal adapter number
	int						width, height;				// current width, height
	int						refresh;					// current refresh rate

	d3d_device *			device;						// pointer to the Direct3DDevice object
	d3d_present_parameters	presentation;				// set of presentation parameters
	D3DDISPLAYMODE			origmode;					// original display mode for the adapter
	D3DFORMAT				pixformat;					// pixel format we are using

	d3d_vertex_buffer *		vertexbuf;					// pointer to the vertex buffer object
	d3d_vertex *			lockedbuf;					// pointer to the locked vertex buffer
	int						numverts;					// number of accumulated vertices

	poly_info				poly[VERTEX_BUFFER_SIZE / 3];// array to hold polygons as they are created
	int						numpolys;					// number of accumulated polygons

	texture_info *			texlist;					// list of active textures
	int						dynamic_supported;			// are dynamic textures supported?
	int						stretch_supported;			// is StretchRect with point filtering supported?
	D3DFORMAT				screen_format;				// format to use for screen textures

	DWORD					texture_caps;				// textureCaps field
	DWORD					texture_max_aspect;			// texture maximum aspect ratio
	DWORD					texture_max_width;			// texture maximum width
	DWORD					texture_max_height;			// texture maximum height

	texture_info *			last_texture;				// previous texture
	int						last_blendenable;			// previous blendmode
	int						last_blendsrc;				// previous blendmode
	int						last_blenddst;				// previous blendmode
	int						last_filter;				// previous texture filter
};


/* line_aa_step is used for drawing antialiased lines */
typedef struct _line_aa_step line_aa_step;
struct _line_aa_step
{
	float					xoffs, yoffs;				// X/Y deltas
	float					weight;						// weight contribution
};



//============================================================
//  GLOBALS
//============================================================

static d3d *				d3dintf;

static const line_aa_step line_aa_1step[] =
{
	{  0.00f,  0.00f,  1.00f  },
	{ 0 }
};

static const line_aa_step line_aa_4step[] =
{
	{ -0.25f,  0.00f,  0.25f  },
	{  0.25f,  0.00f,  0.25f  },
	{  0.00f, -0.25f,  0.25f  },
	{  0.00f,  0.25f,  0.25f  },
	{ 0 }
};



//============================================================
//  INLINES
//============================================================

INLINE UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags)
{
	return (UINT32)texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}


INLINE void set_texture(d3d_info *d3d, texture_info *texture)
{
	HRESULT result;
	if (texture != d3d->last_texture)
	{
		d3d->last_texture = texture;
		result = (*d3dintf->device.set_texture)(d3d->device, 0, (texture == NULL) ? NULL : texture->d3dfinaltex);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_texture call\n", (int)result);
	}
}


INLINE void set_filter(d3d_info *d3d, int filter)
{
	HRESULT result;
	if (filter != d3d->last_filter)
	{
		d3d->last_filter = filter;
		result = (*d3dintf->device.set_texture_stage_state)(d3d->device, 0, D3DTSS_MINFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_texture_stage_state call\n", (int)result);
		result = (*d3dintf->device.set_texture_stage_state)(d3d->device, 0, D3DTSS_MAGFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_texture_stage_state call\n", (int)result);
	}
}


INLINE void set_blendmode(d3d_info *d3d, int blendmode)
{
	HRESULT result;
	int blendenable;
	int blendsrc;
	int blenddst;

	// choose the parameters
	switch (blendmode)
	{
		default:
		case BLENDMODE_NONE:			blendenable = FALSE;	blendsrc = D3DBLEND_SRCALPHA;	blenddst = D3DBLEND_INVSRCALPHA;	break;
		case BLENDMODE_ALPHA:			blendenable = TRUE;		blendsrc = D3DBLEND_SRCALPHA;	blenddst = D3DBLEND_INVSRCALPHA;	break;
		case BLENDMODE_RGB_MULTIPLY:	blendenable = TRUE;		blendsrc = D3DBLEND_DESTCOLOR;	blenddst = D3DBLEND_ZERO;			break;
		case BLENDMODE_ADD:				blendenable = TRUE;		blendsrc = D3DBLEND_SRCALPHA;	blenddst = D3DBLEND_ONE;			break;
	}

	// adjust the bits that changed
	if (blendenable != d3d->last_blendenable)
	{
		d3d->last_blendenable = blendenable;
		result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ALPHABLENDENABLE, blendenable);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_render_state call\n", (int)result);
	}

	if (blendsrc != d3d->last_blendsrc)
	{
		d3d->last_blendsrc = blendsrc;
		result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_SRCBLEND, blendsrc);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_render_state call\n", (int)result);
	}

	if (blenddst != d3d->last_blenddst)
	{
		d3d->last_blenddst = blenddst;
		result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_DESTBLEND, blenddst);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_render_state call\n", (int)result);
	}
}


INLINE void reset_render_states(d3d_info *d3d)
{
	// this ensures subsequent calls to the above setters will force-update the data
	d3d->last_texture = (texture_info *)~0;
	d3d->last_filter = -1;
	d3d->last_blendenable = -1;
	d3d->last_blendsrc = -1;
	d3d->last_blenddst = -1;
}



//============================================================
//  PROTOTYPES
//============================================================

// core functions
static void drawd3d_exit(void);
static int drawd3d_window_init(win_window_info *window);
static void drawd3d_window_destroy(win_window_info *window);
static const render_primitive_list *drawd3d_window_get_primitives(win_window_info *window);
static int drawd3d_window_draw(win_window_info *window, HDC dc, int update);

// devices
static int device_create(win_window_info *window);
static int device_create_resources(d3d_info *d3d);
static void device_delete(d3d_info *d3d);
static void device_delete_resources(d3d_info *d3d);
static int device_verify_caps(d3d_info *d3d);
static int device_test_cooperative(d3d_info *d3d);

// video modes
static int config_adapter_mode(win_window_info *window);
static int get_adapter_for_monitor(d3d_info *d3d, win_monitor_info *monitor);
static void pick_best_mode(win_window_info *window);
static int update_window_size(win_window_info *window);

// drawing
static void draw_line(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip);
static void draw_quad(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip);

// primitives
static d3d_vertex *primitive_alloc(d3d_info *d3d, int numverts);
static void primitive_flush_pending(d3d_info *d3d);

// textures
static texture_info *texture_create(d3d_info *d3d, const render_texinfo *texsource, UINT32 flags);
static void texture_compute_size(d3d_info *d3d, int texwidth, int texheight, texture_info *texture);
static void texture_set_data(d3d_info *d3d, texture_info *texture, const render_texinfo *texsource, UINT32 flags);
static void texture_prescale(d3d_info *d3d, texture_info *texture);
static texture_info *texture_find(d3d_info *d3d, const render_primitive *prim);
static void texture_update(d3d_info *d3d, const render_primitive *prim);



//============================================================
//  drawd3d_init
//============================================================

int drawd3d_init(win_draw_callbacks *callbacks)
{
	int version = options_get_int("d3dversion", TRUE);
	d3dintf = NULL;

	// try Direct3D 9 if requested
	if (version >= 9)
		d3dintf = drawd3d9_init();

	// if that didn't work, try Direct3D 8
	if (d3dintf == NULL && version >= 8)
		d3dintf = drawd3d8_init();

	// if we failed, note the error
	if (d3dintf == NULL)
	{
		fprintf(stderr, "Unable to initialize Direct3D.\n");
		return 1;
	}

	// fill in the callbacks
	callbacks->exit = drawd3d_exit;
	callbacks->window_init = drawd3d_window_init;
	callbacks->window_get_primitives = drawd3d_window_get_primitives;
	callbacks->window_draw = drawd3d_window_draw;
	callbacks->window_destroy = drawd3d_window_destroy;
	return 0;
}



//============================================================
//  drawd3d_exit
//============================================================

static void drawd3d_exit(void)
{
	if (d3dintf != NULL)
		(*d3dintf->d3d.release)(d3dintf);
}



//============================================================
//  drawd3d_window_init
//============================================================

static int drawd3d_window_init(win_window_info *window)
{
	d3d_info *d3d;

	// allocate memory for our structures
	d3d = malloc_or_die(sizeof(*d3d));
	memset(d3d, 0, sizeof(*d3d));
	window->dxdata = d3d;

	// configure the adapter for the mode we want
	if (config_adapter_mode(window))
		goto error;

	// create the device immediately for the full screen case (defer for window mode)
	if (window->fullscreen && device_create(window))
		goto error;

	return 0;

error:
	drawd3d_window_destroy(window);
	fprintf(stderr, "Unable to initialize Direct3D.\n");
	return 1;
}



//============================================================
//  drawd3d_window_destroy
//============================================================

static void drawd3d_window_destroy(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;

	// skip if nothing
	if (d3d == NULL)
		return;

	// delete the device
	device_delete(d3d);

	// free the memory in the window
	free(d3d);
	window->dxdata = NULL;
}



//============================================================
//  drawd3d_window_get_primitives
//============================================================

static const render_primitive_list *drawd3d_window_get_primitives(win_window_info *window)
{
	RECT client;
	GetClientRect(window->hwnd, &client);
	render_target_set_bounds(window->target, rect_width(&client), rect_height(&client), winvideo_monitor_get_aspect(window->monitor));
	return render_target_get_primitives(window->target);
}



//============================================================
//  drawd3d_window_draw
//============================================================

static int drawd3d_window_draw(win_window_info *window, HDC dc, int update)
{
	render_bounds clipstack[8];
	render_bounds *clip = &clipstack[0];
	d3d_info *d3d = window->dxdata;
	const render_primitive *prim;
	HRESULT result;

	// if we haven't been created, just punt
	if (d3d == NULL)
		return 1;

	// if we have a device, check the cooperative level
	if (d3d->device != NULL)
	{
		int error = device_test_cooperative(d3d);
		if (error)
			return 1;
	}

	// in window mode, we need to track the window size
	if (!window->fullscreen || d3d->device == NULL)
	{
		// if the size changes, skip this update since the render target will be out of date
		if (update_window_size(window))
			return 0;

		// if we have no device, after updating the size, return an error so GDI can try
		if (d3d->device == NULL)
			return 1;
	}

mtlog_add("drawd3d_window_draw: begin");

	// set up the initial clipping rect
	clip->x0 = clip->y0 = 0;
	clip->x1 = (float)d3d->width;
	clip->y1 = (float)d3d->height;

	// first update any textures
	osd_lock_acquire(window->primlist->lock);
	for (prim = window->primlist->head; prim != NULL; prim = prim->next)
		if (prim->texture.base != NULL)
			texture_update(d3d, prim);

	// begin the scene
mtlog_add("drawd3d_window_draw: begin_scene");
	result = (*d3dintf->device.begin_scene)(d3d->device);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device begin_scene call\n", (int)result);

	d3d->lockedbuf = NULL;

	// loop over primitives
mtlog_add("drawd3d_window_draw: primitive loop begin");
	for (prim = window->primlist->head; prim != NULL; prim = prim->next)
		switch (prim->type)
		{
			case RENDER_PRIMITIVE_CLIP_PUSH:
				clip++;
				assert(clip - clipstack < ARRAY_LENGTH(clipstack));

				/* extract the new clip */
				*clip = prim->bounds;

				/* clip against the main bounds */
				if (clip->x0 < 0) clip->x0 = 0;
				if (clip->y0 < 0) clip->y0 = 0;
				if (clip->x1 > (float)d3d->width) clip->x1 = (float)d3d->width;
				if (clip->y1 > (float)d3d->height) clip->y1 = (float)d3d->height;
				break;

			case RENDER_PRIMITIVE_CLIP_POP:
				clip--;
				assert(clip >= clipstack);
				break;

			case RENDER_PRIMITIVE_LINE:
				draw_line(d3d, prim, clip);
				break;

			case RENDER_PRIMITIVE_QUAD:
				draw_quad(d3d, prim, clip);
				break;
		}
mtlog_add("drawd3d_window_draw: primitive loop end");
	osd_lock_release(window->primlist->lock);

	// flush any pending polygons
mtlog_add("drawd3d_window_draw: flush_pending begin");
	primitive_flush_pending(d3d);
mtlog_add("drawd3d_window_draw: flush_pending end");

	// finish the scene
mtlog_add("drawd3d_window_draw: end_scene begin");
	result = (*d3dintf->device.end_scene)(d3d->device);
mtlog_add("drawd3d_window_draw: end_scene end");
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device end_scene call\n", (int)result);

	// present the current buffers
mtlog_add("drawd3d_window_draw: present begin");
	result = (*d3dintf->device.present)(d3d->device, NULL, NULL, NULL, NULL, 0);
mtlog_add("drawd3d_window_draw: present end");
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device present call\n", (int)result);
	return 0;
}



//============================================================
//  device_create
//============================================================

static int device_create(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;
	HRESULT result;
	int verify;

	// if a device exists, free it
	if (d3d->device != NULL)
		device_delete(d3d);

	// verify the caps
	verify = device_verify_caps(d3d);
	if (verify == 2)
	{
		fprintf(stderr, "Error: Device does not meet minimum requirements for Direct3D rendering\n");
		return 1;
	}
	if (verify == 1)
		fprintf(stderr, "Warning: Device may not perform well for Direct3D rendering\n");

	// verify texture formats
	result = (*d3dintf->d3d.check_device_format)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error: A8R8G8B8 format textures not supported\n");
		return 1;
	}

try_again:
	// try for XRGB first
	d3d->screen_format = D3DFMT_X8R8G8B8;
	result = (*d3dintf->d3d.check_device_format)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0, D3DRTYPE_TEXTURE, d3d->screen_format);
	if (result != D3D_OK)
	{
		// if not, try for ARGB
		d3d->screen_format = D3DFMT_A8R8G8B8;
		result = (*d3dintf->d3d.check_device_format)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0, D3DRTYPE_TEXTURE, d3d->screen_format);
		if (result != D3D_OK && d3d->dynamic_supported)
		{
			d3d->dynamic_supported = FALSE;
			goto try_again;
		}
		if (result != D3D_OK)
		{
			fprintf(stderr, "Error: unable to configure a screen texture format\n");
			return 1;
		}
	}

	// initialize the D3D presentation parameters
	memset(&d3d->presentation, 0, sizeof(d3d->presentation));
	d3d->presentation.BackBufferWidth				= d3d->width;
	d3d->presentation.BackBufferHeight				= d3d->height;
	d3d->presentation.BackBufferFormat				= d3d->pixformat;
	d3d->presentation.BackBufferCount				= video_config.triplebuf ? 2 : 1;
	d3d->presentation.MultiSampleType				= D3DMULTISAMPLE_NONE;
	d3d->presentation.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	d3d->presentation.hDeviceWindow					= window->hwnd;
#ifdef MESS
	d3d->presentation.Windowed						= video_config.windowed || win_has_menu(window);
#else /* !MESS */
	d3d->presentation.Windowed						= !window->fullscreen;
#endif /* MESS */
	d3d->presentation.EnableAutoDepthStencil		= FALSE;
	d3d->presentation.AutoDepthStencilFormat		= D3DFMT_D16;
	d3d->presentation.Flags							= 0;
	d3d->presentation.FullScreen_RefreshRateInHz	= d3d->refresh;
	d3d->presentation.PresentationInterval			= ((video_config.triplebuf && window->fullscreen) || video_config.waitvsync || video_config.syncrefresh) ?
														D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	// create the D3D device
	result = (*d3dintf->d3d.create_device)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, win_window_list->hwnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &d3d->presentation, &d3d->device);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Unable to create the Direct3D device (%08X)\n", (UINT32)result);
		return 1;
	}

	// set the max texture size
	render_target_set_max_texture_size(window->target, d3d->texture_max_width, d3d->texture_max_height);

	verbose_printf("Direct3D: Device created at %dx%d\n", d3d->width, d3d->height);
	return device_create_resources(d3d);
}



//============================================================
//  device_create_resources
//============================================================

static int device_create_resources(d3d_info *d3d)
{
	HRESULT result;

	// allocate a vertex buffer to use
	result = (*d3dintf->device.create_vertex_buffer)(d3d->device,
				sizeof(d3d_vertex) * VERTEX_BUFFER_SIZE,
				D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY,
				VERTEX_FORMAT, D3DPOOL_DEFAULT, &d3d->vertexbuf);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error creating vertex buffer (%08X)", (UINT32)result);
		return 1;
	}

	// set the vertex format
	result = (*d3dintf->device.set_vertex_shader)(d3d->device, VERTEX_FORMAT);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error setting vertex shader (%08X)", (UINT32)result);
		return 1;
	}

	// set the fixed render state
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ZENABLE, D3DZB_FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_FILLMODE, D3DFILL_SOLID);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_SHADEMODE, D3DSHADE_FLAT);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ZWRITEENABLE, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ALPHATESTENABLE, TRUE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_LASTPIXEL, TRUE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_CULLMODE, D3DCULL_NONE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ZFUNC, D3DCMP_LESS);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ALPHAREF, 0);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_DITHERENABLE, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_FOGENABLE, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_SPECULARENABLE, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_STENCILENABLE, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_WRAP0, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_CLIPPING, TRUE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_LIGHTING, FALSE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_COLORVERTEX, TRUE);
	result = (*d3dintf->device.set_render_state)(d3d->device, D3DRS_BLENDOP, D3DBLENDOP_ADD);

	result = (*d3dintf->device.set_texture_stage_state)(d3d->device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	result = (*d3dintf->device.set_texture_stage_state)(d3d->device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	// reset the local states to force updates
	reset_render_states(d3d);

	// clear the buffer
	result = (*d3dintf->device.clear)(d3d->device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0,0,0), 0, 0);
	result = (*d3dintf->device.present)(d3d->device, NULL, NULL, NULL, NULL, 0);

	return 0;
}



//============================================================
//  device_delete
//============================================================

static void device_delete(d3d_info *d3d)
{
	// free resources
	device_delete_resources(d3d);

	// free the device itself
	if (d3d->device != NULL)
		(*d3dintf->device.release)(d3d->device);
	d3d->device = NULL;
}



//============================================================
//  device_delete_resources
//============================================================

static void device_delete_resources(d3d_info *d3d)
{
	// free all textures
	while (d3d->texlist != NULL)
	{
		texture_info *tex = d3d->texlist;
		d3d->texlist = tex->next;
		if (tex->d3dfinaltex != NULL)
			(*d3dintf->texture.release)(tex->d3dfinaltex);
		if (tex->d3dtex != NULL && tex->d3dtex != tex->d3dfinaltex)
			(*d3dintf->texture.release)(tex->d3dtex);
		if (tex->d3dsurface != NULL)
			(*d3dintf->surface.release)(tex->d3dsurface);
		free(tex);
	}

	// free the vertex buffer
	if (d3d->vertexbuf != NULL)
		(*d3dintf->vertexbuf.release)(d3d->vertexbuf);
	d3d->vertexbuf = NULL;
}



//============================================================
//  device_verify_caps
//============================================================

static int device_verify_caps(d3d_info *d3d)
{
	int retval = 0;
	HRESULT result;
	DWORD tempcaps;

	// fetch a few core caps
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_TEXTURE_CAPS, &d3d->texture_caps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_MAX_TEXTURE_ASPECT, &d3d->texture_max_aspect);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_MAX_TEXTURE_WIDTH, &d3d->texture_max_width);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_MAX_TEXTURE_HEIGHT, &d3d->texture_max_height);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);

	// verify presentation capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_PRESENTATION_INTERVALS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DPRESENT_INTERVAL_IMMEDIATE))
	{
		verbose_printf("Direct3D: Error - Device does not support immediate presentations\n");
		retval = 2;
	}
	if (!(tempcaps & D3DPRESENT_INTERVAL_ONE))
	{
		verbose_printf("Direct3D: Error - Device does not support per-refresh presentations\n");
		retval = 2;
	}

	// verify device capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_DEV_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DDEVCAPS_CANRENDERAFTERFLIP))
	{
		verbose_printf("Direct3D: Warning - Device does not support queued rendering after a page flip\n");
		retval = 1;
	}
	if (!(tempcaps & D3DDEVCAPS_HWRASTERIZATION))
	{
		verbose_printf("Direct3D: Warning - Device does not support hardware rasterization\n");
		retval = 1;
	}

	// verify source blend capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_SRCBLEND_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DPBLENDCAPS_SRCALPHA))
	{
		verbose_printf("Direct3D: Error - Device does not support source alpha blending with source alpha\n");
		retval = 2;
	}
	if (!(tempcaps & D3DPBLENDCAPS_DESTCOLOR))
	{
		verbose_printf("Direct3D: Error - Device does not support source alpha blending with destination color\n");
		retval = 2;
	}

	// verify destination blend capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_DSTBLEND_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DPBLENDCAPS_ZERO))
	{
		verbose_printf("Direct3D: Error - Device does not support dest alpha blending with zero\n");
		retval = 2;
	}
	if (!(tempcaps & D3DPBLENDCAPS_ONE))
	{
		verbose_printf("Direct3D: Error - Device does not support dest alpha blending with one\n");
		retval = 2;
	}
	if (!(tempcaps & D3DPBLENDCAPS_INVSRCALPHA))
	{
		verbose_printf("Direct3D: Error - Device does not support dest alpha blending with inverted source alpha\n");
		retval = 2;
	}

	// verify texture capabilities
	if (!(d3d->texture_caps & D3DPTEXTURECAPS_ALPHA))
	{
		verbose_printf("Direct3D: Error - Device does not support texture alpha\n");
		retval = 2;
	}

	// verify texture filter capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_TEXTURE_FILTER_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		verbose_printf("Direct3D: Warning - Device does not support point-sample texture filtering for magnification\n");
		retval = 1;
	}
	if (!(tempcaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		verbose_printf("Direct3D: Warning - Device does not support bilinear texture filtering for magnification\n");
		retval = 1;
	}
	if (!(tempcaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		verbose_printf("Direct3D: Warning - Device does not support point-sample texture filtering for minification\n");
		retval = 1;
	}
	if (!(tempcaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		verbose_printf("Direct3D: Warning - Device does not support bilinear texture filtering for minification\n");
		retval = 1;
	}

	// verify texture addressing capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_TEXTURE_ADDRESS_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DPTADDRESSCAPS_CLAMP))
	{
		verbose_printf("Direct3D: Warning - Device does not support texture clamping\n");
		retval = 1;
	}
	if (!(tempcaps & D3DPTADDRESSCAPS_WRAP))
	{
		verbose_printf("Direct3D: Warning - Device does not support texture wrapping\n");
		retval = 1;
	}

	// verify texture operation capabilities
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_TEXTURE_OP_CAPS, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	if (!(tempcaps & D3DTEXOPCAPS_MODULATE))
	{
		verbose_printf("Direct3D: Warning - Device does not support texture modulation\n");
		retval = 1;
	}

	// set a simpler flag to indicate we can use dynamic textures
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_CAPS2, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	d3d->dynamic_supported = ((tempcaps & D3DCAPS2_DYNAMICTEXTURES) != 0);
	if (d3d->dynamic_supported) verbose_printf("Direct3D: Using dynamic textures\n");

	// set a simpler flag to indicate we can use StretchRect
	result = (*d3dintf->d3d.get_caps_dword)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, CAPS_STRETCH_RECT_FILTER, &tempcaps);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during get_caps_dword call\n", (int)result);
	d3d->stretch_supported = ((tempcaps & D3DPTFILTERCAPS_MAGFPOINT) != 0);
	if (d3d->stretch_supported && video_config.prescale > 1) verbose_printf("Direct3D: Using StretchRect for prescaling\n");

	return retval;
}



//============================================================
//  device_test_cooperative
//============================================================

static int device_test_cooperative(d3d_info *d3d)
{
	HRESULT result;

	// check our current status; if we lost the device, punt to GDI
	result = (*d3dintf->device.test_cooperative_level)(d3d->device);
	if (result == D3DERR_DEVICELOST)
		return 1;

	// if we're able to reset ourselves, try it
	if (result == D3DERR_DEVICENOTRESET)
	{
		verbose_printf("Direct3D: resetting device\n");

		// free all existing resources and call reset on the device
		device_delete_resources(d3d);
		result = (*d3dintf->device.reset)(d3d->device, &d3d->presentation);

		// if it didn't work, punt to GDI
		if (result != D3D_OK)
			return 1;

		// try to create the resources again; if that didn't work, delete the whole thing
		if (device_create_resources(d3d))
		{
			verbose_printf("Direct3D: failed to recreate resources for device; failing permanently\n");
			device_delete(d3d);
			return 1;
		}
	}
	return 0;
}



//============================================================
//  config_adapter_mode
//============================================================

static int config_adapter_mode(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;
	HRESULT result;

	// choose the monitor number
	d3d->adapter = get_adapter_for_monitor(d3d, window->monitor);

	// get the current display mode
	result = (*d3dintf->d3d.get_adapter_display_mode)(d3dintf, d3d->adapter, &d3d->origmode);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Error getting mode for adapter #%d\n", d3d->adapter);
		return 1;
	}

	// choose a resolution: window mode case
	if (!window->fullscreen)
	{
		RECT client;

		// bounds are from the window client rect
		GetClientRect(window->hwnd, &client);
		d3d->width = client.right - client.left;
		d3d->height = client.bottom - client.top;

		// pix format is from the current mode
		d3d->pixformat = d3d->origmode.Format;
		d3d->refresh = 0;

		// make sure it's a pixel format we can get behind
		if (d3d->pixformat != D3DFMT_X1R5G5B5 && d3d->pixformat != D3DFMT_R5G6B5 && d3d->pixformat != D3DFMT_X8R8G8B8)
		{
			fprintf(stderr, "Device %s currently in an unsupported mode\n", window->monitor->info.szDevice);
			return 1;
		}
	}

	// choose a resolution: full screen mode case
	else
	{
		// default to the current mode exactly
		d3d->width = d3d->origmode.Width;
		d3d->height = d3d->origmode.Height;
		d3d->pixformat = d3d->origmode.Format;
		d3d->refresh = d3d->origmode.RefreshRate;

		// if we're allowed to switch resolutions, override with something better
		if (video_config.switchres)
			pick_best_mode(window);
	}

	// see if we can handle the device type
	result = (*d3dintf->d3d.check_device_type)(d3dintf, d3d->adapter, D3DDEVTYPE_HAL, d3d->pixformat, d3d->pixformat, !window->fullscreen);
	if (result != D3D_OK)
	{
		fprintf(stderr, "Proposed video mode not supported on device %s\n", window->monitor->info.szDevice);
		return 1;
	}
	return 0;
}



//============================================================
//  get_adapter_for_monitor
//============================================================

static int get_adapter_for_monitor(d3d_info *d3d, win_monitor_info *monitor)
{
	int maxadapter = (*d3dintf->d3d.get_adapter_count)(d3dintf);
	int adapternum;

	// iterate over adapters until we error or find a match
	for (adapternum = 0; adapternum < maxadapter; adapternum++)
	{
		HMONITOR curmonitor;

		// get the monitor for this adapter
		curmonitor = (*d3dintf->d3d.get_adapter_monitor)(d3dintf, adapternum);

		// if we match the proposed monitor, this is it
		if (curmonitor == monitor->handle)
			return adapternum;
	}

	// default to the default
	return D3DADAPTER_DEFAULT;
}



//============================================================
//  pick_best_mode
//============================================================

static void pick_best_mode(win_window_info *window)
{
	INT32 target_width, target_height;
	d3d_info *d3d = window->dxdata;
	INT32 minwidth, minheight;
	float best_score = 0.0;
	int maxmodes;
	int modenum;

	// determine the minimum width/height for the selected target
	// note: technically we should not be calling this from an alternate window
	// thread; however, it is only done during init time, and the init code on
	// the main thread is waiting for us to finish, so it is safe to do so here
	render_target_get_minimum_size(window->target, &minwidth, &minheight);

	// use those as the target for now
	target_width = minwidth;
	target_height = minheight;

	// determine the maximum number of modes
	maxmodes = (*d3dintf->d3d.get_adapter_mode_count)(d3dintf, d3d->adapter, D3DFMT_X8R8G8B8);

	// enumerate all the video modes and find the best match
	for (modenum = 0; modenum < maxmodes; modenum++)
	{
		float size_score, refresh_score, final_score;
		D3DDISPLAYMODE mode;
		HRESULT result;

		// check this mode
		result = (*d3dintf->d3d.enum_adapter_modes)(d3dintf, d3d->adapter, D3DFMT_X8R8G8B8, modenum, &mode);
		if (result != D3D_OK)
			break;

		// skip non-32 bit modes
		if (mode.Format != D3DFMT_X8R8G8B8)
			continue;

		// compute initial score based on difference between target and current
		size_score = 1.0f / (1.0f + fabs(mode.Width - target_width) + fabs(mode.Height - target_height));

		// if we're looking for a particular mode, that's a winner
		if (mode.Width == window->maxwidth && mode.Height == window->maxheight)
			size_score = 1.0f;

		// if the mode is too small, give a big penalty
		if (mode.Width < minwidth || mode.Height < minheight)
			size_score *= 0.01f;

		// if mode is smaller than we'd like, it only scores up to 0.1
		if (mode.Width < target_width || mode.Height < target_height)
			size_score *= 0.1f;

		// compute refresh score
		refresh_score = 1.0f / (1.0f + fabs((double)mode.RefreshRate - Machine->refresh_rate[0]));

		// if we're looking for a particular refresh, make sure it matches
		if (mode.RefreshRate == window->refresh)
			refresh_score = 1.0f;

		// if refresh is smaller than we'd like, it only scores up to 0.1
		if ((double)mode.RefreshRate < Machine->refresh_rate[0])
			refresh_score *= 0.1;

		// weight size highest, followed by depth and refresh
		final_score = (size_score * 100.0 + refresh_score) / 101.0;

#if DEBUG_MODE_SCORES
		printf("%4dx%4d@%3d -> %f\n", mode.Width, mode.Height, mode.RefreshRate, final_score);
#endif

		// best so far?
		if (final_score > best_score)
		{
			best_score = final_score;
			d3d->width = mode.Width;
			d3d->height = mode.Height;
			d3d->pixformat = mode.Format;
			d3d->refresh = mode.RefreshRate;
		}
	}
}



//============================================================
//  update_window_size
//============================================================

static int update_window_size(win_window_info *window)
{
	d3d_info *d3d = window->dxdata;
	RECT client;

	// get the current window bounds
	GetClientRect(window->hwnd, &client);

	// if we have a device and matching width/height, nothing to do
	if (d3d->device != NULL && rect_width(&client) == d3d->width && rect_height(&client) == d3d->height)
	{
		// clear out any pending resizing if the area didn't change
		if (window->resize_state == RESIZE_STATE_PENDING)
			window->resize_state = RESIZE_STATE_NORMAL;
		return FALSE;
	}

	// if we're in the middle of resizing, leave it alone as well
	if (window->resize_state == RESIZE_STATE_RESIZING)
		return FALSE;

	// set the new bounds and create the device again
	d3d->width = rect_width(&client);
	d3d->height = rect_height(&client);
	if (device_create(window))
		return FALSE;

	// reset the resize state to normal, and indicate we made a change
	window->resize_state = RESIZE_STATE_NORMAL;
	return TRUE;
}



//============================================================
//  draw_line
//============================================================

static void draw_line(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip)
{
	const line_aa_step *step = line_aa_4step;
	float unitx, unity, effwidth;
	render_bounds bounds;
	d3d_vertex *vertex;
	poly_info *poly;
	DWORD color;
	int i;

	/*
        High-level logic -- due to math optimizations, this info is lost below.

        Imagine a thick line of width (w), drawn from (p0) to (p1), with a unit
        vector (u) indicating the direction from (p0) to (p1).

          B                                                          C
            +-----------------------  ...   -----------------------+
            |                                               ^      |
            |                                               |(w)   |
            |                                               v      |
            |<---->* (p0)        ------------>         (p1) *      |
            |  (w)                    (u)                          |
            |                                                      |
            |                                                      |
            +-----------------------  ...   -----------------------+
          A                                                          D

        To convert this into a quad, we need to compute the four points A, B, C
        and D.

        Starting with point A. We first multiply the unit vector by (w) and then
        rotate the result 135 degrees. This points us in the right direction, but
        needs to be scaled by a factor of sqrt(2) to reach A. Thus, we have:

            A.x = p0.x + w * u.x * cos(135) * sqrt(2) - w * u.y * sin(135) * sqrt(2)
            A.y = p0.y + w * u.y * sin(135) * sqrt(2) + w * u.y * cos(135) * sqrt(2)

        Conveniently, sin(135) = 1/sqrt(2), and cos(135) = -1/sqrt(2), so this
        simplifies to:

            A.x = p0.x - w * u.x - w * u.y
            A.y = p0.y + w * u.y - w * u.y

        Working clockwise around the polygon, the same fallout happens all around as
        we rotate the unit vector by -135 (B), -45 (C), and 45 (D) degrees:

            B.x = p0.x - w * u.x + w * u.y
            B.y = p0.y - w * u.x - w * u.y

            C.x = p1.x + w * u.x + w * u.y
            C.y = p1.y - w * u.x + w * u.y

            D.x = p1.x + w * u.x - w * u.y
            D.y = p1.y + w * u.x + w * u.y
    */

	// first we need to compute the clipped line
	bounds = prim->bounds;
	if (render_clip_line(&bounds, clip))
		return;

	// compute a vector from point 0 to point 1
	unitx = bounds.x1 - bounds.x0;
	unity = bounds.y1 - bounds.y0;

	// points just use a +1/+1 unit vector; this gives a nice diamond pattern
	if (unitx == 0 && unity == 0)
	{
		unitx = 0.70710678f;
		unity = 0.70710678f;
	}

	// lines need to be divided by their length
	else
	{
		float invlength = 1.0f / sqrt(unitx * unitx + unity * unity);
		unitx *= invlength;
		unity *= invlength;
	}

	// compute the effective width based on the direction of the line
	effwidth = prim->width;
	if (effwidth < 0.5f)
		effwidth = 0.5f;

	// prescale unitx and unity by the length
	unitx *= effwidth;
	unity *= effwidth;

	// iterate over AA steps
	for (step = PRIMFLAG_GET_ANTIALIAS(prim->flags) ? line_aa_4step : line_aa_1step; step->weight != 0; step++)
	{
		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;

		// rotate the unit vector by 135 degrees and add to point 0
		vertex[0].x = bounds.x0 - unitx - unity + step->xoffs;
		vertex[0].y = bounds.y0 + unitx - unity + step->yoffs;

		// rotate the unit vector by -135 degrees and add to point 0
		vertex[1].x = bounds.x0 - unitx + unity + step->xoffs;
		vertex[1].y = bounds.y0 - unitx - unity + step->yoffs;

		// rotate the unit vector by 45 degrees and add to point 1
		vertex[2].x = bounds.x1 + unitx - unity + step->xoffs;
		vertex[2].y = bounds.y1 + unitx + unity + step->yoffs;

		// rotate the unit vector by -45 degrees and add to point 1
		vertex[3].x = bounds.x1 + unitx + unity + step->xoffs;
		vertex[3].y = bounds.y1 - unitx + unity + step->yoffs;

		// set the color, Z parameters to standard values
		color = D3DCOLOR_ARGB((DWORD)(prim->color.a * 255.0f * step->weight), (DWORD)(prim->color.r * 255.0f), (DWORD)(prim->color.g * 255.0f), (DWORD)(prim->color.b * 255.0f));
		for (i = 0; i < 4; i++)
		{
			vertex[i].z = 0.0f;
			vertex[i].rhw = 1.0f;
			vertex[i].color = color;
		}

		// now add a polygon entry
		poly = &d3d->poly[d3d->numpolys++];
		poly->type = D3DPT_TRIANGLESTRIP;
		poly->count = 2;
		poly->numverts = 4;
		poly->flags = prim->flags;
		poly->texture = NULL;
	}
}



//============================================================
//  draw_quad
//============================================================

static void draw_quad(d3d_info *d3d, const render_primitive *prim, const render_bounds *clip)
{
	texture_info *texture = texture_find(d3d, prim);
	d3d_vertex *vertex;
	render_bounds bounds;
	poly_info *poly;
	DWORD color;
	int i;

	// make a copy of the bounds
	bounds = prim->bounds;

	// non-textured case
	if (texture == NULL)
	{
		// apply clipping
		if (render_clip_quad(&bounds, clip, NULL, NULL))
			return;

		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;
	}

	// textured case
	else
	{
		float u[4], v[4];

		// set the default coordinates
		u[0] = texture->ustart;		v[0] = texture->vstart;
		u[1] = texture->ustop; 		v[1] = texture->vstart;
		u[2] = texture->ustart;		v[2] = texture->vstop;
		u[3] = texture->ustop; 		v[3] = texture->vstop;

		// apply orientation to the U/V coordinates
		if (prim->flags & ORIENTATION_SWAP_XY) { FSWAP(u[1], u[2]); FSWAP(v[1], v[2]); }
		if (prim->flags & ORIENTATION_FLIP_X) { FSWAP(u[0], u[1]); FSWAP(v[0], v[1]); FSWAP(u[2], u[3]); FSWAP(v[2], v[3]); }
		if (prim->flags & ORIENTATION_FLIP_Y) { FSWAP(u[0], u[2]); FSWAP(v[0], v[2]); FSWAP(u[1], u[3]); FSWAP(v[1], v[3]); }

		// apply clipping
		if (render_clip_quad(&bounds, clip, u, v))
			return;

		// get a pointer to the vertex buffer
		vertex = primitive_alloc(d3d, 4);
		if (vertex == NULL)
			return;

		// set the final coordinates
		vertex[0].u0 = u[0];
		vertex[0].v0 = v[0];
		vertex[1].u0 = u[1];
		vertex[1].v0 = v[1];
		vertex[2].u0 = u[2];
		vertex[2].v0 = v[2];
		vertex[3].u0 = u[3];
		vertex[3].v0 = v[3];
	}

	// fill in the vertexes clockwise
	vertex[0].x = bounds.x0 - 0.5f;
	vertex[0].y = bounds.y0 - 0.5f;
	vertex[1].x = bounds.x1 - 0.5f;
	vertex[1].y = bounds.y0 - 0.5f;
	vertex[2].x = bounds.x0 - 0.5f;
	vertex[2].y = bounds.y1 - 0.5f;
	vertex[3].x = bounds.x1 - 0.5f;
	vertex[3].y = bounds.y1 - 0.5f;

	// set the color, Z parameters to standard values
	color = D3DCOLOR_ARGB((DWORD)(prim->color.a * 255.0f), (DWORD)(prim->color.r * 255.0f), (DWORD)(prim->color.g * 255.0f), (DWORD)(prim->color.b * 255.0f));
	for (i = 0; i < 4; i++)
	{
		vertex[i].z = 0.0f;
		vertex[i].rhw = 1.0f;
		vertex[i].color = color;
	}

	// now add a polygon entry
	poly = &d3d->poly[d3d->numpolys++];
	poly->type = D3DPT_TRIANGLESTRIP;
	poly->count = 2;
	poly->numverts = 4;
	poly->flags = prim->flags;
	poly->texture = texture;
}



//============================================================
//  primitive_alloc
//============================================================

static d3d_vertex *primitive_alloc(d3d_info *d3d, int numverts)
{
	HRESULT result;

	// if we're going to overflow, flush
	if (d3d->lockedbuf != NULL && d3d->numverts + numverts >= VERTEX_BUFFER_SIZE)
		primitive_flush_pending(d3d);

	// if we don't have a lock, grab it now
	if (d3d->lockedbuf == NULL)
	{
		result = (*d3dintf->vertexbuf.lock)(d3d->vertexbuf, 0, 0, (VOID **)&d3d->lockedbuf, D3DLOCK_DISCARD);
		if (result != D3D_OK)
			return NULL;
	}

	// if we already have the lock and enough room, just return a pointer
	if (d3d->lockedbuf != NULL && d3d->numverts + numverts < VERTEX_BUFFER_SIZE)
	{
		int oldverts = d3d->numverts;
		d3d->numverts += numverts;
		return &d3d->lockedbuf[oldverts];
	}
	return NULL;
}



//============================================================
//  primitive_flush_pending
//============================================================

static void primitive_flush_pending(d3d_info *d3d)
{
	HRESULT result;
	int polynum;
	int vertnum;

	// ignore if we're not locked
	if (d3d->lockedbuf == NULL)
		return;

	// unlock the buffer
	result = (*d3dintf->vertexbuf.unlock)(d3d->vertexbuf);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during vertex buffer unlock call\n", (int)result);
	d3d->lockedbuf = NULL;

	// set the stream
	result = (*d3dintf->device.set_stream_source)(d3d->device, 0, d3d->vertexbuf, sizeof(d3d_vertex));
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_stream_source call\n", (int)result);

	// now do the polys
	for (polynum = vertnum = 0; polynum < d3d->numpolys; polynum++)
	{
		poly_info *poly = &d3d->poly[polynum];
		int newfilter;

		// set the texture if different
		set_texture(d3d, poly->texture);

		// set filtering if different
		if (poly->texture != NULL)
		{
			newfilter = FALSE;
			if (PRIMFLAG_GET_SCREENTEX(poly->flags))
				newfilter = video_config.filter;
			set_filter(d3d, newfilter);
		}

		// set the blendmode if different
		set_blendmode(d3d, PRIMFLAG_GET_BLENDMODE(poly->flags));

		// add the primitives
		assert(vertnum + poly->numverts <= d3d->numverts);
		result = (*d3dintf->device.draw_primitive)(d3d->device, poly->type, vertnum, poly->count);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device draw_primitive call\n", (int)result);
		vertnum += poly->numverts;
	}

	// reset the vertex count
	d3d->numverts = 0;
	d3d->numpolys = 0;
}



//============================================================
//  texture_create
//============================================================

static texture_info *texture_create(d3d_info *d3d, const render_texinfo *texsource, UINT32 flags)
{
	texture_info *texture;
	HRESULT result;

	// allocate a new texture
	texture = malloc_or_die(sizeof(*texture));
	memset(texture, 0, sizeof(*texture));

	// fill in the core data
	texture->hash = texture_compute_hash(texsource, flags);
	texture->flags = flags;
	texture->texinfo = *texsource;
	texture->xprescale = video_config.prescale;
	texture->yprescale = video_config.prescale;

	// compute the size
	texture_compute_size(d3d, texsource->width, texsource->height, texture);

	// non-screen textures are easy
	if (!PRIMFLAG_GET_SCREENTEX(flags))
	{
		result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture->d3dtex);
		if (result != D3D_OK)
			goto error;
		texture->d3dfinaltex = texture->d3dtex;
		texture->type = TEXTURE_TYPE_PLAIN;
	}

	// screen textures are allocated differently
	else
	{
		DWORD usage = d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0;
		DWORD pool = d3d->dynamic_supported ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
		int maxdim = MAX(d3d->presentation.BackBufferWidth, d3d->presentation.BackBufferHeight);

		// don't prescale above screen size
		while (texture->xprescale > 1 && texture->rawwidth * texture->xprescale >= 2 * maxdim)
			texture->xprescale--;
		while (texture->xprescale > 1 && texture->rawwidth * texture->xprescale > d3d->texture_max_width)
			texture->xprescale--;
		while (texture->yprescale > 1 && texture->rawheight * texture->yprescale >= 2 * maxdim)
			texture->yprescale--;
		while (texture->yprescale > 1 && texture->rawheight * texture->yprescale > d3d->texture_max_height)
			texture->yprescale--;
		if (texture->xprescale != video_config.prescale || texture->yprescale != video_config.prescale)
			verbose_printf("Direct3D: adjusting prescale from %dx%d to %dx%d\n", video_config.prescale, video_config.prescale, texture->xprescale, texture->yprescale);

		// screen textures with no prescaling are pretty easy
		if (texture->xprescale == 1 && texture->yprescale == 1)
		{
			result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, usage, d3d->screen_format, pool, &texture->d3dtex);
			if (result != D3D_OK)
				goto error;
			texture->d3dfinaltex = texture->d3dtex;
			texture->type = d3d->dynamic_supported ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;
		}

		// screen textures with prescaling require two allocations
		else
		{
			int scwidth, scheight;

			// use an offscreen plain surface for stretching if supported
			if (d3d->stretch_supported)
			{
				result = (*d3dintf->device.create_offscreen_plain_surface)(d3d->device, texture->rawwidth, texture->rawheight, d3d->screen_format, D3DPOOL_DEFAULT, &texture->d3dsurface);
				if (result != D3D_OK)
					goto error;
				texture->type = TEXTURE_TYPE_SURFACE;
			}

			// otherwise, we allocate a dynamic texture for the source
			else
			{
				result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, usage, d3d->screen_format, pool, &texture->d3dtex);
				if (result != D3D_OK)
					goto error;
				texture->type = d3d->dynamic_supported ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;
			}

			// for the target surface, we allocate a render target texture
			scwidth = texture->rawwidth * texture->xprescale;
			scheight = texture->rawheight * texture->yprescale;
			result = (*d3dintf->device.create_texture)(d3d->device, scwidth, scheight, 1, D3DUSAGE_RENDERTARGET, d3d->screen_format, D3DPOOL_DEFAULT, &texture->d3dfinaltex);
			if (result != D3D_OK)
				goto error;
		}
	}

	// copy the data to the texture
	texture_set_data(d3d, texture, texsource, flags);

	// add us to the texture list
	texture->next = d3d->texlist;
	d3d->texlist = texture;
	return texture;

error:
	if (texture->d3dsurface != NULL)
		(*d3dintf->surface.release)(texture->d3dsurface);
	if (texture->d3dtex != NULL)
		(*d3dintf->texture.release)(texture->d3dtex);
	free(texture);
	return NULL;
}



//============================================================
//  texture_compute_size
//============================================================

static void texture_compute_size(d3d_info *d3d, int texwidth, int texheight, texture_info *texture)
{
	int finalheight = texheight;
	int finalwidth = texwidth;

	// if we're not wrapping, add a 1 pixel border on all sides
	texture->borderpix = !(texture->flags & PRIMFLAG_TEXWRAP_MASK);
	if (texture->borderpix)
	{
		finalwidth += 2;
		finalheight += 2;
	}

	// round width/height up to nearest power of 2 if we need to
	if (!(d3d->texture_caps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
	{
		// first the width
		if (finalwidth & (finalwidth - 1))
		{
			finalwidth |= finalwidth >> 1;
			finalwidth |= finalwidth >> 2;
			finalwidth |= finalwidth >> 4;
			finalwidth |= finalwidth >> 8;
			finalwidth++;
		}

		// then the height
		if (finalheight & (finalheight - 1))
		{
			finalheight |= finalheight >> 1;
			finalheight |= finalheight >> 2;
			finalheight |= finalheight >> 4;
			finalheight |= finalheight >> 8;
			finalheight++;
		}
	}

	// round up to square if we need to
	if (d3d->texture_caps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		if (finalwidth < finalheight)
			finalwidth = finalheight;
		else
			finalheight = finalwidth;
	}

	// adjust the aspect ratio if we need to
	while (finalwidth < finalheight && finalheight / finalwidth > d3d->texture_max_aspect)
		finalwidth *= 2;
	while (finalheight < finalwidth && finalwidth / finalheight > d3d->texture_max_aspect)
		finalheight *= 2;

	// if we added pixels for the border, and that just barely pushed us over, take it back
	if (texture->borderpix &&
		((finalwidth > d3d->texture_max_width && finalwidth - 2 <= d3d->texture_max_width) ||
		 (finalheight > d3d->texture_max_height && finalheight - 2 <= d3d->texture_max_height)))
	{
		texture->borderpix = FALSE;
		finalwidth -= 2;
		finalheight -= 2;
	}

	// if we're above the max width/height, do what?
	if (finalwidth > d3d->texture_max_width || finalheight > d3d->texture_max_height)
	{
		static int printed = FALSE;
		if (!printed) fprintf(stderr, "Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth, finalheight, (int)d3d->texture_max_width, (int)d3d->texture_max_height);
		printed = TRUE;
	}

	// compute the U/V scale factors
	if (texture->borderpix)
	{
		texture->ustart = 1.0f / (float)finalwidth;
		texture->ustop = (float)(texwidth + 1) / (float)finalwidth;
		texture->vstart = 1.0f / (float)finalheight;
		texture->vstop = (float)(texheight + 1) / (float)finalheight;
	}
	else
	{
		texture->ustart = 0.0f;
		texture->ustop = (float)texwidth / (float)finalwidth;
		texture->vstart = 0.0f;
		texture->vstop = (float)texheight / (float)finalheight;
	}

	// set the final values
	texture->rawwidth = finalwidth;
	texture->rawheight = finalheight;
}



//============================================================
//  texture_set_data
//============================================================

static void texture_set_data(d3d_info *d3d, texture_info *texture, const render_texinfo *texsource, UINT32 flags)
{
	D3DLOCKED_RECT rect;
	HRESULT result;
	UINT32 *dst32;
	int x, y;

	// lock the texture
	switch (texture->type)
	{
		default:
		case TEXTURE_TYPE_PLAIN:	result = (*d3dintf->texture.lock_rect)(texture->d3dtex, 0, &rect, NULL, 0);					break;
		case TEXTURE_TYPE_DYNAMIC:	result = (*d3dintf->texture.lock_rect)(texture->d3dtex, 0, &rect, NULL, D3DLOCK_DISCARD);	break;
		case TEXTURE_TYPE_SURFACE:	result = (*d3dintf->surface.lock_rect)(texture->d3dsurface, &rect, NULL, D3DLOCK_DISCARD);	break;
	}
	if (result != D3D_OK)
		return;

	// always fill non-wrapping textures with an extra pixel on the top
	if (texture->borderpix)
	{
		dst32 = (UINT32 *)((BYTE *)rect.pBits + 0 * rect.Pitch);
		for (x = 0; x < texsource->width + 2; x++)
			*dst32++ = 0;
	}

	// loop over Y
	for (y = 0; y < texsource->height; y++)
	{
		UINT32 *src32;
		UINT16 *src16;

		// always fill non-wrapping textures with an extra pixel on the left
		if (texture->borderpix)
		{
			dst32 = (UINT32 *)((BYTE *)rect.pBits + (y + 1) * rect.Pitch);
			*dst32++ = 0;
		}
		else
			dst32 = (UINT32 *)((BYTE *)rect.pBits + y * rect.Pitch);

		// switch off of the format and
		switch (PRIMFLAG_GET_TEXFORMAT(flags))
		{
			case TEXFORMAT_ARGB32:
				src32 = (UINT32 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = *src32++;
				break;

			case TEXFORMAT_PALETTE16:
				src16 = (UINT16 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = texsource->palette[*src16++] | 0xff000000;
				break;

			case TEXFORMAT_RGB15:
				src16 = (UINT16 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
				{
					UINT16 pix = *src16++;
					UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
					*dst32++ = color | ((color >> 5) & 0x070707) | 0xff000000;
				}
				break;

			case TEXFORMAT_RGB32:
				src32 = (UINT32 *)texsource->base + y * texsource->rowpixels;
				for (x = 0; x < texsource->width; x++)
					*dst32++ = *src32++ | 0xff000000;
				break;

			default:
				fprintf(stderr, "Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
				break;
		}

		// always fill non-wrapping textures with an extra pixel on the right
		if (texture->borderpix)
			*dst32++ = 0;
	}

	// always fill non-wrapping textures with an extra pixel on the bottom
	if (texture->borderpix)
	{
		dst32 = (UINT32 *)((BYTE *)rect.pBits + (texsource->height + 1) * rect.Pitch);
		for (x = 0; x < texsource->width + 2; x++)
			*dst32++ = 0;
	}

	// unlock
	switch (texture->type)
	{
		default:
		case TEXTURE_TYPE_PLAIN:	result = (*d3dintf->texture.unlock_rect)(texture->d3dtex, 0);	break;
		case TEXTURE_TYPE_DYNAMIC:	result = (*d3dintf->texture.unlock_rect)(texture->d3dtex, 0);	break;
		case TEXTURE_TYPE_SURFACE:	result = (*d3dintf->surface.unlock_rect)(texture->d3dsurface);	break;
	}
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during texture unlock_rect call\n", (int)result);

	// prescale
	texture_prescale(d3d, texture);
}



//============================================================
//  texture_prescale
//============================================================

static void texture_prescale(d3d_info *d3d, texture_info *texture)
{
	d3d_surface *surface;
	HRESULT result;
	int i;

	// if we don't need to, just skip it
	if (texture->d3dtex == texture->d3dfinaltex)
		return;

	// for all cases, we need to get the surface of the render target
	result = (*d3dintf->texture.get_surface_level)(texture->d3dfinaltex, 0, &surface);
	if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during texture get_surface_level call\n", (int)result);

	// if we have an offscreen plain surface, we can just StretchRect to it
	if (texture->type == TEXTURE_TYPE_SURFACE)
	{
		RECT source, dest;

		assert(texture->d3dsurface != NULL);

		// set the source bounds
		source.left = source.top = 0;
		source.right = texture->texinfo.width + 2;
		source.bottom = texture->texinfo.height + 2;

		// set the target bounds
		dest = source;
		dest.right *= texture->xprescale;
		dest.bottom *= texture->yprescale;

		// do the stretchrect
		result = (*d3dintf->device.stretch_rect)(d3d->device, texture->d3dsurface, &source, surface, &dest, D3DTEXF_POINT);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device stretct_rect call\n", (int)result);
	}

	// if we are using a texture render target, we need to do more preparations
	else
	{
		d3d_surface *backbuffer;

		assert(texture->d3dtex != NULL);

		// first remember the original render target and set the new one
		result = (*d3dintf->device.get_render_target)(d3d->device, 0, &backbuffer);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device get_render_target call\n", (int)result);
		result = (*d3dintf->device.set_render_target)(d3d->device, 0, surface);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_render_target call\n", (int)result);
		reset_render_states(d3d);

		// start the scene
		result = (*d3dintf->device.begin_scene)(d3d->device);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device begin_scene call\n", (int)result);

		// configure the rendering pipeline
		set_filter(d3d, FALSE);
		set_blendmode(d3d, BLENDMODE_NONE);
		result = (*d3dintf->device.set_texture)(d3d->device, 0, texture->d3dtex);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_texture call\n", (int)result);

		// lock the vertex buffer
		result = (*d3dintf->vertexbuf.lock)(d3d->vertexbuf, 0, 0, (VOID **)&d3d->lockedbuf, D3DLOCK_DISCARD);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during vertex buffer lock call\n", (int)result);

		// configure the X/Y coordinates on the target surface
		d3d->lockedbuf[0].x = -0.5f;
		d3d->lockedbuf[0].y = -0.5f;
		d3d->lockedbuf[1].x = (float)((texture->texinfo.width + 2) * texture->xprescale) - 0.5f;
		d3d->lockedbuf[1].y = -0.5f;
		d3d->lockedbuf[2].x = -0.5f;
		d3d->lockedbuf[2].y = (float)((texture->texinfo.height + 2) * texture->yprescale) - 0.5f;
		d3d->lockedbuf[3].x = (float)((texture->texinfo.width + 2) * texture->xprescale) - 0.5f;
		d3d->lockedbuf[3].y = (float)((texture->texinfo.height + 2) * texture->yprescale) - 0.5f;

		// configure the U/V coordintes on the source texture
		d3d->lockedbuf[0].u0 = 0.0f;
		d3d->lockedbuf[0].v0 = 0.0f;
		d3d->lockedbuf[1].u0 = (float)(texture->texinfo.width + 2) / (float)texture->rawwidth;
		d3d->lockedbuf[1].v0 = 0.0f;
		d3d->lockedbuf[2].u0 = 0.0f;
		d3d->lockedbuf[2].v0 = (float)(texture->texinfo.height + 2) / (float)texture->rawheight;
		d3d->lockedbuf[3].u0 = (float)(texture->texinfo.width + 2) / (float)texture->rawwidth;
		d3d->lockedbuf[3].v0 = (float)(texture->texinfo.height + 2) / (float)texture->rawheight;

		// reset the remaining vertex parameters
		for (i = 0; i < 4; i++)
		{
			d3d->lockedbuf[i].z = 0.0f;
			d3d->lockedbuf[i].rhw = 1.0f;
			d3d->lockedbuf[i].color = D3DCOLOR_ARGB(0xff,0xff,0xff,0xff);
		}

		// unlock the vertex buffer
		result = (*d3dintf->vertexbuf.unlock)(d3d->vertexbuf);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during vertex buffer unlock call\n", (int)result);
		d3d->lockedbuf = NULL;

		// set the stream and draw the triangle strip
		result = (*d3dintf->device.set_stream_source)(d3d->device, 0, d3d->vertexbuf, sizeof(d3d_vertex));
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_stream_source call\n", (int)result);
		result = (*d3dintf->device.draw_primitive)(d3d->device, D3DPT_TRIANGLESTRIP, 0, 2);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device draw_primitive call\n", (int)result);

		// end the scene
		result = (*d3dintf->device.end_scene)(d3d->device);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device end_scene call\n", (int)result);

		// reset the render target and release our reference to the backbuffer
		result = (*d3dintf->device.set_render_target)(d3d->device, 0, backbuffer);
		if (result != D3D_OK) verbose_printf("Direct3D: Error %08X during device set_render_target call\n", (int)result);
		(*d3dintf->surface.release)(backbuffer);
		reset_render_states(d3d);
	}

	// release our reference to the target surface
	(*d3dintf->surface.release)(surface);
}



//============================================================
//  texture_find
//============================================================

static texture_info *texture_find(d3d_info *d3d, const render_primitive *prim)
{
	UINT32 texhash = texture_compute_hash(&prim->texture, prim->flags);
	texture_info *texture;

	// find a match
	for (texture = d3d->texlist; texture != NULL; texture = texture->next)
		if (texture->hash == texhash &&
			texture->texinfo.base == prim->texture.base &&
			texture->texinfo.width == prim->texture.width &&
			texture->texinfo.height == prim->texture.height &&
			((texture->flags ^ prim->flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
			return texture;

	// nothing found
	return NULL;
}



//============================================================
//  texture_update
//============================================================

static void texture_update(d3d_info *d3d, const render_primitive *prim)
{
	texture_info *texture = texture_find(d3d, prim);

	// if we didn't find one, create a new texture
	if (texture == NULL)
		texture = texture_create(d3d, &prim->texture, prim->flags);

	// if we found it, but with a different seqid, copy the data
	if (texture->texinfo.seqid != prim->texture.seqid)
	{
		texture_set_data(d3d, texture, &prim->texture, prim->flags);
		texture->texinfo.seqid = prim->texture.seqid;
	}
}
