//============================================================
//
//  config.c - Win32 configuration routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// standard C headers
#include <stdarg.h>
#include <ctype.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "render.h"
#include "rendutil.h"
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#include "emuopts.h"

// MAMEOS headers
#include "winmain.h"
#include "video.h"
#include "osd_so.h"


//============================================================
//  EXTERNALS
//============================================================

int frontend_listxml(FILE *output);
int frontend_listgames(FILE *output);
int frontend_listfull(FILE *output);
int frontend_listsource(FILE *output);
int frontend_listclones(FILE *output);
int frontend_listcrc(FILE *output);
int frontend_listroms(FILE *output);
int frontend_listsamples(FILE *output);
int frontend_verifyroms(FILE *output);
int frontend_verifysamples(FILE *output);
int frontend_romident(FILE *output);
int frontend_isknown(FILE *output);

#ifdef MESS
int frontend_listdevices(FILE *output);
#endif /* MESS */

void set_pathlist(int file_type, const char *new_rawpath);



//============================================================
//  PROTOTYPES
//============================================================

static void parse_ini_file(const char *name);
static void execute_simple_commands(void);
static void execute_commands(const char *argv0);
#ifdef DRIVER_SWITCH
void assign_drivers(void);
#endif /* DRIVER_SWITCH */
static void display_help(void);
static void setup_language(void);
static char *extract_base_name(const char *name, char *dest, int destsize);
static char *extract_path(const char *name, char *dest, int destsize);



//============================================================
//  OPTIONS
//============================================================

