#define USE_SEAL
//#define USE_ALLEGRO

#include "driver.h"
#include "mamalleg.h"
#include <dos.h>
#include <conio.h>

#ifdef USE_ALLEGRO
/* cut down Allegro size */
BEGIN_DIGI_DRIVER_LIST
   DIGI_DRIVER_SOUNDSCAPE
   DIGI_DRIVER_AUDIODRIVE
   DIGI_DRIVER_WINSOUNDSYS
   DIGI_DRIVER_SB
END_DIGI_DRIVER_LIST

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

SAMPLE *mysample;
int myvoice;
#endif

#ifdef USE_SEAL
#include <audio.h>
/* audio related stuff */
#define SOUND_CHANNELS 2	/* left and right */
HAC hVoice[SOUND_CHANNELS];
LPAUDIOWAVE lpWave[SOUND_CHANNELS];
AUDIOINFO info;
AUDIOCAPS caps;
#endif

int nominal_sample_rate;
int soundcard,usestereo;
int attenuation = 0;
int sampleratedetect = 1;
static int master_volume = 256;

static int stream_playing;
static INT16 *stream_cache_data;
static int stream_cache_len;
static int stream_cache_stereo;

#define NUM_BUFFERS 3	/* raising this number should improve performance with frameskip, */
						/* but also increases the latency. */

static int voice_pos;
static int audio_buffer_length;

/* global sample tracking */
static double samples_per_frame;
static double samples_left_over;
static UINT32 samples_this_frame;

int msdos_init_seal (void)
{
#ifdef USE_SEAL
	if (AInitialize() == AUDIO_ERROR_NONE)
	{
		return 0;
	}
	else
	{
		return 1;
	}
#else
	return 0;
#endif
}


