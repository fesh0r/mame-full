/*
 * Driver for Mac OS X core audio interface, primarily for use
 * with the OpenStep driver on this system. This is a driver for
 * a beta operating system, and thus does not support things that the
 * API cannot currently support. This currently means that only audio
 * at 44100Hz is really supported and that we expand mono into stereo if
 * the game only provides mono. We do, however, support the lower speeds
 * that are integral divisions of 44100 by repeating the samples.
 *
 * -bat. 06/12/2000
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <CoreAudio/AudioHardware.h>

#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

#define NAME_LEN 256		/* max length of a device name */

/*
 * We have to implement a FIFO of blocks of floats, without using up
 * vast amounts of memory, or being too inefficient. We thus require
 * the queued blocks to be created by malloc (so we do not have to copy
 * the data) but free them ourself upon dequeue.
 */

struct audio_block {
	float *block_base;		/* base of the memory block */
	float *current_base;		/* current unread position */
	int samples_left;		/* number left in block */
	struct audio_block *next;	/* next block in list */
};

struct audio_queue {
	struct audio_block *head;	/* head of queue */
	struct audio_block *tail;	/* tail of queue */
	pthread_mutex_t	mutex;		/* queue locking mutex */
};

/*
 * Adds a block of audio samples to the end of the given queue. This
 * function is 'unsafe' and is thus wrapped by a function that handles
 * the queue mutex lock.
 */

static void
unsafe_queue_audio_block(struct audio_queue *queue, float *block, int len)
{
	struct audio_block *new = malloc(sizeof(struct audio_block));

	/* if malloc fails we ditch the block */
	if(!new)
		return;

	/* fill it out */
	new->block_base = block;
	new->current_base = block;
	new->samples_left = len;
	new->next = NULL;

	/* add it to the end */
	if(!queue->head) {
		queue->head = new;
		queue->tail = new;
	} else {
		queue->tail->next = new;
		queue->tail = new;
	}
}

/*
 * Wrapper to handle mutex locking on queue
 */

static void
queue_audio_block(struct audio_queue *queue, float *block, int len)
{
	pthread_mutex_lock(&queue->mutex);
	unsafe_queue_audio_block(queue, block, len);
	pthread_mutex_unlock(&queue->mutex);
}

/*
 * Dequeue a certain number of audio bytes from the fifo. For blocks
 * smaller than the remaining number we simply copy them and reduce
 * the count. For blocks that go over this limit we have to make a
 * recursive call on the function to fill out the rest. When there
 * are no blocks left the buffer is zero padded. As with the queueing function
 * this is unsafe and wrapped by a mutex handling funcion.
 */

static void
unsafe_dequeue_audio_block(struct audio_queue *queue, float *block, int len)
{
	struct audio_block *head = queue->head;

	/* anything to do ? */
	if(len < 1)
		return;

	/* zero padding ? */
	if(!head) {
		int i;
		for(i=0;i<len;i++)
			*block++ = 0.0;
		return;
	}

	/* smaller than current */
	if(len < head->samples_left) {
		memcpy(block, head->current_base, len*sizeof(float));
		head->samples_left -= len;
		head->current_base += len;
		return;
	}

	/* copy all of this block */
	memcpy(block, head->current_base, head->samples_left*sizeof(float));
	len -= head->samples_left;
	block += head->samples_left;

	/* free it up and recuurse */
	queue->head = head->next;
	if(!queue->head)
		queue->tail = NULL;
	free(head->block_base);
	free(head);
	unsafe_dequeue_audio_block(queue, block, len);
}

/*
 * Wrapper to handle mutex locking on queue
 */

static void
dequeue_audio_block(struct audio_queue *queue, float *block, int len)
{
	pthread_mutex_lock(&queue->mutex);
	unsafe_dequeue_audio_block(queue, block, len);
	pthread_mutex_unlock(&queue->mutex);
}

/*
 * This is the private data structure containing the audio queue and the
 * size of the sound buffer. We assume this is static so we do not have to
 * request it every time the callback is called.
 */

struct coreaudio_private {
	AudioDeviceID device;
	struct audio_queue queue;
	int buffer_size;
};

