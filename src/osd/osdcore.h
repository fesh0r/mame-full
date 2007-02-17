/***************************************************************************

    osdcore.h

    Core OS-dependent code interface.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    The prototypes in this file describe the interfaces that the MAME core
    and various tools rely upon to interact with the outside world. They are
    broken out into several categories.

***************************************************************************/

#pragma once

#ifndef __OSDCORE_H__
#define __OSDCORE_H__

#include "osdcomm.h"



/***************************************************************************
    FILE I/O INTERFACES
***************************************************************************/

/* flags controlling file access */
#define OPEN_FLAG_READ			0x0001		/* open for read */
#define OPEN_FLAG_WRITE			0x0002		/* open for write */
#define OPEN_FLAG_CREATE		0x0004		/* create & truncate file */
#define OPEN_FLAG_CREATE_PATHS	0x0008		/* create paths as necessary */

/* error codes returned by routines below */
enum _mame_file_error
{
	FILERR_NONE,
	FILERR_FAILURE,
	FILERR_OUT_OF_MEMORY,
	FILERR_NOT_FOUND,
	FILERR_ACCESS_DENIED,
	FILERR_ALREADY_OPEN,
	FILERR_TOO_MANY_FILES,
	FILERR_INVALID_DATA,
	FILERR_INVALID_ACCESS
};
typedef enum _mame_file_error mame_file_error;

/* osd_file is an opaque type which represents an open file */
typedef struct _osd_file osd_file;


/*-----------------------------------------------------------------------------
    osd_open: open a new file.

    Parameters:

        path - path to the file to open

        openflags - some combination of:

            OPEN_FLAG_READ - open the file for read access
            OPEN_FLAG_WRITE - open the file for write access
            OPEN_FLAG_CREATE - create/truncate the file when opening
            OPEN_FLAG_CREATE_PATHS - specifies that non-existant paths
                    should be created if necessary

        file - pointer to an osd_file * to receive the newly-opened file
            handle; this is only valid if the function returns FILERR_NONE

        filesize - pointer to a UINT64 to receive the size of the opened
            file; this is only valid if the function returns FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while opening
        the file, or FILERR_NONE if no error occurred

    Notes:

        This function is called by mame_fopen and several other places in
        the core to access files. These functions will construct paths by
        concatenating various search paths held in the options.c options
        database with partial paths specified by the core. The core assumes
        that the path separator is the first character of the string
        PATH_SEPARATOR, but does not interpret any path separators in the
        search paths, so if you use a different path separator in a search
        path, you may get a mixture of PATH_SEPARATORs (from the core) and
        alternate path separators (specified by users and placed into the
        options database).
-----------------------------------------------------------------------------*/
mame_file_error osd_open(const char *path, UINT32 openflags, osd_file **file, UINT64 *filesize);


/*-----------------------------------------------------------------------------
    osd_close: close an open file

    Parameters:

        file - handle to a file previously opened via osd_open

    Return value:

        a mame_file_error describing any error that occurred while closing
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_close(osd_file *file);


/*-----------------------------------------------------------------------------
    osd_read: read from an open file

    Parameters:

        file - handle to a file previously opened via osd_open

        buffer - pointer to memory that will receive the data read

        offset - offset within the file to read from

        length - number of bytes to read from the file

        actual - pointer to a UINT32 to receive the number of bytes actually
            read during the operation; valid only if the function returns
            FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while reading
        from the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_read(osd_file *file, void *buffer, UINT64 offset, UINT32 length, UINT32 *actual);


/*-----------------------------------------------------------------------------
    osd_write: write to an open file

    Parameters:

        file - handle to a file previously opened via osd_open

        buffer - pointer to memory that contains the data to write

        offset - offset within the file to write to

        length - number of bytes to write to the file

        actual - pointer to a UINT32 to receive the number of bytes actually
            written during the operation; valid only if the function returns
            FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while writing to
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_write(osd_file *file, const void *buffer, UINT64 offset, UINT32 length, UINT32 *actual);


/*-----------------------------------------------------------------------------
    osd_rmfile: deletes a file

    Parameters:

        filename - path to file to delete

    Return value:

        a mame_file_error describing any error that occurred while deleting
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_rmfile(const char *filename);


/*-----------------------------------------------------------------------------
    osd_get_physical_drive_geometry: if the given path points to a physical
        drive, return the geometry of that drive

    Parameters:

        filename - pointer to a path which might describe a physical drive

        cylinders - pointer to a UINT32 to receive the number of cylinders
            of the physical drive

        heads - pointer to a UINT32 to receive the number of heads per
            cylinder of the physical drive

        sectors - pointer to a UINT32 to receive the number of sectors per
            cylinder of the physical drive

        bps - pointer to a UINT32 to receive the number of bytes per sector
            of the physical drive

    Return value:

        TRUE if the filename points to a physical drive and if the values
        pointed to by cylinders, heads, sectors, and bps are valid; FALSE in
        any other case
-----------------------------------------------------------------------------*/
int osd_get_physical_drive_geometry(const char *filename, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps);


