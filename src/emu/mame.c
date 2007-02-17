/***************************************************************************

    mame.c

    Controls execution of the core MAME system.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Since there has been confusion in the past over the order of
    initialization and other such things, here it is, all spelled out
    as of February, 2006:

    main()
        - does platform-specific init
        - calls run_game() [mame.c]

        run_game() [mame.c]
            - calls mame_validitychecks() [validity.c] to perform validity checks on all compiled drivers
            - calls setjmp to prepare for deep error handling
            - begins resource tracking (level 1)
            - calls create_machine [mame.c] to initialize the Machine structure
            - calls init_machine() [mame.c]

            init_machine() [mame.c]
                - calls cpuintrf_init() [cpuintrf.c] to determine which CPUs are available
                - calls sndintrf_init() [sndintrf.c] to determine which sound chips are available
                - calls fileio_init() [fileio.c] to initialize file I/O info
                - calls config_init() [config.c] to initialize configuration system
                - calls state_init() [state.c] to initialize save state system
                - calls state_save_allow_registration() [state.c] to allow registrations
                - calls drawgfx_init() [drawgfx.c] to initialize rendering globals
                - calls palette_init() [palette.c] to initialize palette system
                - calls render_init() [render.c] to initialize the rendering system
                - calls ui_init() [ui.c] to initialize the user interface
                - calls generic_machine_init() [machine/generic.c] to initialize generic machine structures
                - calls generic_video_init() [video/generic.c] to initialize generic video structures
                - calls osd_init() [osdepend.h] to do platform-specific initialization
                - calls code_init() [input.c] to initialize the input system
                - calls input_port_init() [inptport.c] to set up the input ports
                - calls rom_init() [romload.c] to load the game's ROMs
                - calls timer_init() [timer.c] to reset the timer system
                - calls memory_init() [memory.c] to process the game's memory maps
                - calls cpuexec_init() [cpuexec.c] to initialize the CPUs
                - calls cpuint_init() [cpuint.c] to initialize the CPU interrupts
                - calls saveload_init() [mame.c] to set up for save/load
                - calls the driver's DRIVER_INIT callback
                - calls sound_init() [sound.c] to start the audio system
                - calls video_init() [video.c] to start the video system
                - calls cheat_init() [cheat.c] to initialize the cheat system
                - calls the driver's MACHINE_START, SOUND_START, and VIDEO_START callbacks
                - disposes of regions marked as disposable
                - calls mame_debug_init() [debugcpu.c] to set up the debugger

            - calls config_load_settings() [config.c] to load the configuration file
            - calls nvram_load [machine/generic.c] to load NVRAM
            - calls ui_init() [ui.c] to initialize the user interface
            - begins resource tracking (level 2)
            - calls soft_reset() [mame.c] to reset all systems

                -------------------( at this point, we're up and running )----------------------

            - calls cpuexec_timeslice() [cpuexec.c] over and over until we exit
            - ends resource tracking (level 2), freeing all auto_mallocs and timers
            - calls the nvram_save() [machine/generic.c] to save NVRAM
            - calls config_save_settings() [config.c] to save the game's configuration
            - calls all registered exit routines [mame.c]
            - ends resource tracking (level 1), freeing all auto_mallocs and timers

        - exits the program

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "config.h"
#include "cheat.h"
#include "debugger.h"
#include "profiler.h"
#include "render.h"
#include "ui.h"

#ifdef MAME_DEBUG
#include "debug/debugcon.h"
#endif

#include <stdarg.h>
#include <setjmp.h>
#include <time.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_MEMORY_REGIONS		32



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _region_info region_info;
struct _region_info
{
	UINT8 *			base;
	UINT32			length;
	UINT32			type;
	UINT32			flags;
};


typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	union
	{
		void		(*exit)(running_machine *);
		void		(*reset)(running_machine *);
		void		(*pause)(running_machine *, int);
		void		(*log)(running_machine *, const char *);
	} func;
};


/* typedef struct _mame_private mame_private; */
struct _mame_private
{
	/* system state */
	int				current_phase;
	UINT8			paused;
	UINT8			hard_reset_pending;
	UINT8			exit_pending;
	char *			saveload_pending_file;
	mame_timer *	soft_reset_timer;

	/* callbacks */
	callback_item *	reset_callback_list;
	callback_item *	pause_callback_list;
	callback_item *	exit_callback_list;
	callback_item *	logerror_callback_list;

	/* load/save */
	void 			(*saveload_schedule_callback)(running_machine *);
	mame_time		saveload_schedule_time;

	/* array of memory regions */
	region_info		mem_region[MAX_MEMORY_REGIONS];

	/* error recovery and exiting */
	jmp_buf			fatal_error_jmpbuf;
	int				fatal_error_jmpbuf_valid;

	/* random number seed */
	UINT32			rand_seed;

