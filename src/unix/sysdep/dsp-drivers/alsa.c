/*
 * ALSA Sound Driver for xMAME
 *
 *  Copyright 2000 Luc Saillard <luc.saillard@alcove.fr>
 *  
 *  This file and the acompanying files in this directory are free software;
 *  you can redistribute them and/or modify them under the terms of the GNU
 *  Library General Public License as published by the Free Software Foundation;
 *  either version 2 of the License, or (at your option) any later version.
 *
 *  These files are distributed in the hope that they will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with these files; see the file COPYING.LIB.  If not,
 *  write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 * Changelog:
 *   v 0.1 Thu, 10 Aug 2000 08:29:00 +0200
 *     - initial release
 *     - TODO: find the best sound card to play sound.
 * 
 */

#include "xmame.h"           /* xMAME common header */
#include "devices.h"         /* xMAME device header */

#ifdef SYSDEP_DSP_ALSA
#include <sys/ioctl.h>       /* System and I/O control */
#include <sys/asoundlib.h>   /* ALSA sound library header */
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct alsa_dsp_priv_data
{
  snd_pcm_t *pcm_handle;
  char *audiobuf;
  int frags;
  int frag;
  int buffer_size;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static int alsa_dsp_init(void);
static void *alsa_dsp_create(const void *flags);
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
    int count);
static int alsa_device_list(struct rc_option *option, const char *arg,
    int priority);
static int alsa_dsp_set_format(struct alsa_dsp_priv_data *priv, snd_pcm_format_t *format);


/* public variables */

static int alsa_card;
static int alsa_device;


struct rc_option alsa_dsp_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
    { "Alsa Sound System", NULL,     rc_seperator, NULL,
      NULL,    0,      0,    NULL,
      NULL },
    { "list-alsa-cards", NULL,	rc_use_function_no_arg, NULL,
      NULL,    0,      0,    alsa_device_list,
      "List available sound-dsp plugins" },
    { "alsacard",    NULL,     rc_int,   &alsa_card,
      "0",     0,     32,    NULL,
      "select card # or card id (0-32)" },
    { "alsadevice",    NULL,     rc_int,   &alsa_device,
      "0",     0,     32,    NULL,
      "select device #" },
    { NULL,    NULL,     rc_end,   NULL,
      NULL,    0,      0,    NULL,
      NULL }
};

const struct plugin_struct sysdep_dsp_alsa = {
   "alsa",
   "sysdep_dsp",
   "Alsa Sound System DSP plugin",
   alsa_dsp_opts,
   alsa_dsp_init,
   NULL, /* no exit */
   alsa_dsp_create,
   4     /* high priority */
};

/* private variables */
static int alsa_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;


/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */

/*
 * Function name : alsa_dsp_init
 *
 * Description : Detect if a card is present on the machine
 * Output :
 *   a boolean
 */
static int alsa_dsp_init(void)
{
  int cards=snd_cards();
  if (cards<=0)
   {
     fprintf(stderr,"No cards detected.\nALSA sound disabled.\n");
     return 1;
   }
  return 0;
}



/*
 * Function name : alsa_dsp_create
 *
 * Description : Create an instance of dsp plugins
 * Input :
 *   flags: a ptr to struct sysdep_dsp_create_params
 * Output :
 *   a ptr to a struct sysdep_dsp_struct
 */
static void *alsa_dsp_create(const void *flags)
{
  int err;
  struct alsa_dsp_priv_data *priv = NULL;
  struct sysdep_dsp_struct *dsp = NULL;
  const struct sysdep_dsp_create_params *params = flags;
  char *cardname;
  struct snd_pcm_channel_info cinfo;
  snd_pcm_format_t format;

  /* allocate the dsp struct */
  if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
     fprintf(stderr,
	 "error malloc failed for struct sysdep_dsp_struct\n");
     return NULL;
   }

  /* alloc private data */
  if(!(priv = calloc(1, sizeof(struct alsa_dsp_priv_data))))
   {
     fprintf(stderr,
	 "error malloc failed for struct alsa_dsp_priv_data\n");
     alsa_dsp_destroy(dsp);
     return NULL;
   }

  /* fill in the functions and some data */
  memset(priv,0,sizeof(struct alsa_dsp_priv_data));
  dsp->_priv = priv;
  dsp->get_freespace = alsa_dsp_get_freespace;
  dsp->write = alsa_dsp_write;
  dsp->destroy = alsa_dsp_destroy;
  dsp->hw_info.type = params->type;
  dsp->hw_info.samplerate = params->samplerate;

  priv->buffer_size = -1;
  memset(&format, 0, sizeof(format));
  format.interleave = 4;
#ifdef LSB_FIRST
  format.format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)? SND_PCM_SFMT_S16_LE:SND_PCM_SFMT_U8;
#else
  format.format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)? SND_PCM_SFMT_S16_BE:SND_PCM_SFMT_U8;