/*-----------------------------------------------------------------------------
    osd_uchar_from_osdchar: convert the given character or sequence of
        characters from the OS-default encoding to a Unicode character

    Parameters:

        uchar - pointer to a UINT32 to receive the resulting unicode
            character

        osdchar - pointer to one or more chars that are in the OS-default
            encoding

        count - number of characters provided in the OS-default encoding

    Return value:

        The number of characters required to form a Unicode character.
-----------------------------------------------------------------------------*/
int osd_uchar_from_osdchar(UINT32 /* unicode_char */ *uchar, const char *osdchar, size_t count);



/***************************************************************************
    DIRECTORY INTERFACES
***************************************************************************/

/* types of directory entries that can be returned */
enum _osd_dir_entry_type
{
	ENTTYPE_NONE,
	ENTTYPE_FILE,
	ENTTYPE_DIR,
	ENTTYPE_OTHER
};
typedef enum _osd_dir_entry_type osd_dir_entry_type;

/* osd_directory is an opaque type which represents an open directory */
typedef struct _osd_directory osd_directory;

/* osd_directory_entry contains basic information about a file when iterating through */
/* a directory */
typedef struct _osd_directory_entry osd_directory_entry;
struct _osd_directory_entry
{
	const char *		name;			/* name of the entry */
	osd_dir_entry_type 	type;			/* type of the entry */
	UINT64				size;			/* size of the entry */
};


/*-----------------------------------------------------------------------------
    osd_opendir: open a directory for iteration

    Parameters:

        dirname - path to the directory in question

    Return value:

        upon success, this function should return an osd_directory pointer
        which contains opaque data necessary to traverse the directory; on
        failure, this function should return NULL
-----------------------------------------------------------------------------*/
osd_directory *osd_opendir(const char *dirname);


/*-----------------------------------------------------------------------------
    osd_readdir: return information about the next entry in the directory

    Parameters:

        dir - pointer to an osd_directory that was returned from a prior
            call to osd_opendir

    Return value:

        a constant pointer to an osd_directory_entry representing the current item
        in the directory, or NULL, indicating that no more entries are
        present
-----------------------------------------------------------------------------*/
const osd_directory_entry *osd_readdir(osd_directory *dir);


/*-----------------------------------------------------------------------------
    osd_closedir: close an open directory for iteration

    Parameters:

        dir - pointer to an osd_directory that was returned from a prior
            call to osd_opendir

    Return value:

        frees any allocated memory and resources associated with the open
        directory
-----------------------------------------------------------------------------*/
void osd_closedir(osd_directory *dir);


/*-----------------------------------------------------------------------------
    osd_is_absolute_path: returns whether the specified path is absolute

    Parameters:

        path - the path in question

    Return value:

        non-zero if the path is absolute, zero otherwise

-----------------------------------------------------------------------------*/
int osd_is_absolute_path(const char *path);



/***************************************************************************
    TIMING INTERFACES
***************************************************************************/

/* a osd_ticks_t is a 64-bit integer that is used as a core type in timing interfaces */
typedef INT64 osd_ticks_t;


/*-----------------------------------------------------------------------------
    osd_ticks: return the current running tick counter

    Parameters:

        None

    Return value:

        an osd_ticks_t value which represents the current tick counter

    Notes:

        The resolution of this counter should be 1ms or better for accurate
        performance. It is also important that the source of this timer be
        accurate. It is ok if this call is not ultra-fast, since it is
        primarily used for once/frame synchronization.
-----------------------------------------------------------------------------*/
osd_ticks_t osd_ticks(void);