	/* base time */
	time_t			base_time;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* the active machine */
running_machine *Machine;

/* various game options filled in by the OSD */
global_options options;

/* output channels */
static output_callback output_cb[OUTPUT_CHANNEL_COUNT];
static void *output_cb_param[OUTPUT_CHANNEL_COUNT];

/* the "disclaimer" that should be printed when run with no parameters */
const char *mame_disclaimer =
	"MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
	"several arcade machines. But hardware is useless without software, so an image\n"
	"of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
	"commercial software, are copyrighted material and it is therefore illegal to\n"
	"use them if you don't own the original arcade machine. Needless to say, ROMs\n"
	"are not distributed together with MAME. Distribution of MAME together with ROM\n"
	"images is a violation of copyright law and should be promptly reported to the\n"
	"authors so that appropriate legal action can be taken.\n";

const char *memory_region_names[REGION_MAX] =
{
	"REGION_INVALID",
	"REGION_CPU1",
	"REGION_CPU2",
	"REGION_CPU3",
	"REGION_CPU4",
	"REGION_CPU5",
	"REGION_CPU6",
	"REGION_CPU7",
	"REGION_CPU8",
	"REGION_GFX1",
	"REGION_GFX2",
	"REGION_GFX3",
	"REGION_GFX4",
	"REGION_GFX5",
	"REGION_GFX6",
	"REGION_GFX7",
	"REGION_GFX8",
	"REGION_PROMS",
	"REGION_SOUND1",
	"REGION_SOUND2",
	"REGION_SOUND3",
	"REGION_SOUND4",
	"REGION_SOUND5",
	"REGION_SOUND6",
	"REGION_SOUND7",
	"REGION_SOUND8",
	"REGION_USER1",
	"REGION_USER2",
	"REGION_USER3",
	"REGION_USER4",
	"REGION_USER5",
	"REGION_USER6",
	"REGION_USER7",
	"REGION_USER8",
	"REGION_DISKS",
	"REGION_PLDS"
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

extern int mame_validitychecks(int game);

static running_machine *create_machine(int game);
static void destroy_machine(running_machine *machine);
static void init_machine(running_machine *machine);
static void soft_reset(int param);
static void free_callback_list(callback_item **cb);

static void saveload_init(running_machine *machine);
static void handle_save(running_machine *machine);
static void handle_load(running_machine *machine);

static void logfile_callback(running_machine *machine, const char *buffer);



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    run_game - run the given game in a session
-------------------------------------------------*/

int run_game(int game)
{
	running_machine *machine;
	int error = MAMERR_NONE;
	mame_private *mame;
	callback_item *cb;

	/* create the machine structure and driver */
	machine = create_machine(game);
	mame = machine->mame_data;

	/* looooong term: remove this */
	Machine = machine;

	/* start in the "pre-init phase" */
	mame->current_phase = MAME_PHASE_PREINIT;

	/* perform validity checks before anything else */
	if (mame_validitychecks(game) != 0)
		return MAMERR_FAILED_VALIDITY;

	/* loop across multiple hard resets */
	mame->exit_pending = FALSE;
	while (error == 0 && !mame->exit_pending)
	{
		init_resource_tracking();
		add_free_resources_callback(timer_free);
		add_free_resources_callback(state_save_free);

		/* use setjmp/longjmp for deep error recovery */
		mame->fatal_error_jmpbuf_valid = TRUE;
		error = setjmp(mame->fatal_error_jmpbuf);
		if (error == 0)
		{
			int settingsloaded;

			/* move to the init phase */
			mame->current_phase = MAME_PHASE_INIT;

			/* start tracking resources for real */
			begin_resource_tracking();

			/* if we have a logfile, set up the callback */
			mame->logerror_callback_list = NULL;
			if (options.logfile)
				add_logerror_callback(machine, logfile_callback);

			/* then finish setting up our local machine */
			init_machine(machine);

			/* load the configuration settings and NVRAM */
			settingsloaded = config_load_settings();
			nvram_load();

			/* display the startup screens */
			ui_display_startup_screens(!settingsloaded && !options.skip_disclaimer, !options.skip_warnings, !options.skip_gameinfo);

			/* ensure we don't show the opening screens on a reset */
			options.skip_disclaimer = options.skip_warnings = options.skip_gameinfo = TRUE;

			/* start resource tracking; note that soft_reset assumes it can */
			/* call end_resource_tracking followed by begin_resource_tracking */
			/* to clear out resources allocated between resets */
			begin_resource_tracking();

			/* perform a soft reset -- this takes us to the running phase */
			soft_reset(0);

			/* run the CPUs until a reset or exit */
			mame->hard_reset_pending = FALSE;
			while ((!mame->hard_reset_pending && !mame->exit_pending) || mame->saveload_pending_file != NULL)
			{
				profiler_mark(PROFILER_EXTRA);

				/* execute CPUs if not paused */
				if (!mame->paused)
					cpuexec_timeslice();

				/* otherwise, just pump video updates through */
				else
					video_frame_update();

				/* handle save/load */
				if (mame->saveload_schedule_callback)
					(*mame->saveload_schedule_callback)(machine);

				profiler_mark(PROFILER_END);
			}

			/* and out via the exit phase */
			mame->current_phase = MAME_PHASE_EXIT;

			/* stop tracking resources at this level */
			end_resource_tracking();

			/* save the NVRAM and configuration */
			nvram_save();
			config_save_settings();
		}
		mame->fatal_error_jmpbuf_valid = FALSE;

		/* call all exit callbacks registered */
		for (cb = mame->exit_callback_list; cb; cb = cb->next)
			(*cb->func.exit)(machine);

		/* close all inner resource tracking */
		exit_resource_tracking();

		/* free our callback lists */
		free_callback_list(&mame->exit_callback_list);
		free_callback_list(&mame->reset_callback_list);
		free_callback_list(&mame->pause_callback_list);
	}

	/* destroy the machine */
	destroy_machine(machine);

	/* return an error */
	return error;
}


/*-------------------------------------------------
    mame_get_phase - return the current program
    phase
-------------------------------------------------*/

int mame_get_phase(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	return mame->current_phase;
}


/*-------------------------------------------------
    add_exit_callback - request a callback on
    termination
-------------------------------------------------*/

void add_exit_callback(running_machine *machine, void (*callback)(running_machine *))
{
	mame_private *mame = machine->mame_data;
	callback_item *cb;

	assert_always(mame_get_phase(machine) == MAME_PHASE_INIT, "Can only call add_exit_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the head of the list */
	cb->func.exit = callback;
	cb->next = mame->exit_callback_list;
	mame->exit_callback_list = cb;
}


/*-------------------------------------------------
    add_reset_callback - request a callback on
    reset
-------------------------------------------------*/

void add_reset_callback(running_machine *machine, void (*callback)(running_machine *))
{
	mame_private *mame = machine->mame_data;
	callback_item *cb, **cur;

	assert_always(mame_get_phase(machine) == MAME_PHASE_INIT, "Can only call add_reset_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.reset = callback;
	cb->next = NULL;
	for (cur = &mame->reset_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    add_pause_callback - request a callback on
    pause
-------------------------------------------------*/

void add_pause_callback(running_machine *machine, void (*callback)(running_machine *, int))
{
	mame_private *mame = machine->mame_data;
	callback_item *cb, **cur;

	assert_always(mame_get_phase(machine) == MAME_PHASE_INIT, "Can only call add_pause_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.pause = callback;
	cb->next = NULL;
	for (cur = &mame->pause_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}



/***************************************************************************
    GLOBAL SYSTEM STATES
***************************************************************************/

/*-------------------------------------------------
    mame_schedule_exit - schedule a clean exit
-------------------------------------------------*/

void mame_schedule_exit(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	mame->exit_pending = TRUE;

	/* if we're autosaving on exit, schedule a save as well */
	if (options.auto_save && (machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_save(machine, machine->gamedrv->name);
}


/*-------------------------------------------------
    mame_schedule_hard_reset - schedule a hard-
    reset of the system
-------------------------------------------------*/

void mame_schedule_hard_reset(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	mame->hard_reset_pending = TRUE;
}


/*-------------------------------------------------
    mame_schedule_soft_reset - schedule a soft-
    reset of the system
-------------------------------------------------*/

void mame_schedule_soft_reset(running_machine *machine)
{
	mame_private *mame = machine->mame_data;

	mame_timer_adjust(mame->soft_reset_timer, time_zero, 0, time_zero);

	/* we can't be paused since the timer needs to fire */
	mame_pause(machine, FALSE);
}


/*-------------------------------------------------
    mame_schedule_save - schedule a save to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_save(running_machine *machine, const char *filename)
{
	mame_private *mame = machine->mame_data;

	/* free any existing request and allocate a copy of the requested name */
	if (mame->saveload_pending_file != NULL)
		free(mame->saveload_pending_file);
	mame->saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	mame->saveload_schedule_callback = handle_save;
	mame->saveload_schedule_time = mame_timer_get_time();

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(machine, FALSE);
}


/*-------------------------------------------------
    mame_schedule_load - schedule a load to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_load(running_machine *machine, const char *filename)
{
	mame_private *mame = machine->mame_data;

	/* free any existing request and allocate a copy of the requested name */
	if (mame->saveload_pending_file != NULL)
		free(mame->saveload_pending_file);
	mame->saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	mame->saveload_schedule_callback = handle_load;
	mame->saveload_schedule_time = mame_timer_get_time();

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(machine, FALSE);
}


/*-------------------------------------------------
    mame_is_scheduled_event_pending - is a
    scheduled event pending?
-------------------------------------------------*/

int mame_is_scheduled_event_pending(running_machine *machine)
{
	/* we can't check for saveload_pending_file here because it will bypass */
	/* required UI screens if a state is queued from the command line */
	mame_private *mame = machine->mame_data;
	return mame->exit_pending || mame->hard_reset_pending;
}


/*-------------------------------------------------
    mame_pause - pause or resume the system
-------------------------------------------------*/

void mame_pause(running_machine *machine, int pause)
{
	mame_private *mame = machine->mame_data;
	callback_item *cb;

	/* ignore if nothing has changed */
	if (mame->paused == pause)
		return;
	mame->paused = pause;

	/* call all registered pause callbacks */
	for (cb = mame->pause_callback_list; cb; cb = cb->next)
		(*cb->func.pause)(machine, mame->paused);
}


/*-------------------------------------------------
    mame_is_paused - the system paused?
-------------------------------------------------*/

int mame_is_paused(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	return (mame->current_phase != MAME_PHASE_RUNNING) || mame->paused;
}



/***************************************************************************
    MEMORY REGIONS
***************************************************************************/

/*-------------------------------------------------
    memory_region_to_index - returns an index
    given either an index or a REGION_* identifier
-------------------------------------------------*/

static int memory_region_to_index(mame_private *mame, int num)
{
	int i;

	/* if we're already an index, stop there */
	if (num < MAX_MEMORY_REGIONS)
		return num;

	/* scan for a match */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		if (mame->mem_region[i].type == num)
			return i;

	return -1;
}


/*-------------------------------------------------
    new_memory_region - allocates memory for a
    region
-------------------------------------------------*/

UINT8 *new_memory_region(running_machine *machine, int type, UINT32 length, UINT32 flags)
{
	mame_private *mame = machine->mame_data;
    int num;

    assert(type >= MAX_MEMORY_REGIONS);

    /* find a free slot */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mame->mem_region[num].base == NULL)
			break;
	if (num < 0)
		fatalerror("Out of memory regions!");

    /* allocate the region */
	mame->mem_region[num].length = length;
	mame->mem_region[num].type = type;
	mame->mem_region[num].flags = flags;
	mame->mem_region[num].base = malloc_or_die(length);
	return mame->mem_region[num].base;
}


/*-------------------------------------------------
    free_memory_region - releases memory for a
    region
-------------------------------------------------*/

void free_memory_region(running_machine *machine, int num)
{
	mame_private *mame = machine->mame_data;

	/* convert to an index and bail if invalid */
	num = memory_region_to_index(mame, num);
	if (num < 0)
		return;

	/* free the region in question */
	free(mame->mem_region[num].base);
	memset(&mame->mem_region[num], 0, sizeof(mame->mem_region[num]));
}


/*-------------------------------------------------
    memory_region - returns pointer to a memory
    region
-------------------------------------------------*/

UINT8 *memory_region(int num)
{
	running_machine *machine = Machine;
	mame_private *mame = machine->mame_data;

	/* convert to an index and return the result */
	num = memory_region_to_index(mame, num);
	return (num >= 0) ? mame->mem_region[num].base : NULL;
}


/*-------------------------------------------------
    memory_region_length - returns length of a
    memory region
-------------------------------------------------*/

UINT32 memory_region_length(int num)
{
	running_machine *machine = Machine;
	mame_private *mame = machine->mame_data;

	/* convert to an index and return the result */
	num = memory_region_to_index(mame, num);
	return (num >= 0) ? mame->mem_region[num].length : 0;
}


/*-------------------------------------------------
    memory_region_type - returns the type of a
    memory region
-------------------------------------------------*/

UINT32 memory_region_type(running_machine *machine, int num)
{
	mame_private *mame = machine->mame_data;

	/* convert to an index and return the result */
	num = memory_region_to_index(mame, num);
	return (num >= 0) ? mame->mem_region[num].type : 0;
}


/*-------------------------------------------------
    memory_region_flags - returns flags for a
    memory region
-------------------------------------------------*/

UINT32 memory_region_flags(running_machine *machine, int num)
{
	mame_private *mame = machine->mame_data;

	/* convert to an index and return the result */
	num = memory_region_to_index(mame, num);
	return (num >= 0) ? mame->mem_region[num].flags : 0;
}



/***************************************************************************
    OUTPUT MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    mame_set_output_channel - configure an output
    channel
-------------------------------------------------*/

void mame_set_output_channel(output_channel channel, output_callback callback, void *param, output_callback *prevcb, void **prevparam)
{
	assert(channel < OUTPUT_CHANNEL_COUNT);
	assert(callback != NULL);

	/* return the originals if requested */
	if (prevcb != NULL)
		*prevcb = output_cb[channel];
	if (prevparam != NULL)
		*prevparam = output_cb_param[channel];

	/* set the new ones */
	output_cb[channel] = callback;
	output_cb_param[channel] = param;
}


/*-------------------------------------------------
    mame_file_output_callback - default callback
    for file output
-------------------------------------------------*/

void mame_file_output_callback(void *param, const char *format, va_list argptr)
{
	vfprintf((FILE *)param, format, argptr);
}


/*-------------------------------------------------
    mame_null_output_callback - default callback
    for no output
-------------------------------------------------*/

void mame_null_output_callback(void *param, const char *format, va_list argptr)
{
}


/*-------------------------------------------------
    mame_printf_error - output an error to the
    appropriate callback
-------------------------------------------------*/

void mame_printf_error(const char *format, ...)
{
	va_list argptr;

	/* by default, we go to stderr */
	if (output_cb[OUTPUT_CHANNEL_ERROR] == NULL)
	{
		output_cb[OUTPUT_CHANNEL_ERROR] = mame_file_output_callback;
		output_cb_param[OUTPUT_CHANNEL_ERROR] = stderr;
	}

	/* do the output */
	va_start(argptr, format);
	(*output_cb[OUTPUT_CHANNEL_ERROR])(output_cb_param[OUTPUT_CHANNEL_ERROR], format, argptr);
	va_end(argptr);
}


/*-------------------------------------------------
    mame_printf_warning - output a warning to the
    appropriate callback
-------------------------------------------------*/

void mame_printf_warning(const char *format, ...)
{
	va_list argptr;

	/* by default, we go to stderr */
	if (output_cb[OUTPUT_CHANNEL_WARNING] == NULL)
	{
		output_cb[OUTPUT_CHANNEL_WARNING] = mame_file_output_callback;
		output_cb_param[OUTPUT_CHANNEL_WARNING] = stderr;
	}

	/* do the output */
	va_start(argptr, format);
	(*output_cb[OUTPUT_CHANNEL_WARNING])(output_cb_param[OUTPUT_CHANNEL_WARNING], format, argptr);
	va_end(argptr);
}


/*-------------------------------------------------
    mame_printf_info - output info text to the
    appropriate callback
-------------------------------------------------*/

void mame_printf_info(const char *format, ...)
{
	va_list argptr;

	/* by default, we go to stdout */
	if (output_cb[OUTPUT_CHANNEL_INFO] == NULL)
	{
		output_cb[OUTPUT_CHANNEL_INFO] = mame_file_output_callback;
		output_cb_param[OUTPUT_CHANNEL_INFO] = stdout;
	}

	/* do the output */
	va_start(argptr, format);
	(*output_cb[OUTPUT_CHANNEL_INFO])(output_cb_param[OUTPUT_CHANNEL_INFO], format, argptr);
	va_end(argptr);
}


/*-------------------------------------------------
    mame_printf_debug - output debug text to the
    appropriate callback
-------------------------------------------------*/

void mame_printf_debug(const char *format, ...)
{
	va_list argptr;

	/* by default, we go to stderr */
	if (output_cb[OUTPUT_CHANNEL_DEBUG] == NULL)
	{
#ifdef MAME_DEBUG
		output_cb[OUTPUT_CHANNEL_DEBUG] = mame_file_output_callback;
		output_cb_param[OUTPUT_CHANNEL_DEBUG] = stdout;
#else
		output_cb[OUTPUT_CHANNEL_DEBUG] = mame_null_output_callback;
		output_cb_param[OUTPUT_CHANNEL_DEBUG] = NULL;
#endif
	}

	/* do the output */
	va_start(argptr, format);
	(*output_cb[OUTPUT_CHANNEL_DEBUG])(output_cb_param[OUTPUT_CHANNEL_DEBUG], format, argptr);
	va_end(argptr);
}


/*-------------------------------------------------
    mame_printf_log - output log text to the
    appropriate callback
-------------------------------------------------*/

void mame_printf_log(const char *format, ...)
{
	va_list argptr;

	/* by default, we go to stderr */
	if (output_cb[OUTPUT_CHANNEL_LOG] == NULL)
	{
		output_cb[OUTPUT_CHANNEL_LOG] = mame_file_output_callback;
		output_cb_param[OUTPUT_CHANNEL_LOG] = stderr;
	}

	/* do the output */
	va_start(argptr, format);
	(*output_cb[OUTPUT_CHANNEL_LOG])(output_cb_param[OUTPUT_CHANNEL_LOG], format, argptr);
	va_end(argptr);
}



/***************************************************************************
    MISCELLANEOUS
***************************************************************************/

/*-------------------------------------------------
    fatalerror - print a message and escape back
    to the OSD layer
-------------------------------------------------*/

DECL_NORETURN static void fatalerror_common(running_machine *machine, int exitcode, const char *buffer) ATTR_NORETURN;

static void fatalerror_common(running_machine *machine, int exitcode, const char *buffer)
{
	/* output and return */
	mame_printf_error("%s\n", giant_string_buffer);

	/* break into the debugger if attached */
	osd_break_into_debugger(giant_string_buffer);

	/* longjmp back if we can; otherwise, exit */
	if (machine != NULL && machine->mame_data != NULL && machine->mame_data->fatal_error_jmpbuf_valid)
  		longjmp(machine->mame_data->fatal_error_jmpbuf, exitcode);
	else
		exit(exitcode);
}


void CLIB_DECL fatalerror(const char *text, ...)
{
	running_machine *machine = Machine;
	va_list arg;

	/* dump to the buffer; assume no one writes >2k lines this way */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, text, arg);
	va_end(arg);

	fatalerror_common(machine, MAMERR_FATALERROR, giant_string_buffer);
}


void CLIB_DECL fatalerror_exitcode(int exitcode, const char *text, ...)
{
	running_machine *machine = Machine;
	va_list arg;

	/* dump to the buffer; assume no one writes >2k lines this way */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, text, arg);
	va_end(arg);

	fatalerror_common(machine, exitcode, giant_string_buffer);
}


/*-------------------------------------------------
    popmessage - pop up a user-visible message
-------------------------------------------------*/

void CLIB_DECL popmessage(const char *text, ...)
{
	extern void CLIB_DECL ui_popup(const char *text, ...) ATTR_PRINTF(1,2);
	va_list arg;

	/* dump to the buffer */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, text, arg);
	va_end(arg);

	/* pop it in the UI */
	ui_popup("%s", giant_string_buffer);
}


/*-------------------------------------------------
    logerror - log to the debugger and any other
    OSD-defined output streams
-------------------------------------------------*/

void CLIB_DECL logerror(const char *text, ...)
{
	running_machine *machine = Machine;

	/* currently, we need a machine to do this */
	if (machine != NULL)
	{
		mame_private *mame = machine->mame_data;
		callback_item *cb;

		/* process only if there is a target */
		if (mame->logerror_callback_list != NULL)
		{
			va_list arg;

			profiler_mark(PROFILER_LOGERROR);

			/* dump to the buffer */
			va_start(arg, text);
			vsnprintf(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, text, arg);
			va_end(arg);

			/* log to all callbacks */
			for (cb = mame->logerror_callback_list; cb; cb = cb->next)
				(*cb->func.log)(machine, giant_string_buffer);

			profiler_mark(PROFILER_END);
		}
	}
}


/*-------------------------------------------------
    add_logerror_callback - adds a callback to be
    called on logerror()
-------------------------------------------------*/

void add_logerror_callback(running_machine *machine, void (*callback)(running_machine *, const char *))
{
	mame_private *mame = machine->mame_data;
	callback_item *cb, **cur;

	assert_always(mame_get_phase(machine) == MAME_PHASE_INIT, "Can only call add_logerror_callback at init time!");

	cb = auto_malloc(sizeof(*cb));
	cb->func.log = callback;
	cb->next = NULL;

	for (cur = &mame->logerror_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    logfile_callback - callback for logging to
    logfile
-------------------------------------------------*/

static void logfile_callback(running_machine *machine, const char *buffer)
{
	if (options.logfile)
		mame_fputs(options.logfile, buffer);
}


/*-------------------------------------------------
    mame_find_cpu_index - return the index of the
    given CPU, or -1 if not found
-------------------------------------------------*/

int mame_find_cpu_index(running_machine *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->drv->cpu[cpunum].tag && strcmp(machine->drv->cpu[cpunum].tag, tag) == 0)
			return cpunum;

	return -1;
}


/*-------------------------------------------------
    mame_rand - standardized random numbers
-------------------------------------------------*/

UINT32 mame_rand(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	mame->rand_seed = 1664525 * mame->rand_seed + 1013904223;
	return mame->rand_seed;
}



/***************************************************************************
    INTERNAL INITIALIZATION LOGIC
***************************************************************************/

/*-------------------------------------------------
    create_machine - create the running machine
    object and initialize it based on options
-------------------------------------------------*/

static running_machine *create_machine(int game)
{
	running_machine *machine;
	int scrnum;

	/* allocate memory for the machine */
	machine = malloc(sizeof(*machine));
	if (machine == NULL)
		goto error;
	memset(machine, 0, sizeof(*machine));

	/* allocate memory for the internal mame_data */
	machine->mame_data = malloc(sizeof(*machine->mame_data));
	if (machine->mame_data == NULL)
		goto error;
	memset(machine->mame_data, 0, sizeof(*machine->mame_data));

	/* initialize the driver-related variables in the machine */
	machine->gamedrv = drivers[game];
	machine->drv = malloc(sizeof(*machine->drv));
	if (machine->drv == NULL)
		goto error;
	machine->basename = mame_strdup(machine->gamedrv->name);
	expand_machine_driver(machine->gamedrv->drv, (machine_config *)machine->drv);

	/* allocate the driver data */
	if (machine->drv->driver_data_size != 0)
	{
		machine->driver_data = malloc(machine->drv->driver_data_size);
		if (machine->driver_data == NULL)
			goto error;
		memset(machine->driver_data, 0, machine->drv->driver_data_size);
	}

	/* configure all screens to be the default */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		machine->screen[scrnum] = machine->drv->screen[scrnum].defstate;

	/* convert some options into live state */
	machine->sample_rate = options.samplerate;
	machine->record_file = options.record;
	machine->playback_file = options.playback;
	machine->debug_mode = options.mame_debug;

	return machine;

error:
	if (machine->driver_data != NULL)
		free(machine->driver_data);
	if (machine->drv != NULL)
		free((machine_config *)machine->drv);
	if (machine->mame_data != NULL)
		free(machine->mame_data);
	if (machine != NULL)
		free(machine);
	return NULL;
}


/*-------------------------------------------------
    destroy_machine - free the machine data
-------------------------------------------------*/

static void destroy_machine(running_machine *machine)
{
	assert(machine == Machine);
	if (machine->driver_data != NULL)
		free(machine->driver_data);
	if (machine->drv != NULL)
		free((machine_config *)machine->drv);
	if (machine->mame_data != NULL)
		free(machine->mame_data);
	if (machine->basename != NULL)
		free((void *)machine->basename);
	free(machine);
	Machine = NULL;
}


/*-------------------------------------------------
    init_machine - initialize the emulated machine
-------------------------------------------------*/

static void init_machine(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	int num;

	/* initialize basic can't-fail systems here */
	cpuintrf_init(machine);
	sndintrf_init(machine);
	fileio_init(machine);
	config_init(machine);
	output_init(machine);
	state_init(machine);
	state_save_allow_registration(TRUE);
	drawgfx_init(machine);
	palette_init(machine);
	render_init(machine);
	ui_init(machine);
	generic_machine_init(machine);
	generic_video_init(machine);
	mame->rand_seed = 0x9d14abd7;

	/* initialize the base time (if not doing record/playback) */
	if (!Machine->record_file && !Machine->playback_file)
		time(&mame->base_time);
	else
		mame->base_time = 0;

	/* init the osd layer */
	if (osd_init(machine) != 0)
		fatalerror("osd_init failed");

	/* initialize the input system */
	/* this must be done before the input ports are initialized */
	if (code_init(machine) != 0)
		fatalerror("code_init failed");

	/* initialize the input ports for the game */
	/* this must be done before memory_init in order to allow specifying */
	/* callbacks based on input port tags */
	if (input_port_init(machine, machine->gamedrv->ipt) != 0)
		fatalerror("input_port_init failed");

	/* load the ROMs if we have some */
	/* this must be done before memory_init in order to allocate memory regions */
	rom_init(machine, machine->gamedrv->rom);

	/* initialize the timers and allocate a soft_reset timer */
	/* this must be done before cpu_init so that CPU's can allocate timers */
	timer_init(machine);
	mame->soft_reset_timer = timer_alloc(soft_reset);

	/* initialize the memory system for this game */
	/* this must be done before cpu_init so that set_context can look up the opcode base */
	if (memory_init(machine) != 0)
		fatalerror("memory_init failed");

	/* now set up all the CPUs */
	if (cpuexec_init(machine) != 0)
		fatalerror("cpuexec_init failed");
	if (cpuint_init(machine) != 0)
		fatalerror("cpuint_init failed");

#ifdef MESS
	/* initialize the devices */
	devices_init(machine);
#endif

	/* start the save/load system */
	saveload_init(machine);

	/* call the game driver's init function */
	/* this is where decryption is done and memory maps are altered */
	/* so this location in the init order is important */
	ui_set_startup_text("Initializing...", TRUE);
	if (machine->gamedrv->driver_init != NULL)
		(*machine->gamedrv->driver_init)(machine);

	/* start the audio system */
	if (sound_init(machine) != 0)
		fatalerror("sound_init failed");

	/* start the video hardware */
	if (video_init(machine) != 0)
		fatalerror("video_init failed");

	/* start the cheat engine */
	if (options.cheat)
		cheat_init(machine);

	/* call the driver's _START callbacks */
	if (machine->drv->machine_start != NULL && (*machine->drv->machine_start)(machine) != 0)
		fatalerror("Unable to start machine emulation");
	if (machine->drv->sound_start != NULL && (*machine->drv->sound_start)(machine) != 0)
		fatalerror("Unable to start sound emulation");
	if (machine->drv->video_start != NULL && (*machine->drv->video_start)(machine) != 0)
		fatalerror("Unable to start video emulation");

	/* free memory regions allocated with REGIONFLAG_DISPOSE (typically gfx roms) */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mame->mem_region[num].flags & ROMREGION_DISPOSE)
			free_memory_region(machine, num);

#ifdef MAME_DEBUG
	/* initialize the debugger */
	if (machine->debug_mode)
		mame_debug_init(machine);
#endif
}


/*-------------------------------------------------
    soft_reset - actually perform a soft-reset
    of the system
-------------------------------------------------*/

static void soft_reset(int param)
{
	running_machine *machine = Machine;
	mame_private *mame = machine->mame_data;
	callback_item *cb;

	logerror("Soft reset\n");

	/* temporarily in the reset phase */
	mame->current_phase = MAME_PHASE_RESET;

	/* a bit gross -- back off of the resource tracking, and put it back at the end */
	assert(get_resource_tag() == 2);
	end_resource_tracking();
	begin_resource_tracking();

	/* allow save state registrations during the reset */
	state_save_allow_registration(TRUE);

	/* unfortunately, we can't rely on callbacks to reset the interrupt */
	/* structures, as these need to happen before we call the reset */
	/* functions registered by the drivers */
	cpuint_reset();

	/* run the driver's reset callbacks */
	if (machine->drv->machine_reset != NULL)
		(*machine->drv->machine_reset)(machine);
	if (machine->drv->sound_reset != NULL)
		(*machine->drv->sound_reset)(machine);
	if (machine->drv->video_reset != NULL)
		(*machine->drv->video_reset)(machine);

	/* call all registered reset callbacks */
	for (cb = machine->mame_data->reset_callback_list; cb; cb = cb->next)
		(*cb->func.reset)(machine);

	/* disallow save state registrations starting here */
	state_save_allow_registration(FALSE);

	/* now we're running */
	mame->current_phase = MAME_PHASE_RUNNING;

	/* set the global time to the current time */
	/* this allows 0-time queued callbacks to run before any CPUs execute */
	mame_timer_set_global_time(mame_timer_get_time());
}


/*-------------------------------------------------
    free_callback_list - free a list of callbacks
-------------------------------------------------*/

static void free_callback_list(callback_item **cb)
{
	while (*cb)
	{
		callback_item *temp = *cb;
		*cb = (*cb)->next;
		free(temp);
	}
}



/***************************************************************************
    SAVE/RESTORE
***************************************************************************/

/*-------------------------------------------------
    saveload_init - initialize the save/load logic
-------------------------------------------------*/

static void saveload_init(running_machine *machine)
{
	/* if we're coming in with a savegame request, process it now */
	if (options.savegame)
	{
		char name[20];

		if (strlen(options.savegame) == 1)
		{
			sprintf(name, "%s-%c", machine->gamedrv->name, options.savegame[0]);
			mame_schedule_load(machine, name);
		}
		else
			mame_schedule_load(machine, options.savegame);
	}

	/* if we're in autosave mode, schedule a load */
	else if (options.auto_save && (machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_load(machine, machine->gamedrv->name);
}


/*-------------------------------------------------
    handle_save - attempt to perform a save
-------------------------------------------------*/

static void handle_save(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	mame_file_error filerr;
	mame_file *file;

	/* if no name, bail */
	if (mame->saveload_pending_file == NULL)
	{
		mame->saveload_schedule_callback = NULL;
		return;
	}

	/* if there are anonymous timers, we can't save just yet */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), mame->saveload_schedule_time).seconds > 0)
		{
			popmessage("Unable to save due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return;
	}

	/* open the file */
	filerr = mame_fopen(SEARCHPATH_STATE, mame->saveload_pending_file, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);
	if (filerr == FILERR_NONE)
	{
		int cpunum;

		/* write the save state */
		if (state_save_save_begin(file) != 0)
		{
			popmessage("Error: Unable to save state due to illegal registrations. See error.log for details.");
			mame_fclose(file);
			goto cancel;
		}

		/* write the default tag */
		state_save_push_tag(0);
		state_save_save_continue();
		state_save_pop_tag();

		/* loop over CPUs */
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			cpuintrf_push_context(cpunum);

			/* make sure banking is set */
			activecpu_reset_banking();

			/* save the CPU data */
			state_save_push_tag(cpunum + 1);
			state_save_save_continue();
			state_save_pop_tag();

			cpuintrf_pop_context();
		}

		/* finish and close */
		state_save_save_finish();
		mame_fclose(file);

		/* pop a warning if the game doesn't support saves */
		if (!(machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
			popmessage("State successfully saved.\nWarning: Save states are not officially supported for this game.");
		else
			popmessage("State successfully saved.");
	}
	else
		popmessage("Error: Failed to save state");

cancel:
	/* unschedule the save */
	free(mame->saveload_pending_file);
	mame->saveload_pending_file = NULL;
	mame->saveload_schedule_callback = NULL;
}


/*-------------------------------------------------
    handle_load - attempt to perform a load
-------------------------------------------------*/

static void handle_load(running_machine *machine)
{
	mame_private *mame = machine->mame_data;
	mame_file_error filerr;
	mame_file *file;

	/* if no name, bail */
	if (mame->saveload_pending_file == NULL)
	{
		mame->saveload_schedule_callback = NULL;
		return;
	}

	/* if there are anonymous timers, we can't load just yet because the timers might */
	/* overwrite data we have loaded */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), mame->saveload_schedule_time).seconds > 0)
		{
			popmessage("Unable to load due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return;
	}

	/* open the file */
	filerr = mame_fopen(SEARCHPATH_STATE, mame->saveload_pending_file, OPEN_FLAG_READ, &file);
	if (filerr == FILERR_NONE)
	{
		/* start loading */
		if (state_save_load_begin(file) == 0)
		{
			int cpunum;

			/* read tag 0 */
			state_save_push_tag(0);
			state_save_load_continue();
			state_save_pop_tag();

			/* loop over CPUs */
			for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
			{
				cpuintrf_push_context(cpunum);

				/* make sure banking is set */
				activecpu_reset_banking();

				/* load the CPU data */
				state_save_push_tag(cpunum + 1);
				state_save_load_continue();
				state_save_pop_tag();

				/* make sure banking is set */
				activecpu_reset_banking();

				cpuintrf_pop_context();
			}

			/* finish and close */
			state_save_load_finish();
			popmessage("State successfully loaded.");
		}
		else
			popmessage("Error: Failed to load state");
		mame_fclose(file);
	}
	else
		popmessage("Error: Failed to load state");

cancel:
	/* unschedule the load */
	free(mame->saveload_pending_file);
	mame->saveload_pending_file = NULL;
	mame->saveload_schedule_callback = NULL;
}



/***************************************************************************
    SYSTEM TIME
***************************************************************************/

/*-------------------------------------------------
    get_tm_time - converts a MAME
-------------------------------------------------*/

static void get_tm_time(struct tm *t, mame_system_tm *systm)
{
	systm->second	= t->tm_sec;
	systm->minute	= t->tm_min;
	systm->hour		= t->tm_hour;
	systm->mday		= t->tm_mday;
	systm->month	= t->tm_mon;
	systm->year		= t->tm_year + 1900;
	systm->weekday	= t->tm_wday;
	systm->day		= t->tm_yday;
	systm->is_dst	= t->tm_isdst;
}



/*-------------------------------------------------
    fill_systime - fills out a mame_system_time
    structure
-------------------------------------------------*/

static void fill_systime(mame_system_time *systime, time_t t)
{
	systime->time = t;
	get_tm_time(localtime(&t), &systime->local_time);
	get_tm_time(gmtime(&t), &systime->utc_time);
}



/*-------------------------------------------------
    mame_get_base_datetime - retrieve the time of
    the host system; useful for RTC implementations
-------------------------------------------------*/

void mame_get_base_datetime(running_machine *machine, mame_system_time *systime)
{
	mame_private *mame = machine->mame_data;
	fill_systime(systime, mame->base_time);
}



/*-------------------------------------------------
    mame_get_current_datetime - retrieve the current
    time (offsetted by the baes); useful for RTC
    implementations
-------------------------------------------------*/

void mame_get_current_datetime(running_machine *machine, mame_system_time *systime)
{
	mame_private *mame = machine->mame_data;
	fill_systime(systime, mame->base_time + mame_timer_get_time().seconds);
}
