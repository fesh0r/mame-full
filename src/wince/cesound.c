/***************************************************************************

    M.A.M.E.CE3  -  Multiple Arcade Machine Emulator for Pocket PC
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAMECE3, and may only be used, modified and
    distributed under the terms of the MAME license, in "MAME.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  CESound.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "mame.h"
#include "driver.h"
#include "rc.h"

/***************************************************************************
    External variables
 ***************************************************************************/

void osd_sound_enable(int enable)
{
}

void osd_opl_control(int chip,int reg)
{
    // removed no OPL in CE
}

void osd_opl_write(int chip,int data)
{
    // removed no OPL in CE
}

// sound options (none at this time)
struct rc_option sound_opts[] = 
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

int	attenuation = 0;

/***************************************************************************
    External variables
 ***************************************************************************/

/* global sample tracking */
static double samples_per_frame;
static double samples_left_over;
static UINT32 samples_this_frame;

/***************************************************************************
    Internal structures
 ***************************************************************************/

#define NUM_WAVEHDRS 2
//#define FPS			60

struct tSound_private
{
//	WAVEOUTCAPS		m_Caps;
	HWAVEOUT		m_hWaveOut;
	WAVEHDR			m_WaveHdrs[NUM_WAVEHDRS];

	int				m_nVolume;		// -32 to 0 attenuation value
	int				m_nChannels;
	int				m_nSampleRate;
	int				m_nSampleBits;

    unsigned int    m_nSamplesPerFrame;
    unsigned int    m_nBytesPerFrame;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tSound_private      This;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static int CESound_init(void)
{

	WAVEFORMATEX wf;
	
	This.m_nVolume = 0;
	This.m_hWaveOut = NULL;
	This.m_nSampleRate = Machine->sample_rate; 
	This.m_nSampleBits = 16;       
	This.m_nChannels = 1;
	
	memset(&This.m_WaveHdrs, 0, sizeof(WAVEHDR) * NUM_WAVEHDRS); // set WaveHdrs Buffers to 0s
	
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = This.m_nChannels; 
	wf.nSamplesPerSec = This.m_nSampleRate; 
	wf.nBlockAlign = This.m_nChannels * This.m_nSampleBits / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.wBitsPerSample = This.m_nSampleBits; 
	wf.cbSize = 0;

	if ( waveOutOpen(
		&This.m_hWaveOut,	// Handle
		0,					// ID (0 for wave mapper)
		&wf,				// Wave format
		0,					// Callback
		0,					// Instance data
		CALLBACK_NULL) != MMSYSERR_NOERROR)
	{
		This.m_hWaveOut = NULL;
		return 1; //Sound Init Failed - Can't open Wave Device
	}
	return 0;  //Sound Init successful
}


static void CESound_exit(void)
{
	int i, ticks;
	
	waveOutClose(This.m_hWaveOut);
	
	ticks = GetTickCount();
	while (GetTickCount() - ticks < 500)
	{ 
		; //Loop to let sound buffers time to finish
	}

	for (i = 0; i < NUM_WAVEHDRS; i++)
	{
		waveOutUnprepareHeader(This.m_hWaveOut, &This.m_WaveHdrs[i], sizeof(WAVEHDR));
	}
	This.m_hWaveOut=NULL;
}


int osd_start_audio_stream(int stereo)
{
	int i; // count the WAVEHDRS
    int buflen;

	// skip if sound disabled
	if (Machine->sample_rate != 0)
	{
		if (CESound_init())
			return 1;
	}

    if (stereo)
        stereo = 1;	/* make sure it's either 0 or 1 */

	// determine the number of samples and bytes per frame //
	This.m_nSamplesPerFrame = (double)Machine->sample_rate /(double)Machine->drv->frames_per_second;
	This.m_nBytesPerFrame   = This.m_nSamplesPerFrame * sizeof(INT16) * (stereo + 1);//*2;

	// compute how many samples to generate this frame
	samples_left_over = This.m_nSamplesPerFrame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	buflen = This.m_nBytesPerFrame;

    // set up the buffer on each wave header
	if (Machine->sample_rate != 0)
	{
		for (i = 0; i < NUM_WAVEHDRS; i++)
		{

			This.m_WaveHdrs[i].lpData			= (LPSTR) auto_malloc(buflen);
			This.m_WaveHdrs[i].dwBufferLength	= buflen;
			This.m_WaveHdrs[i].dwBytesRecorded	= 0;
			This.m_WaveHdrs[i].dwUser			= 0;
			This.m_WaveHdrs[i].dwFlags			= 0;
			This.m_WaveHdrs[i].dwLoops			= 0;
			This.m_WaveHdrs[i].lpNext			= NULL;
			This.m_WaveHdrs[i].reserved			= 0;
			
			waveOutPrepareHeader(This.m_hWaveOut, &This.m_WaveHdrs[i], sizeof(WAVEHDR));
		}
	}
	return samples_this_frame;
}


int osd_update_audio_stream(INT16* buffer)
{
	int i = 0;
	short *s;
	short *d;
	
	if (Machine->sample_rate != 0)
	{
		s = (short *)buffer;
		for (i = 0; i < NUM_WAVEHDRS; i++)
		{		
			d = (short *)This.m_WaveHdrs[i].lpData;
			memcpy(d,s,This.m_nBytesPerFrame);
			waveOutWrite(This.m_hWaveOut, &This.m_WaveHdrs[i], sizeof(WAVEHDR));
		}
	}

	samples_left_over += This.m_nSamplesPerFrame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	return samples_this_frame;
}


void osd_stop_audio_stream(void)
{
	waveOutReset (This.m_hWaveOut);
	CESound_exit();
}


void osd_set_mastervolume(int volume)
{
	This.m_nVolume = volume;
}


int osd_get_mastervolume(void)
{
    return This.m_nVolume;
}