/*
 * This is the callback function which the audio device calls whenever it
 * wants a new block of data to process. The data is copied directly from the
 * queue into the output buffer, padded with zeroes in the case of an
 * underrun.
 */

static OSStatus
coreaudio_dsp_play (AudioDeviceID device, const AudioTimeStamp *now_time,
		const void *in_data, const AudioTimeStamp *in_time,
		void *out_data, const AudioTimeStamp *out_time,
		void *data)
{
	struct coreaudio_private *priv = (struct coreaudio_private*)data;
	dequeue_audio_block(&(priv->queue), out_data, priv->buffer_size);
	return noErr;
}

/*
 * Destroy function. We stop the sound device from playing and detach
 * ourselves from it. We do noit free up the dsp structure as we dont
 * entriely trust public beta not to make a final call to the callback.
 */

static void
coreaudio_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
	struct coreaudio_private *priv = (struct coreaudio_private*)dsp->_priv;
	OSStatus audio_err;

	/* stop the device */
        audio_err = AudioDeviceStop(priv->device, coreaudio_dsp_play);
        if(audio_err != noErr)
		fprintf(stderr,"error %ld stopping audio device\n",
				audio_err);

	/* remove the callback function */
        audio_err = AudioDeviceRemoveIOProc(priv->device, coreaudio_dsp_play);
        if(audio_err != noErr)
		fprintf(stderr,"error %ld removing callback function\n",
				audio_err);
}

/*
 * Take a block of data, covert it into stereo floats and queue it.
 * The conversion cannot be a fast process, but the API requires that
 * samples are expressed as floats between -1.0 and 1.0 for some reason.
 * It makes sense numerically, but it's hardly fast ! We also mutiply up
 * the samples here so we are always playing at 44100Hz.
 */
   
static int
coreaudio_dsp_write(struct sysdep_dsp_struct *dsp,
		unsigned char *data, int count)
{
	struct coreaudio_private *priv = (struct coreaudio_private*)dsp->_priv;
	int is_stereo = dsp->hw_info.type & SYSDEP_DSP_STEREO;
	int mult = 44100 / dsp->hw_info.samplerate;
	float *data_block, *data_ptr;
	short *short_ptr;
	int i;

	/* make the data block - always stereo */
	data_block = malloc(count * 2 * sizeof(float) * mult);
	if(!data_block) {
		fprintf(stderr, "out of memory queueing audio block");
		return count;
	}

	/*
	 * copy in the data, each sample is added twice if we are running
	 * a half rate emulation. If we are in mono mode then the second sample
	 * is the same as the first and not taken from the buffer at all
	 */
	data_ptr = data_block;
	short_ptr = (short*)data;
	for(i=0;i<count;i++) {
		float left_chan, right_chan;
		int m;

		/* setup the left and right values */
		left_chan = *short_ptr++ / 32768.0;
		if(is_stereo)
			right_chan = *short_ptr++ / 32768.0;
		else
			right_chan = left_chan;

		/* add the sammples */
		for(m=0;m<mult;m++) {
			*data_ptr++ = left_chan;
			*data_ptr++ = right_chan;
		}
	}

	/* and queue it */
	queue_audio_block(&(priv->queue), data_block, count * 2 * mult);
	return count;
}

/*
 * Creation function. Attach ourselves to the default audio device if
 * we can, and set up the various paerts of our internal structure. Due
 * to the limitations of public beta we can only play 44.1kHz sample rate
 * sound, which isnt going to help make things runs fast either.
 */