// struct definitions
const options_entry mame_win_options[] =
{
	// core commands
	{ NULL,                       NULL,       OPTION_HEADER,     "CORE COMMANDS" },
	{ "help;h;?",                 "0",        OPTION_COMMAND,    "show help message" },
	{ "validate;valid",           "0",        OPTION_COMMAND,    "perform driver validation on all game drivers" },

	// configuration commands
	{ NULL,                       NULL,       OPTION_HEADER,     "CONFIGURATION COMMANDS" },
	{ "createconfig;cc",          "0",        OPTION_COMMAND,    "create the default configuration file" },
	{ "showconfig;sc",            "0",        OPTION_COMMAND,    "display running parameters" },
	{ "showusage;su",             "0",        OPTION_COMMAND,    "show this help" },

	// frontend commands
	{ NULL,                       NULL,       OPTION_HEADER,     "FRONTEND COMMANDS" },
	{ "listxml;lx",               "0",        OPTION_COMMAND,    "all available info on driver in XML format" },
	{ "listgames",                "0",        OPTION_COMMAND,    "year, manufacturer and full name" },
	{ "listfull;ll",              "0",        OPTION_COMMAND,    "short name, full name" },
	{ "listsource;ls",            "0",        OPTION_COMMAND,    "driver sourcefile" },
	{ "listclones;lc",            "0",        OPTION_COMMAND,    "show clones" },
	{ "listcrc",                  "0",        OPTION_COMMAND,    "CRC-32s" },
	{ "listroms",                 "0",        OPTION_COMMAND,    "list required roms for a driver" },
	{ "listsamples",              "0",        OPTION_COMMAND,    "list optional samples for a driver" },
	{ "verifyroms",               "0",        OPTION_COMMAND,    "report romsets that have problems" },
	{ "verifysamples",            "0",        OPTION_COMMAND,    "report samplesets that have problems" },
	{ "romident",                 "0",        OPTION_COMMAND,    "compare files with known MAME roms" },
	{ "isknown",                  "0",        OPTION_COMMAND,    "compare files with known MAME roms (brief)" },
#ifdef MESS
	{ "listdevices",              "0",        OPTION_COMMAND,    "list available devices" },
#endif

	// config options
	{ NULL,                       NULL,       OPTION_HEADER,     "CONFIGURATION OPTIONS" },
	{ "readconfig;rc",            "1",        OPTION_BOOLEAN,    "enable loading of configuration files" },

	// debugging options
	{ NULL,                       NULL,       OPTION_HEADER,     "DEBUGGING OPTIONS" },
	{ "oslog",                    "0",        OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",        OPTION_BOOLEAN,    "display additional diagnostic information" },

	// performance options
	{ NULL,                       NULL,       OPTION_HEADER,     "WINDOWS PERFORMANCE OPTIONS" },
	{ "priority",                 "0",        0,                 "thread priority for the main game thread; range from -15 to 1" },
	{ "multithreading;mt",        "0",        OPTION_BOOLEAN,    "enable multithreading; this enables rendering and blitting on a separate thread" },

	// video options
	{ NULL,                       NULL,       OPTION_HEADER,     "WINDOWS VIDEO OPTIONS" },
	{ "video",                    "d3d",      0,                 "video output method: none, gdi, ddraw, or d3d" },
	{ "numscreens",               "1",        0,                 "number of screens to create; usually, you want just one" },
	{ "window;w",                 "0",        OPTION_BOOLEAN,    "enable window mode; otherwise, full screen mode is assumed" },
	{ "maximize;max",             "1",        OPTION_BOOLEAN,    "default to maximized windows; otherwise, windows will be minimized" },
	{ "keepaspect;ka",            "1",        OPTION_BOOLEAN,    "constrain to the proper aspect ratio" },
	{ "prescale",                 "1",        0,                 "scale screen rendering by this amount in software" },
	{ "effect",                   "none",     0,                 "name of a PNG file to use for visual effects, or 'none'" },
	{ "waitvsync",                "0",        OPTION_BOOLEAN,    "enable waiting for the start of VBLANK before flipping screens; reduces tearing effects" },
	{ "syncrefresh",              "0",        OPTION_BOOLEAN,    "enable using the start of VBLANK for throttling instead of the game time" },

	// DirectDraw-specific options
	{ NULL,                       NULL,       OPTION_HEADER,     "DIRECTDRAW-SPECIFIC OPTIONS" },
	{ "hwstretch;hws",            "1",        OPTION_BOOLEAN,    "enable hardware stretching" },

	// Direct3D-specific options
	{ NULL,                       NULL,       OPTION_HEADER,     "DIRECT3D-SPECIFIC OPTIONS" },
	{ "d3dversion",               "9",        0,                 "specify the preferred Direct3D version (8 or 9)" },
	{ "filter;d3dfilter;flt",     "1",        OPTION_BOOLEAN,    "enable bilinear filtering on screen output" },

	// per-window options
	{ NULL,                       NULL,       OPTION_HEADER,     "PER-WINDOW VIDEO OPTIONS" },
	{ "screen",                   "auto",     0,                 "explicit name of all screen; 'auto' here will try to make a best guess" },
	{ "aspect;screen_aspect",     "auto",     0,                 "aspect ratio for all screens; 'auto' here will try to make a best guess" },
	{ "resolution;r",             "auto",     0,                 "preferred resolution for all screens; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view",                     "auto",     0,                 "preferred view for all screens" },

	{ "screen0",                  "auto",     0,                 "explicit name of the first screen; 'auto' here will try to make a best guess" },
	{ "aspect0",                  "auto",     0,                 "aspect ratio of the first screen; 'auto' here will try to make a best guess" },
	{ "resolution0;r0",           "auto",     0,                 "preferred resolution of the first screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view0",                    "auto",     0,                 "preferred view for the first screen" },

	{ "screen1",                  "auto",     0,                 "explicit name of the second screen; 'auto' here will try to make a best guess" },
	{ "aspect1",                  "auto",     0,                 "aspect ratio of the second screen; 'auto' here will try to make a best guess" },
	{ "resolution1;r1",           "auto",     0,                 "preferred resolution of the second screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view1",                    "auto",     0,                 "preferred view for the second screen" },

	{ "screen2",                  "auto",     0,                 "explicit name of the third screen; 'auto' here will try to make a best guess" },
	{ "aspect2",                  "auto",     0,                 "aspect ratio of the third screen; 'auto' here will try to make a best guess" },
	{ "resolution2;r2",           "auto",     0,                 "preferred resolution of the third screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view2",                    "auto",     0,                 "preferred view for the third screen" },

	{ "screen3",                  "auto",     0,                 "explicit name of the fourth screen; 'auto' here will try to make a best guess" },
	{ "aspect3",                  "auto",     0,                 "aspect ratio of the fourth screen; 'auto' here will try to make a best guess" },
	{ "resolution3;r3",           "auto",     0,                 "preferred resolution of the fourth screen; format is <width>x<height>[@<refreshrate>] or 'auto'" },
	{ "view3",                    "auto",     0,                 "preferred view for the fourth screen" },

	// full screen options
	{ NULL,                       NULL,       OPTION_HEADER,     "FULL SCREEN OPTIONS" },
	{ "triplebuffer;tb",          "0",        OPTION_BOOLEAN,    "enable triple buffering" },
	{ "switchres",                "0",        OPTION_BOOLEAN,    "enable resolution switching" },
	{ "full_screen_brightness;fsb","1.0",     0,                 "brightness value in full screen mode" },
	{ "full_screen_contrast;fsc", "1.0",      0,                 "contrast value in full screen mode" },
	{ "full_screen_gamma;fsg",    "1.0",      0,                 "gamma value in full screen mode" },

	// sound options
	{ NULL,                       NULL,       OPTION_HEADER,     "WINDOWS SOUND OPTIONS" },
	{ "audio_latency",            "1",        0,                 "set audio latency (increase to reduce glitches)" },

	// input options
	{ NULL,                       NULL,       OPTION_HEADER,     "INPUT DEVICE OPTIONS" },
	{ "mouse",                    "0",        OPTION_BOOLEAN,    "enable mouse input" },
	{ "joystick;joy",             "0",        OPTION_BOOLEAN,    "enable joystick input" },
#ifdef USE_JOY_MOUSE_MOVE
	{ "stickpoint",               "0",        OPTION_BOOLEAN,    "enable pointing stick input" },
#else /* USE_JOY_MOUSE_MOVE */
	{ "stickpoint",               "0",        OPTION_DEPRECATED, "(disabled by compiling option)" },
#endif /* USE_JOY_MOUSE_MOVE */
	{ "lightgun;gun",             "0",        OPTION_BOOLEAN,    "enable lightgun input" },
	{ "dual_lightgun;dual",       "0",        OPTION_BOOLEAN,    "enable dual lightgun input" },
	{ "offscreen_reload;reload",  "0",        OPTION_BOOLEAN,    "offscreen shots reload" },
	{ "steadykey;steady",         "0",        OPTION_BOOLEAN,    "enable steadykey support" },
	{ "joy_deadzone;jdz",         "0.3",      0,                 "center deadzone range for joystick where change is ignored (0.0 center, 1.0 end)" },
	{ "joy_saturation;jsat",      "0.85",     0,                 "end of axis saturation range for joystick where change is ignored (0.0 center, 1.0 end)" },
	{ "digital",                  "none",     0,                 "mark certain joysticks or axes as digital (none|all|j<N>*|j<N>a<M>[,...])" },

#ifdef JOYSTICK_ID
	{ "joyid1",                   "0",        0,                 "set joystick ID (Player1)" },
	{ "joyid2",                   "1",        0,                 "set joystick ID (Player2)" },
	{ "joyid3",                   "2",        0,                 "set joystick ID (Player3)" },
	{ "joyid4",                   "3",        0,                 "set joystick ID (Player4)" },
	{ "joyid5",                   "4",        0,                 "set joystick ID (Player5)" },
	{ "joyid6",                   "5",        0,                 "set joystick ID (Player6)" },
	{ "joyid7",                   "6",        0,                 "set joystick ID (Player7)" },
	{ "joyid8",                   "7",        0,                 "set joystick ID (Player8)" },
#else /* JOYSTICK_ID */
	{ "joyid1",                   "0",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid2",                   "1",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid3",                   "2",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid4",                   "3",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid5",                   "4",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid6",                   "5",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid7",                   "6",        OPTION_DEPRECATED, "(disabled by compiling option)" },
	{ "joyid8",                   "7",        OPTION_DEPRECATED, "(disabled by compiling option)" },
#endif /* JOYSTICK_ID */

	{ NULL,                       NULL,       OPTION_HEADER,     "AUTOMATIC DEVICE SELECTION OPTIONS" },
	{ "paddle_device;paddle",     "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a paddle control is present" },
	{ "adstick_device;adstick",   "keyboard", 0,                 "enable (keyboard|mouse|joystick) if an analog joystick control is present" },
	{ "pedal_device;pedal",       "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a pedal control is present" },
	{ "dial_device;dial",         "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a dial control is present" },
	{ "trackball_device;trackball","keyboard", 0,                "enable (keyboard|mouse|joystick) if a trackball control is present" },
	{ "lightgun_device",          "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a lightgun control is present" },
	{ "positional_device",        "keyboard", 0,                 "enable (keyboard|mouse|joystick) if a positional control is present" },
#ifdef MESS
	{ "mouse_device",             "mouse",    0,                 "enable (keyboard|mouse|joystick) if a mouse control is present" },
#endif

	{ NULL }
};


#ifdef MESS
#include "configms.h"
#endif



//============================================================
//  INLINES
//============================================================

INLINE int is_directory_separator(char c)
{
	return (c == '\\' || c == '/' || c == ':');
}



//============================================================
//  win_options_init
//============================================================

void win_options_init(void)
{
	mame_options_init(mame_win_options);
}



//============================================================
//  cli_frontend_init
//============================================================

int cli_frontend_init(int argc, char **argv)
{
	const char *gamename;
	machine_config drv;
	char basename[20];
	char buffer[512];
	int drvnum = -1;

	// initialize the options manager
	win_options_init();
#ifdef MESS
	win_mess_options_init();
#endif // MESS

	// parse the command line first; if we fail here, we're screwed
	if (options_parse_command_line(mame_options(), argc, argv))
		exit(MAMERR_INVALID_CONFIG);

	// now parse the core set of INI files
	parse_ini_file(CONFIGNAME);
	parse_ini_file(extract_base_name(argv[0], buffer, ARRAY_LENGTH(buffer)));
#ifdef MAME_DEBUG
	parse_ini_file("debug");
#endif

	// reparse the command line to ensure its options override all
	// note that we re-fetch the gamename here as it will get overridden
	options_parse_command_line(mame_options(), argc, argv);
	setup_language();

#ifdef DRIVER_SWITCH
	assign_drivers();
#endif /* DRIVER_SWITCH */

	// parse the simple commmands before we go any further
	execute_simple_commands();

	// find out what game we might be referring to
	gamename = options_get_string(mame_options(), OPTION_GAMENAME);
	if (gamename != NULL)
		drvnum = driver_get_index(extract_base_name(gamename, basename, ARRAY_LENGTH(basename)));

	// if we have a valid game driver, parse game-specific INI files
	if (drvnum != -1)
	{
		const game_driver *driver = drivers[drvnum];
		const game_driver *parent = driver_get_clone(driver);
		const game_driver *gparent = (parent != NULL) ? driver_get_clone(parent) : NULL;

		// expand the machine driver to look at the info
		expand_machine_driver(driver->drv, &drv);

		// parse vector.ini for vector games
		if (drv.video_attributes & VIDEO_TYPE_VECTOR)
			parse_ini_file("vector");

		// then parse sourcefile.ini
		parse_ini_file(extract_base_name(driver->source_file, buffer, ARRAY_LENGTH(buffer)));

		// then parent the grandparent, parent, and game-specific INIs
		if (gparent != NULL)
			parse_ini_file(gparent->name);
		if (parent != NULL)
			parse_ini_file(parent->name);

#ifdef USE_IPS
		// HACK: DO NOT INHERIT IPS CONFIGURATION
		options_set_string(mame_options(), "ips", NULL);
#endif /* USE_IPS */

		parse_ini_file(driver->name);
	}

	// reparse the command line to ensure its options override all
	// note that we re-fetch the gamename here as it will get overridden
	options_parse_command_line(mame_options(), argc, argv);
	setup_language();

	// execute any commands specified
	execute_commands(argv[0]);

	// if no driver specified, display help
	if (gamename == NULL)
	{
		display_help();
		exit(MAMERR_INVALID_CONFIG);
	}

	// if we don't have a valid driver selected, offer some suggestions
	if (drvnum == -1)
	{
		int matches[10];
		faprintf(stderr, _WINDOWS("\n\"%s\" approximately matches the following\n"
				"supported " GAMESNOUN " (best match first):\n\n"), basename);
		driver_get_approx_matches(basename, ARRAY_LENGTH(matches), matches);
		for (drvnum = 0; drvnum < ARRAY_LENGTH(matches); drvnum++)
			if (matches[drvnum] != -1)
				faprintf(stderr, "%-10s%s\n", drivers[matches[drvnum]]->name, _LST(drivers[matches[drvnum]]->description));
		exit(MAMERR_NO_SUCH_GAME);
	}

	// extract the directory name
	extract_path(gamename, buffer, ARRAY_LENGTH(buffer));
	if (buffer[0] != 0)
	{
		// okay, we got one; prepend it to the rompath
		const char *rompath = options_get_string(mame_options(), "rompath");
		if (rompath == NULL)
			options_set_string(mame_options(), "rompath", buffer);
		else
		{
			char *newpath = malloc_or_die(strlen(rompath) + strlen(buffer) + 2);
			sprintf(newpath, "%s;%s", buffer, rompath);
			options_set_string(mame_options(), "rompath", newpath);
			free(newpath);
		}
	}

	// debugging options
{
	extern int verbose;
	verbose = options_get_bool(mame_options(), "verbose");
}

	// thread priority
	if (!options_get_bool(mame_options(), OPTION_DEBUG))
		SetThreadPriority(GetCurrentThread(), options_get_int_range(mame_options(), "priority", -15, 1));

	return drvnum;
}



//============================================================
//  cli_frontend_exit
//============================================================

void cli_frontend_exit(void)
{
#ifdef DRIVER_SWITCH
	free(drivers);
#endif /* DRIVER_SWITCH */

	// free the options that we added previously
	options_free(mame_options());
}



//============================================================
//  parse_ini_file
//============================================================

static void parse_ini_file(const char *name)
{
	file_error filerr;
	mame_file *file;
	char *fname;

	// don't parse if it has been disabled
	if (!options_get_bool(mame_options(), "readconfig"))
		return;

	// open the file; if we fail, that's ok
	fname = assemble_2_strings(name, ".ini");
	filerr = mame_fopen(strcmp(CONFIGNAME, name) == 0 ? SEARCHPATH_RAW : SEARCHPATH_INI, fname, OPEN_FLAG_READ, &file);
	free(fname);
	if (filerr != FILERR_NONE)
		return;

	// parse the file and close it
	options_parse_ini_file(mame_options(), mame_core_file(file));
	mame_fclose(file);
}



//============================================================
//  mame_puts_info
//============================================================

static void mame_puts_info(const char *s)
{
	mame_printf_info("%s", s);
}



//============================================================
//  execute_simple_commands
//============================================================

static void execute_simple_commands(void)
{
	// help?
	if (options_get_bool(mame_options(), "help"))
	{
		setup_language();
		display_help();
		exit(MAMERR_NONE);
	}

	// showusage?
	if (options_get_bool(mame_options(), "showusage"))
	{
		setup_language();
		mame_printf_info(_WINDOWS("Usage: %s [%s] [options]\n\nOptions:\n"), APPNAME_LOWER, GAMENOUN);
		options_output_help(mame_options(), mame_puts_info);
		exit(MAMERR_NONE);
	}

	// validate?
	if (options_get_bool(mame_options(), "validate"))
	{
		extern int mame_validitychecks(int game);
		exit(mame_validitychecks(-1));
	}
}



//============================================================
//  execute_commands
//============================================================

static void execute_commands(const char *argv0)
{
	// frontend options
	static const struct
	{
		const char *option;
		int (*function)(FILE *output);
	} frontend_options[] =
	{
		{ "listxml",		frontend_listxml },
		{ "listgames",		frontend_listgames },
		{ "listfull",		frontend_listfull },
		{ "listsource",		frontend_listsource },
		{ "listclones",		frontend_listclones },
		{ "listcrc",		frontend_listcrc },
#ifdef MESS
		{ "listdevices",	frontend_listdevices },
#endif
		{ "listroms",		frontend_listroms },
		{ "listsamples",	frontend_listsamples },
		{ "verifyroms",		frontend_verifyroms },
		{ "verifysamples",	frontend_verifysamples },
		{ "romident",		frontend_romident },
		{ "isknown",		frontend_isknown  }
	};
	int i;

	// createconfig?
	if (options_get_bool(mame_options(), "createconfig"))
	{
		char basename[128];
		mame_file *file;
		file_error filerr;

		// make the base name
		extract_base_name(argv0, basename, ARRAY_LENGTH(basename) - 5);
		strcat(basename, ".ini");
		filerr = mame_fopen(NULL, basename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);

		// error if unable to create the file
		if (filerr)
		{
			faprintf(stderr, _WINDOWS("Unable to create file %s\n"), basename);
			exit(MAMERR_FATALERROR);
		}

		// output the configuration and exit cleanly
		options_output_ini_file(mame_options(), mame_core_file(file));
		mame_fclose(file);
		exit(MAMERR_NONE);
	}

	// showconfig?
	if (options_get_bool(mame_options(), "showconfig"))
	{
		options_output_ini_stdfile(mame_options(), stdout);
		exit(MAMERR_NONE);
	}

	// frontend options?
	for (i = 0; i < ARRAY_LENGTH(frontend_options); i++)
		if (options_get_bool(mame_options(), frontend_options[i].option))
		{
			int result;

			init_resource_tracking();
			cpuintrf_init(NULL);
			sndintrf_init(NULL);
			result = (*frontend_options[i].function)(stdout);
			exit_resource_tracking();
			exit(result);
		}
}



//============================================================
//  display_help
//============================================================

static void display_help(void)
{
#ifndef MESS
	mame_printf_info(_WINDOWS("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
		   "Copyright (C) 1997-2007 by Nicola Salmoria and the MAME Team\n\n"),build_version);
	mame_printf_info("%s\n", _(mame_disclaimer));
	mame_printf_info(_WINDOWS("Usage:  MAME gamename [options]\n\n"
		   "        MAME -showusage    for a brief list of options\n"
		   "        MAME -showconfig   for a list of configuration options\n"
		   "        MAME -createconfig to create a mame.ini\n\n"
		   "For usage instructions, please consult the file windows.txt\n"));
#else
	showmessinfo();
#endif
}

#ifdef DRIVER_SWITCH
void assign_drivers(void)
{
	static const struct
	{
		const char *name;
		const game_driver * const *driver;
	} drivers_table[] =
	{
		{ "mame",	mamedrivers },
#ifndef TINY_NAME
		{ "plus",	plusdrivers },
		{ "homebrew",	homebrewdrivers },
		{ "neod",	neoddrivers },
	#ifndef NEOCPSMAME
		{ "noncpu",	noncpudrivers },
		{ "hazemd",	hazemddrivers },
	#endif /* NEOCPSMAME */
#endif /* !TINY_NAME */
		{ NULL }
	};

	UINT32 enabled = 0;
	int i, n;

#ifndef TINY_NAME
	const char *drv_option = options_get_string(mame_options(), "driver_config");
	if (drv_option)
	{
		char *temp = mame_strdup(drv_option);
		if (temp)
		{
			char *p = strtok(temp, ",");

			while (p)
			{
				char *s = mame_strtrim(p);	//get individual driver name
				if (s[0])
				{
					if (mame_stricmp(s, "all") == 0)
					{
						enabled = (UINT32)-1;
						break;
					}

					for (i = 0; drivers_table[i].name; i++)
						if (mame_stricmp(s, drivers_table[i].name) == 0)
						{
							enabled |= 1 << i;
							break;
						}

					if (!drivers_table[i].name)
						faprintf(stderr, _WINDOWS("Illegal value for %s = %s\n"), "driver_config", s);
				}
				free(s);

				p = strtok(NULL, ",");
			}

			free(temp);
		}
	}
#endif /* !TINY_NAME */

	if (enabled == 0)
		enabled = 1;	// default to mamedrivers

	n = 0;
	for (i = 0; drivers_table[i].name; i++)
		if (enabled & (1 << i))
		{
			int c;

			for (c = 0; drivers_table[i].driver[c]; c++)
				n++;
		}

	if (drivers)
		free(drivers);
	drivers = malloc((n + 1) * sizeof (game_driver*));

	n = 0;
	for (i = 0; drivers_table[i].name; i++)
		if (enabled & (1 << i))
		{
			int c;

			for (c = 0; drivers_table[i].driver[c]; c++)
				drivers[n++] = drivers_table[i].driver[c];
		}

	drivers[n] = NULL;
}
#endif /* DRIVER_SWITCH */



//============================================================
//  setup_language
//============================================================

static void setup_language(void)
{
	const char *langname = options_get_string(mame_options(), "language");
	int use_lang_list =options_get_bool(mame_options(), "use_lang_list");

	int langcode = mame_stricmp(langname, "auto") ?
		lang_find_langname(langname) :
		lang_find_codepage(GetOEMCP());

	if (langcode < 0)
	{
		langcode = UI_LANG_EN_US;
		lang_set_langcode(langcode);
		set_osdcore_acp(ui_lang_info[langcode].codepage);

		if (mame_stricmp(langname, "auto"))
			faprintf(stderr, _WINDOWS("error: invalid value for language: %s\nUse %s\n"),
		                langname, ui_lang_info[langcode].description);
	}

	lang_set_langcode(langcode);
	set_osdcore_acp(ui_lang_info[langcode].codepage);

	lang_message_enable(UI_MSG_LIST, use_lang_list);
	lang_message_enable(UI_MSG_MANUFACTURE, use_lang_list);
}


//============================================================
//  extract_base_name
//============================================================

static char *extract_base_name(const char *name, char *dest, int destsize)
{
	const char *start;
	int i;

	// extract the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// copy in the base name
	for (i = 0; i < destsize; i++)
	{
		if (start[i] == 0 || start[i] == '.')
			break;
		else
			dest[i] = start[i];
	}

	// NULL terminate
	if (i < destsize)
		dest[i] = 0;
	else
		dest[destsize - 1] = 0;

	return dest;
}



//============================================================
//  extract_path
//============================================================

static char *extract_path(const char *name, char *dest, int destsize)
{
	const char *start;

	// find the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// handle the null path case
	if (start == name)
		*dest = 0;

	// else just copy the path part
	else
	{
		int bytes = start - 1 - name;
		bytes = MIN(destsize - 1, bytes);
		memcpy(dest, name, bytes);
		dest[bytes] = 0;
	}
	return dest;
}
