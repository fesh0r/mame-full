/*
 * Implementation of the standard functions in direct.h
 *
 * opendir(), readdir(), closedir() and rewinddir().
 *
 * 06/17/2000 by Mike Haaland <mhaaland@hypertech.com>
 * 08/10/2001 by Nathan Woods <npwoods@bigfoot.com>
 */

#include "dirent.h"

/** open the current directory and return a structure
 *  to be used in subsequent readdir() and closedir()
 *  calls. 
 *
 *  returns NULL if one error. 
 */
DIR *opendir(const char *dirname)
{
    
    DIR *dir;

	dir = malloc(sizeof(DIR));
	if (!dir)
		return NULL;

    /* Stash the directory name */
#if UNICODE
	_snwprintf(dir->pathName, sizeof(dir->pathName) / sizeof(dir->pathName), L"%S\\*.*", dirname);
#else
	_snprintf(dir->pathName, sizeof(dir->pathName) / sizeof(dir->pathName), "%s\\*.*", dirname);
#endif

    /* set the handle to invalid and set the firstTime flag */
    dir->handle    = INVALID_HANDLE_VALUE;
    dir->firstTime = TRUE;
	return dir;
}

static int resetdir(DIR *dirp)
{
    /* reset ourselves to the first file in the directory
     *
     * We just close the current handle and reset for the
     * next readdir call
     */
    int result = 1;
    
    if (dirp->handle != INVALID_HANDLE_VALUE)
    {
        result = FindClose(dirp->handle);
        dirp->handle = INVALID_HANDLE_VALUE;
    }
	return result;
}

/** Close the current directory - return 0 if success */
int	closedir(DIR *dirp)
{
	int result;
	result = resetdir(dirp);
	free(dirp);
	return result;
}

/** get the next entry in the directory */
struct dirent *	readdir(DIR *dirp)
{
    static struct dirent d;

    if (dirp->firstTime)
    {
        /** Get the first entry in the directory */
        dirp->handle = FindFirstFile(dirp->pathName, &dirp->findFileData);
        dirp->firstTime = FALSE;
        if (dirp->handle == INVALID_HANDLE_VALUE)
			return NULL;
    }
    else
    {
        int result;
		result = FindNextFile(dirp->handle, &dirp->findFileData);
		if (!result)
			return NULL;
    }
    /* we have a valid FIND_FILE_DATA, copy the filename */
    memset(&d,'\0', sizeof(struct dirent));

#ifdef UNICODE
	_snprintf(d.d_name, sizeof(d.d_name) / sizeof(d.d_name[0]), "%S", dirp->findFileData.cFileName);
#else
    strcpy(d.d_name,dirp->findFileData.cFileName);
#endif
    d.d_namlen = strlen(d.d_name);  

    return &d;
}

/** reopen the current directory */
void rewinddir(DIR *dirp)
{
     resetdir(dirp);
     dirp->firstTime = TRUE;
}