static void*
coreaudio_dsp_create(const void *flags)
{
	const struct sysdep_dsp_create_params *params = flags;
	struct sysdep_dsp_struct *dsp = NULL;
	struct coreaudio_private *priv = NULL;
	OSStatus audio_err;
	UInt32 audio_count, audio_buff_len;
	char audio_device[NAME_LEN];
	AudioStreamBasicDescription audio_format;

	/* allocate the dsp struct */
	if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
	{
		perror("malloc failed for struct sysdep_dsp_struct\n");
		return NULL;
	}

	/* set the parameters */
	dsp->hw_info.type |= SYSDEP_DSP_16BIT;
	if(params->type & SYSDEP_DSP_STEREO)
		dsp->hw_info.type |= SYSDEP_DSP_STEREO;
	else 
		fprintf(stderr, "info: emulating mono onto stereo output\n");

	/* print the info and check samplerate */
	fprintf(stderr, "info: %s sound at %d hz\n",
			(dsp->hw_info.type & SYSDEP_DSP_STEREO) ?
			"stereo" : "mono", params->samplerate);

	/* check integral multiples */
	if(44100 % params->samplerate) {
		fprintf(stderr, "info: sample rate must divide into 44100\n");
		return NULL;
	} else {
		int mult = 44100 / params->samplerate;
		if(mult != 1)
			fprintf(stderr, "info: repeating %d samples %d times\n",
					params->samplerate, mult);
	}
	
	/* make the private structure */
	if (!(priv = calloc(1, sizeof(struct coreaudio_private)))) {
		perror("malloc failed for struct coreaudio_private\n");
		return NULL;
	}

	/* set up with queue, no device, and zero length buffer */
 	priv->device = kAudioDeviceUnknown;
	priv->queue.head = NULL;
	priv->queue.tail = NULL;
	if(pthread_mutex_init(&(priv->queue.mutex),NULL)) {
		perror("failed to create mutex\n");
		return NULL;
	}
	priv->buffer_size = 0;

	/* get the default device */
	audio_count = sizeof(AudioDeviceID);
	audio_err = AudioHardwareGetProperty(
			kAudioHardwarePropertyDefaultOutputDevice,
			&audio_count, &(priv->device));
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld getting default audio device\n",
				audio_err);
		return NULL;
	}

	/* whats it called ? */
	audio_count = NAME_LEN;
	audio_err = AudioDeviceGetProperty(priv->device, 0, false,
			kAudioDevicePropertyDeviceName,
			&audio_count, audio_device);
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld getting default device name\n",
				audio_err);
		return NULL;
	}
	audio_device[NAME_LEN-1] = '\0';	/* just in case */
	fprintf(stderr, "info: sound device is %s\n", audio_device);

	/* get the basic parameters */
	audio_count = sizeof(AudioStreamBasicDescription);
	audio_err = AudioDeviceGetProperty(priv->device, 0, false,
			kAudioDevicePropertyStreamFormat,
			&audio_count, &audio_format);
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld getting basic device parameters\n",
				audio_err);
		return NULL;
	}

	/* check the parameters */
	if(audio_format.mChannelsPerFrame != 2) {
		fprintf(stderr,"basic device does not do stereo!\n");
		return NULL;
	}
	if(audio_format.mFormatID != kAudioFormatLinearPCM) {
		fprintf(stderr,"basic device does not do linear pcm!\n");
		return NULL;
	}

	/* get the buffer size */
	audio_count = sizeof(UInt32);
	audio_err = AudioDeviceGetProperty(priv->device, 0, false,
			kAudioDevicePropertyBufferSize,
			&audio_count, &audio_buff_len);
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld getting device buffer length\n",
				audio_err);
		return NULL;
	}
	priv->buffer_size = audio_buff_len / sizeof(float);
	fprintf(stderr, "info: buffer length is %d samples\n",
			priv->buffer_size);

	/* attach the callback function */
        audio_err = AudioDeviceAddIOProc(priv->device,
			coreaudio_dsp_play, (void*)priv);
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld adding callback function\n",
				audio_err);
		return NULL;
	}

	/* start it playing */
        audio_err = AudioDeviceStart(priv->device, coreaudio_dsp_play);
	if(audio_err != noErr) {
		fprintf(stderr,"error %ld starting audio device\n",
				audio_err);
		return NULL;
	}
	
	/* fill in the functions and private data */
	dsp->_priv = priv;
	dsp->write = coreaudio_dsp_write;
	dsp->destroy = coreaudio_dsp_destroy;
	dsp->hw_info.type = params->type;
	dsp->hw_info.samplerate = params->samplerate;

	return dsp;
}

/*
 * The public variables structure
 */

const struct plugin_struct sysdep_dsp_coreaudio = {
	"coreaudio",
	"sysdep_dsp",
	"Apple OS X beta CoreAudio plugin",
	NULL, 				/* no options */
	NULL,				/* no init */
	NULL,				/* no exit */
	coreaudio_dsp_create,
	3				/* high priority */
};