#endif
  format.rate = dsp->hw_info.samplerate;
  format.voices = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2:1;

  if ((err = snd_card_get_longname(alsa_card, &cardname)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to obtain longname: %s\n", snd_strerror(err));
     return NULL;
   }
  fprintf(stdout_file, "Using soundcard '%s'\n", cardname);
  free(cardname);

  if ((err = snd_pcm_open(&priv->pcm_handle, alsa_card, alsa_device, SND_PCM_OPEN_PLAYBACK|SND_PCM_OPEN_NONBLOCK)) < 0)
   {
     fprintf(stderr_file, "Alsa error: audio open error: %s\n", snd_strerror(err));
     return NULL;
   }

  /* Get some information about this device */
  memset(&cinfo, 0, sizeof(cinfo));
  cinfo.channel = SND_PCM_CHANNEL_PLAYBACK;
  if ((err = snd_pcm_channel_info(priv->pcm_handle, &cinfo)) < 0)
   {
     fprintf(stderr_file, "Error: channel info error: %s\n", snd_strerror(err));
     alsa_dsp_destroy(dsp);
     return NULL;
   }
  
  /* Control if this device can play the sound */
  if (cinfo.min_rate > format.rate || cinfo.max_rate < format.rate)
   {
     fprintf(stderr_file, "Alsa error: unsupported rate %iHz (valid range is %iHz-%iHz)\n", format.rate, cinfo.min_rate, cinfo.max_rate);
     return NULL;
   }
  if (!(cinfo.formats & (1 << format.format)))
   {
     fprintf(stderr_file, "Alsa error: requested format %s isn't supported with hardware\n", snd_pcm_get_format_name(format.format));
     return NULL;
   }

  /* Alloc a minimal buffer */
  priv->buffer_size = 1024;
  priv->audiobuf = (char *)malloc(priv->buffer_size);
  if (priv->audiobuf == NULL)
   {
     fprintf(stderr_file, "Alsa error: not enough memory\n");
     return NULL;
   }

  if (alsa_dsp_set_format(priv,&format)==0)
    return NULL;

  dsp->hw_info.bufsize =  priv->buffer_size * priv->frags /
    alsa_dsp_bytes_per_sample[dsp->hw_info.type];


  memset(priv->audiobuf,0,priv->buffer_size * priv->frag);

  if (snd_pcm_channel_go(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK)<0) 
   {
     fprintf(stderr_file, "Alsa error: unable to start playback\n");
     return NULL;
   }

  fprintf(stderr_file, "info: set to %dbit linear %s %dHz bufsize=%d\n",
      (dsp->hw_info.type & SYSDEP_DSP_16BIT)? 16:8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO)? "stereo":"mono",
      dsp->hw_info.samplerate,
      dsp->hw_info.bufsize);

  return dsp;
}

/*
 * Function name : alsa_dsp_destroy
 *
 * Description :
 * Input :
 * Output :
 */
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
  struct alsa_dsp_priv_data *priv = dsp->_priv;

  if(priv)
   {
     if(priv->pcm_handle)
      {
	snd_pcm_playback_drain(priv->pcm_handle);
	snd_pcm_close(priv->pcm_handle);
      }
     if (priv->audiobuf)
       free(priv->audiobuf);
     free(priv);
   }
  free(dsp);
}

/*
 * Function name : alsa_dsp_get_freespace
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
  struct alsa_dsp_priv_data *priv = dsp->_priv;
  snd_pcm_channel_status_t status;

  memset(&status, 0, sizeof(status));
  status.channel = SND_PCM_CHANNEL_PLAYBACK;
  if (snd_pcm_channel_status(priv->pcm_handle,&status))
    return -1;
  else
    return status.free / alsa_dsp_bytes_per_sample[dsp->hw_info.type];
}

/*
 * Function name : alsa_dsp_write
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
    int count)
{
  int buffer_size,result;
  struct alsa_dsp_priv_data *priv = dsp->_priv;

  buffer_size=count * alsa_dsp_bytes_per_sample[dsp->hw_info.type];

  if ( (result=snd_pcm_write(priv->pcm_handle,(const void *)data, buffer_size))!=buffer_size)
   {
     snd_pcm_channel_status_t status;

     memset(&status, 0, sizeof(status));
     status.channel = SND_PCM_CHANNEL_PLAYBACK;
     if (snd_pcm_channel_status(priv->pcm_handle, &status)<0)
      {
	fprintf(stderr_file, "Alsa error: playback channel status error\n");
	return -1;
      }
     if (status.status == SND_PCM_STATUS_UNDERRUN)
      {
	fprintf(stdout_file,"Alsa warning: underrun at position %u!!!\n", status.scount);
	if (snd_pcm_plugin_prepare(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK)<0)
	 {
	   fprintf(stderr_file, "Alsa error: underrun playback channel prepare error\n");
	   return -1;
	 }
	priv->frag = 0;
	return 0;
      }
     else
      {
	fprintf(stderr_file, "Alsa error: %d\n",status.status);
      }
   }
  return result / alsa_dsp_bytes_per_sample[dsp->hw_info.type];
}

/*
 * Function name : 
 *
 * Description :
 * Input :
 * Output :
 */