/*-----------------------------------------------------------------------------
    osd_ticks_per_second: return the number of ticks per second

    Parameters:

        None

    Return value:

        an osd_ticks_t value which represents the number of ticks per
        second
-----------------------------------------------------------------------------*/
osd_ticks_t osd_ticks_per_second(void);


/*-----------------------------------------------------------------------------
    osd_profiling_ticks: return the current running "profiling" tick counter

    Parameters:

        None

    Return value:

        an osd_ticks_t value which represents the current "profiling" tick
        counter

    Notes:

        The profiling tick counter may or may not be different from the
        regular tick counter. However, the profiling counter has differing
        requirements. First, it must be as fast as possible, so as not to
        perturb profiling measurements in a significant way. Second, it
        should be a high resolution as possible to provide accurate short-
        term measurements (1us resolution or better is ideal). Third, it
        is not necessary to calibrate the timing (hence the lack of an
        osd_profiling_ticks_per_second call).

        On x86 system, this generally maps to an RDTSC instruction.
-----------------------------------------------------------------------------*/
osd_ticks_t osd_profiling_ticks(void);



/***************************************************************************
    SYNCHRONIZATION INTERFACES
***************************************************************************/

/* osd_lock is an opaque type which represents a recursive lock/mutex */
typedef struct _osd_lock osd_lock;


/*-----------------------------------------------------------------------------
    osd_lock_alloc: allocate a new lock

    Parameters:

        None.

    Return value:

        A pointer to the allocated lock.
-----------------------------------------------------------------------------*/
osd_lock *osd_lock_alloc(void);


/*-----------------------------------------------------------------------------
    osd_lock_acquire: acquire a lock, blocking until it can be acquired

    Parameters:

        lock - a pointer to a previously allocated osd_lock.

    Return value:

        None.

    Notes:

        osd_locks are defined to be recursive. If the current thread already
        owns the lock, this function should return immediately.
-----------------------------------------------------------------------------*/
void osd_lock_acquire(osd_lock *lock);


/*-----------------------------------------------------------------------------
    osd_lock_try: attempt to acquire a lock

    Parameters:

        lock - a pointer to a previously allocated osd_lock.

    Return value:

        TRUE if the lock was available and was acquired successfully.
        FALSE if the lock was already in used by another thread.
-----------------------------------------------------------------------------*/
int osd_lock_try(osd_lock *lock);


/*-----------------------------------------------------------------------------
    osd_lock_release: release control of a lock that has been acquired

    Parameters:

        lock - a pointer to a previously allocated osd_lock.

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_lock_release(osd_lock *lock);


/*-----------------------------------------------------------------------------
    osd_lock_free: free the memory and resources associated with an osd_lock

    Parameters:

        lock - a pointer to a previously allocated osd_lock.

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_lock_free(osd_lock *lock);



/***************************************************************************
    WORK ITEM INTERFACES
***************************************************************************/

/* these flags can be set when creating a queue to give hints to the code about
   how to configure the queue */
#define WORK_QUEUE_FLAG_IO			0x0001
#define WORK_QUEUE_FLAG_MULTI		0x0002

/* osd_work_queue is an opaque type which represents a queue of work items */
typedef struct _osd_work_queue osd_work_queue;

/* osd_work_item is an opaque type which represents a single work item */
typedef struct _osd_work_item osd_work_item;

/* osd_work_callback is a callback function that does work */
typedef void *(*osd_work_callback)(void *param);


/*-----------------------------------------------------------------------------
    osd_work_queue_alloc: create a new work queue

    Parameters:

        flags - one or more of the WORK_QUEUE_FLAG_* values ORed together:

            WORK_QUEUE_FLAG_IO - indicates that the work queue will do some
                I/O; this may be a useful hint so that threads are created
                even on single-processor systems since I/O can often be
                overlapped with other work

            WORK_QUEUE_FLAG_MULTI - indicates that the work queue should
                take advantage of as many processors as it can; items queued
                here are assumed to be independent

    Return value:

        A pointer to an allocated osd_work_queue object.

    Notes:

        A work queue abstracts the notion of how potentially threaded work
        can be performed. If no threading support is available, it is a
        simple matter to execute the work items as they are queued.
-----------------------------------------------------------------------------*/
osd_work_queue *osd_work_queue_alloc(int flags);


/*-----------------------------------------------------------------------------
    osd_work_queue_items: return the number of pending items in the queue

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

    Return value:

        The number of incomplete items remaining in the queue.
-----------------------------------------------------------------------------*/
int osd_work_queue_items(osd_work_queue *queue);