int msdos_init_sound(void)
{
#ifdef USE_SEAL
	int i;

	/* Ask the user if no soundcard was chosen */
	if (soundcard == -1)
	{
		unsigned int k;

		printf("\nSelect the audio device:\n");

		for (k = 0;k < AGetAudioNumDevs();k++)
		{
			/* don't show the AWE32, it's too slow, users must choose Sound Blaster */
			if (AGetAudioDevCaps(k,&caps) == AUDIO_ERROR_NONE &&
					strcmp(caps.szProductName,"Sound Blaster AWE32"))
			{
				printf("  %2d. %s\n",k,caps.szProductName);
			}
		}
		printf("\n");

		if (k < 10)
		{
			i = getch();
			soundcard = i - '0';
		}
		else
		{
			scanf("%d",&soundcard);
		}
	}

	stream_playing = 0;
	stream_cache_data = 0;
	stream_cache_len = 0;
	stream_cache_stereo = 0;

	/* initialize SEAL audio library */
	if( soundcard == 0 || Machine->sample_rate == 0 )
	{
		return 0;
	}

	/* open audio device */
	/*                              info.nDeviceId = AUDIO_DEVICE_MAPPER;*/
	info.nDeviceId = soundcard;
	/* always use 16 bit mixing if possible - better quality and same speed of 8 bit */
	info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_RAW_SAMPLE;

	/* use stereo output if supported */
	if (usestereo)
	{
		if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
			info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO | AUDIO_FORMAT_RAW_SAMPLE;
	}

	info.nSampleRate = Machine->sample_rate;
	if (AOpenAudio(&info) != AUDIO_ERROR_NONE)
	{
		printf("audio initialization failed\n");
		return 1;
	}

	AGetAudioDevCaps(info.nDeviceId,&caps);
	logerror("Using %s at %d-bit %s %u Hz\n",
			caps.szProductName,
			info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
			info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
			info.nSampleRate);

	/* open and allocate voices, allocate waveforms */
	if (AOpenVoices(SOUND_CHANNELS) != AUDIO_ERROR_NONE)
	{
		printf("voices initialization failed\n");
		return 1;
	}

	for (i = 0; i < SOUND_CHANNELS; i++)
	{
		lpWave[i] = 0;
	}

	/* update the Machine structure to reflect the actual sample rate */
	Machine->sample_rate = info.nSampleRate;

	logerror("set sample rate: %d\n",Machine->sample_rate);
	if (sampleratedetect)
	{
		cycles_t a,b;
		cycles_t cps = osd_cycles_per_second();
		LONG start,end;

		if (ACreateAudioVoice(&hVoice[0]) != AUDIO_ERROR_NONE)
			return 1;

		if ((lpWave[0] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
		{
			ADestroyAudioVoice(hVoice[0]);
			return 1;
		}

		lpWave[0]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
		lpWave[0]->nSampleRate = Machine->sample_rate;
		lpWave[0]->dwLength = 3*Machine->sample_rate;
		lpWave[0]->dwLoopStart = 0;
		lpWave[0]->dwLoopEnd = 3*Machine->sample_rate;
		if (ACreateAudioData(lpWave[0]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[0]);
			lpWave[0] = 0;

			return 1;
		}

		memset(lpWave[0]->lpData,0,3*Machine->sample_rate);
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[0],0,3*Machine->sample_rate);
		APrimeVoice(hVoice[0],lpWave[0]);
		ASetVoiceFrequency(hVoice[0],Machine->sample_rate);
		ASetVoiceVolume(hVoice[0],0);
		AStartVoice(hVoice[0]);

		a = osd_cycles();
		/* wait some time to let everything stabilize */
		do
		{
			AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
			b = osd_cycles();
		} while (b-a < cps/10);

		a = osd_cycles();
		AGetVoicePosition(hVoice[0],&start);
		do
		{
			AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
			b = osd_cycles();
		} while (b-a < cps);
		AGetVoicePosition(hVoice[0],&end);
		nominal_sample_rate = Machine->sample_rate;
		Machine->sample_rate = end - start;
		logerror("actual sample rate: %d\n",Machine->sample_rate);

		AStopVoice(hVoice[0]);
		ADestroyAudioData(lpWave[0]);
		free(lpWave[0]);
		lpWave[0] = 0;
		ADestroyAudioVoice(hVoice[0]);
	}
	else
	{
		nominal_sample_rate = Machine->sample_rate;
	}
#endif

#ifdef USE_ALLEGRO
	reserve_voices(1,0);
	if (install_sound(DIGI_AUTODETECT,MIDI_NONE,0) != 0)
	{
		logerror("Allegro install_sound error: %s\n",allegro_error);
		return 1;
	}

	nominal_sample_rate = Machine->sample_rate;
#endif

	osd_set_mastervolume(attenuation);	/* set the startup volume */

	return 0;
}


void msdos_shutdown_sound(void)
{
	if( soundcard == 0 || Machine->sample_rate == 0 )
	{
		return;
	}
#ifdef USE_SEAL
	ACloseVoices();
	ACloseAudio();
#endif
}


int osd_start_audio_stream(int stereo)
{
#ifdef USE_SEAL
	int channel;
#endif

	if( stereo != 0 )
	{
		/* make sure it's either 0 or 1 */
		stereo = 1;
	}

	stream_cache_stereo = stereo;

	/* determine the number of samples per frame */
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;

	/* compute how many samples to generate this frame */
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	audio_buffer_length = NUM_BUFFERS * samples_per_frame + 20;

	if( soundcard == 0 || Machine->sample_rate == 0 )
	{
		return 0;
	}

#ifdef USE_SEAL
	for (channel = 0;channel <= stereo;channel++)
	{
		if (ACreateAudioVoice(&hVoice[channel]) != AUDIO_ERROR_NONE)
		{
			return 0;
		}

		if ((lpWave[channel] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
		{
			ADestroyAudioVoice(hVoice[channel]);
			return 0;
		}

		lpWave[channel]->wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->nSampleRate = nominal_sample_rate;
		lpWave[channel]->dwLength = 2*audio_buffer_length;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = lpWave[channel]->dwLength;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return 0;
		}

		memset(lpWave[channel]->lpData,0,lpWave[channel]->dwLength);
		APrimeVoice(hVoice[channel],lpWave[channel]);
		ASetVoiceFrequency(hVoice[channel],nominal_sample_rate);
	}

	if (stereo)
	{
		/* SEAL doubles volume for panned channels, so we have to compensate */
		ASetVoiceVolume(hVoice[0],32);
		ASetVoiceVolume(hVoice[1],32);

		ASetVoicePanning(hVoice[0],0);
		ASetVoicePanning(hVoice[1],255);

		AStartVoice(hVoice[0]);
		AStartVoice(hVoice[1]);
	}
	else
	{
		ASetVoiceVolume(hVoice[0],64);
		ASetVoicePanning(hVoice[0],128);
		AStartVoice(hVoice[0]);
	}
#endif

#ifdef USE_ALLEGRO
	mysample = create_sample(16,stereo,nominal_sample_rate,audio_buffer_length);
	if (mysample == 0)
	{
		return 0;
	}
	myvoice = allocate_voice(mysample);
	voice_set_playmode(myvoice,PLAYMODE_LOOP);
	if (stereo)
	{
		INT16 *buf = mysample->data;
		int p = 0;
		while (p != audio_buffer_length)
		{
			buf[2*p] = (INT16)0x8000;
			buf[2*p+1] = (INT16)0x8000;
			p++;
		}
	}
	else
	{
		INT16 *buf = mysample->data;
		int p = 0;
		while (p != audio_buffer_length)
		{
			buf[p] = (INT16)0x8000;
			p++;
		}
	}
	voice_start(myvoice);
#endif

	stream_playing = 1;
	voice_pos = 0;

	osd_sound_enable( 1 );

	return samples_this_frame;
}

void osd_stop_audio_stream(void)
{
#ifdef USE_SEAL
	int i;
#endif

	if( soundcard == 0 || Machine->sample_rate == 0 )
	{
		return;
	}

	osd_sound_enable( 0 );

#ifdef USE_SEAL
	/* stop and release voices */
	for (i = 0;i < SOUND_CHANNELS;i++)
	{
		if (lpWave[i] != 0)
		{
			AStopVoice(hVoice[i]);
			ADestroyAudioData(lpWave[i]);
			free(lpWave[i]);
			lpWave[i] = 0;
			ADestroyAudioVoice(hVoice[i]);
		}
	}
#endif
#ifdef USE_ALLEGRO
	voice_stop(myvoice);
	deallocate_voice(myvoice);
	destroy_sample(mysample);
	mysample = 0;
#endif

	stream_playing = 0;
}

static void updateaudiostream( int throttle )
{
	INT16 *data = stream_cache_data;
	int stereo = stream_cache_stereo;
	int len = stream_cache_len;
	int buflen;
	int start,end;

	if (!stream_playing)
	{
		/* error */
		return;
	}

	buflen = audio_buffer_length;
	start = voice_pos;
	end = voice_pos + len;
	if (end > buflen)
	{
		end -= buflen;
	}

#ifdef USE_SEAL
	if (throttle)   /* sync with audio only when speed throttling is not turned off */
	{
		profiler_mark(PROFILER_IDLE);
		for (;;)
		{
			LONG curpos;

			AGetVoicePosition(hVoice[0],&curpos);
			if (start < end)
			{
				if (curpos < start || curpos >= end)
				{
					break;
				}
			}
			else
			{
				if (curpos < start && curpos >= end)
				{
					break;
				}
			}
			AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
		}
		profiler_mark(PROFILER_END);
	}

	if (stereo)
	{
		INT16 *bufL,*bufR;
		int p;

		bufL = (INT16 *)lpWave[0]->lpData;
		bufR = (INT16 *)lpWave[1]->lpData;
		p = start;
		while (p != end)
		{
			if (p >= buflen)
			{
				p -= buflen;
			}
			bufL[p] = *data++;
			bufR[p] = *data++;
			p++;
		}
	}
	else
	{
		INT16 *buf;
		int p;

		buf = (INT16 *)lpWave[0]->lpData;
		p = start;
		while (p != end)
		{
			if (p >= buflen)
			{
				p -= buflen;
			}
			buf[p] = *data++;
			p++;
		}
	}

	if (start < end)
	{
		AWriteAudioData(lpWave[0],2*start,2*len);
		if (stereo)
		{
			AWriteAudioData(lpWave[1],2*start,2*len);
		}
	}
	else
	{
		int remain = buflen-start;
		AWriteAudioData(lpWave[0],2*start,2*remain);
		AWriteAudioData(lpWave[0],0,2*(len-remain));
		if (stereo)
		{
			AWriteAudioData(lpWave[1],2*start,2*remain);
			AWriteAudioData(lpWave[1],0,2*(len-remain));
		}
	}
#endif
#ifdef USE_ALLEGRO
{
	if (throttle)   /* sync with audio only when speed throttling is not turned off */
	{
		profiler_mark(PROFILER_IDLE);
		for (;;)
		{
			int curpos;

			curpos = voice_get_position(myvoice);
			if (start < end)
			{
				if (curpos < start || curpos >= end)
				{
					break;
				}
			}
			else
			{
				if (curpos < start && curpos >= end)
				{
					break;
				}
			}
		}
		profiler_mark(PROFILER_END);
	}

	if (stereo)
	{
		INT16 *buf = mysample->data;
		int p = start;
		while (p != end)
		{
			if (p >= buflen)
			{
				p -= buflen;
			}
			buf[2*p] = (*data++ * master_volume / 256) ^ 0x8000;
			buf[2*p+1] = (*data++ * master_volume / 256) ^ 0x8000;
			p++;
		}
	}
	else
	{
		INT16 *buf = mysample->data;
		int p = start;
		while (p != end)
		{
			if (p >= buflen)
			{
				p -= buflen;
			}
			buf[p] = (*data++ * master_volume / 256) ^ 0x8000;
			p++;
		}
	}
}
#endif

	voice_pos = end;
	if (voice_pos == buflen)
	{
		voice_pos = 0;
	}
}


int osd_update_audio_stream(INT16 *buffer)
{
	stream_cache_data = buffer;
	stream_cache_len = samples_this_frame;

	/* compute how many samples to generate next frame */
	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	return samples_this_frame;
}


int msdos_update_audio( int throttle )
{
	if( soundcard == 0 || Machine->sample_rate == 0 || stream_cache_data == 0 )
	{
		return 0;
	}

	profiler_mark(PROFILER_MIXER);

#ifdef USE_SEAL
	AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
#endif

	updateaudiostream( throttle );

	profiler_mark(PROFILER_END);

	return 1;
}


/* attenuation in dB */
void osd_set_mastervolume(int _attenuation)
{
	float volume;

	if (_attenuation > 0)
	{
		_attenuation = 0;
	}
	if (_attenuation < -32)
	{
		_attenuation = -32;
	}

	attenuation = _attenuation;

 	volume = 256.0;	/* range is 0-256 */
	while (_attenuation++ < 0)
	{
		volume /= 1.122018454;	/* = (10 ^ (1/20)) = 1dB */
	}

	master_volume = volume;

#ifdef USE_SEAL
	ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,master_volume);
#endif
}


int osd_get_mastervolume(void)
{
	return attenuation;
}


void osd_sound_enable(int enable_it)
{
#ifdef USE_SEAL
	if (enable_it)
	{
		ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,master_volume);
	}
	else
	{
		ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,0);
	}
#endif
#ifdef USE_ALLEGRO
	if (enable_it)
	{
		set_volume(255,0);
	}
	else
	{
		set_volume(0,0);
	}
#endif
}