static int alsa_device_list(struct rc_option *option, const char *arg,
    int priority)
{
  snd_ctl_t *handle;
  int card, err, dev;
  unsigned int mask;
  struct snd_ctl_hw_info info;
  snd_pcm_info_t pcminfo;

  mask = snd_cards_mask();
  if (!mask)
   {
     printf("Alsa: no soundcards found...\n");
     return -1;
   }
  fprintf(stdout,"Alsa Cards:\n");
  for (card = 0; card < SND_CARDS; card++)
   {
     if (!(mask & (1 << card)))
       continue;
     if ((err = snd_ctl_open(&handle, card)) < 0)
      {
	fprintf(stderr,"Alsa error: control open (%i): %s\n", card, snd_strerror(err));
	continue;
      }
     if ((err = snd_ctl_hw_info(handle, &info)) < 0)
      {
	fprintf(stderr,"Alsa error: control hardware info (%i): %s\n", card, snd_strerror(err));
	snd_ctl_close(handle);
	continue;
      }
     for (dev = 0; dev < info.pcmdevs; dev++)
      {
	if ((err = snd_ctl_pcm_info(handle, dev, &pcminfo)) < 0)
	 {
	   fprintf(stderr,"Alsa error: control digital audio info (%i): %s\n", card, snd_strerror(err));
	   continue;
	 }
	if (pcminfo.flags & SND_PCM_INFO_PLAYBACK)
	 {
	   printf("Card #%i / Device #%i => %s\n",
	       card,
	       dev,
	       pcminfo.name);
	 }
      }
     snd_ctl_close(handle);
   }
  return -1;
}

/*
 * Function name : alsa_dsp_set_format
 *
 * Description : Set the card to the audio format
 * Input :
 *   priv: a ptr to struct alsa_dsp_priv_data
 *   format: the format wanted
 * Output :
 *  priv is modified with the current parameters.
 *  a boolean if the card accept the value.
 *  
 */
static int alsa_dsp_set_format(struct alsa_dsp_priv_data *priv, snd_pcm_format_t *format)
{
  unsigned int bps;	/* bytes per second */
  unsigned int size;	/* fragment size */
  int err;
  struct snd_pcm_channel_params params;
  struct snd_pcm_channel_setup setup;

  bps = format->rate * format->voices;
  switch (format->format)
   {
    case SND_PCM_SFMT_U8:
      break;
    case SND_PCM_SFMT_S16_LE:
    case SND_PCM_SFMT_S16_BE:
      bps <<= 1;
      break;
    default:
      fprintf(stderr_file,"Alsa error: playback format unknow (%d)",format->format);
      return 0;
   }
  bps >>= 2;		/* ok.. this buffer should be 0.25 sec */
  if (bps < 16)
    bps = 16;
  size = 1;
  while ((size << 1) < bps)
    size <<= 1;

  snd_pcm_channel_flush(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK);

  /* Configure the device with the format specified */
  memset(&params, 0, sizeof(params));
  params.channel = SND_PCM_CHANNEL_PLAYBACK;
  params.mode = SND_PCM_MODE_BLOCK;
  memcpy(&params.format, format, sizeof(snd_pcm_format_t));
  params.start_mode = SND_PCM_START_FULL;
  params.stop_mode = SND_PCM_STOP_STOP;
  params.buf.block.frag_size = size;
  params.buf.block.frags_min = 1;
  params.buf.block.frags_max = -1;		/* little trick (playback only) */

  if ((err = snd_pcm_channel_params(priv->pcm_handle, &params)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to set channel params: %s\n",snd_strerror(err));
     return 0;
   }

  if ((err = snd_pcm_plugin_prepare(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to prepare channel: %s\n",snd_strerror(err));
     return 0;
   }

  /* obtain current PCM setup */
  memset(&setup, 0, sizeof(setup));
  setup.mode = SND_PCM_MODE_BLOCK;
  setup.channel = SND_PCM_CHANNEL_PLAYBACK;

  if ((err = snd_pcm_channel_setup(priv->pcm_handle, &setup)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to obtain setup: %s\n",snd_strerror(err));
     return 0;
   }

  fprintf(stdout_file,"frag_size = %d / frags = %d / frags_min = %d / frags_max = %d\n",
      setup.buf.block.frag_size,
      setup.buf.block.frags,
      setup.buf.block.frags_min,
      setup.buf.block.frags_max);

  /* Fill the private structure */
  priv->frags = setup.buf.block.frags;
  priv->buffer_size = setup.buf.block.frag_size;
  priv->audiobuf = (char *)realloc(priv->audiobuf, priv->buffer_size);
  if (priv->audiobuf == NULL)
   {
     fprintf(stderr_file, "Alsa error: not enough memory, need to allocate %d\n",priv->buffer_size);
     return 0;
   }
  return 1;
}

#endif

/*
 * Function name : 
 *
 * Description :
 * Input :
 * Output :
 */