/*-----------------------------------------------------------------------------
    osd_work_queue_wait: wait for the queue to be empty

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

        timeout - a timeout value in osd_ticks_per_second()

    Return value:

        TRUE if the queue is empty; FALSE if the wait timed out before the
        queue was emptied.
-----------------------------------------------------------------------------*/
int osd_work_queue_wait(osd_work_queue *queue, osd_ticks_t timeout);


/*-----------------------------------------------------------------------------
    osd_work_queue_free: free a work queue, waiting for all items to complete

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

    Return value:

        None.
-----------------------------------------------------------------------------*/
void osd_work_queue_free(osd_work_queue *queue);


/*-----------------------------------------------------------------------------
    osd_work_item_queue: queue a new work item

    Parameters:

        queue - pointer to an osd_work_queue that was previously created via
            osd_work_queue_alloc

        callback - pointer to a function that will do the work

        param - a void * parameter that can be used to pass data to the
            function

    Return value:

        A pointer to an allocated osd_work_item representing the item.

    Notes:

        On single-threaded systems, this function may actually execute the
        work item immediately before returning.
-----------------------------------------------------------------------------*/
osd_work_item *osd_work_item_queue(osd_work_queue *queue, osd_work_callback callback, void *param);


/*-----------------------------------------------------------------------------
    osd_work_item_wait: wait for a work item to complete

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

        timeout - a timeout value in osd_ticks_per_second()

    Return value:

        TRUE if the item completed; FALSE if the wait timed out before the
        item completed.
-----------------------------------------------------------------------------*/
int osd_work_item_wait(osd_work_item *item, osd_ticks_t timeout);


/*-----------------------------------------------------------------------------
    osd_work_item_result: get the result of a work item

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

    Return value:

        A void * that represents the work item's result.
-----------------------------------------------------------------------------*/
void *osd_work_item_result(osd_work_item *item);


/*-----------------------------------------------------------------------------
    osd_work_item_release: release the memory allocated to a work item

    Parameters:

        item - pointer to an osd_work_item that was previously returned from
            osd_work_item_queue

    Return value:

        None.

    Notes:

        The osd_work_item exists until explicitly released, even if it has
        long since completed. It is the queuer's responsibility to release
        any work items it has queued.
-----------------------------------------------------------------------------*/
void osd_work_item_release(osd_work_item *item);



/***************************************************************************
    MISCELLANEOUS INTERFACES
***************************************************************************/

/*-----------------------------------------------------------------------------
    osd_alloc_executable: allocate memory that can contain executable code

    Parameters:

        size - the number of bytes to allocate

    Return value:

        a pointer to the allocated memory

    Notes:

        On many systems, this call may acceptably map to malloc(). On systems
        where pages are tagged with "no execute" privileges, it may be
        necessary to perform some kind of special allocation to ensure that
        code placed into this buffer can be executed.
-----------------------------------------------------------------------------*/
void *osd_alloc_executable(size_t size);


/*-----------------------------------------------------------------------------
    osd_free_executable: free memory allocated by osd_alloc_executable

    Parameters:

        ptr - the pointer returned from osd_alloc_executable

        size - the number of bytes originally requested

    Return value:

        None
-----------------------------------------------------------------------------*/
void osd_free_executable(void *ptr, size_t size);


/*-----------------------------------------------------------------------------
    osd_is_bad_read_ptr: attempt to determine if the given pointer will
        generate an access violation if accessed for read

    Parameters:

        ptr - the pointer to examine

        size - the number of bytes to reference

    Return value:

        TRUE if an access to the referenced memory will generate an access
        violation on a read; FALSE otherwise.

    Notes:

        This function will eventually be deprecated.
-----------------------------------------------------------------------------*/
int osd_is_bad_read_ptr(const void *ptr, size_t size);


/*-----------------------------------------------------------------------------
    osd_break_into_debugger: break into the hosting system's debugger if one
        is attached

    Parameters:

        message - pointer to string to output to the debugger

    Return value:

        None.

    Notes:

        This function is called when an assertion or other important error
        occurs. If a debugger is attached to the current process, it should
        break into the debugger and display the given message.
-----------------------------------------------------------------------------*/
void osd_break_into_debugger(const char *message);



#endif	/* __OSDEPEND_H__ */
