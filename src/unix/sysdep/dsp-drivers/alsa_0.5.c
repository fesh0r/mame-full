/*
 * ALSA 0.5.x Sound Driver for xMAME
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
 */

/* our per instance private data struct */
struct alsa_dsp_priv_data
{
  snd_pcm_t *pcm_handle;
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
      "List available sound cards" },
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
  int cards = snd_cards();
  if (cards <= 0)
   {
     fprintf(stderr, "No cards detected.\n" "ALSA sound disabled.\n");
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
  snd_pcm_channel_info_t cinfo;
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
  dsp->hw_info.bufsize = 0;

  memset(&format, 0, sizeof(format));
  format.interleave = 1;
#ifdef LSB_FIRST
  format.format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)? SND_PCM_SFMT_S16_LE:SND_PCM_SFMT_U8;
#else
  format.format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)? SND_PCM_SFMT_S16_BE:SND_PCM_SFMT_U8;
#endif
  format.rate = dsp->hw_info.samplerate;
  format.voices = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 2 : 1;

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

  if (alsa_dsp_set_format(priv, &format) == 0)
    return NULL;

  fprintf(stderr_file, "info: set to %dbit linear %s %dHz\n",
      (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? "stereo" : "mono",
      dsp->hw_info.samplerate);

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
  if (snd_pcm_channel_status(priv->pcm_handle, &status))
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
  int buffer_size, result;
  struct alsa_dsp_priv_data *priv = dsp->_priv;

  buffer_size = count * alsa_dsp_bytes_per_sample[dsp->hw_info.type];

  if ( (result=snd_pcm_write(priv->pcm_handle, (const void *)data, buffer_size)) != buffer_size)
   {
     snd_pcm_channel_status_t status;

     memset(&status, 0, sizeof(status));
     status.channel = SND_PCM_CHANNEL_PLAYBACK;
     if (snd_pcm_channel_status(priv->pcm_handle, &status) < 0)
      {
	fprintf(stderr_file, "Alsa error: playback channel status error\n");
	return -1;
      }
     if (status.status == SND_PCM_STATUS_UNDERRUN)
      {
#if 0 /* DEBUG */
	fprintf(stdout_file,"Alsa warning: underrun at position %u!!!\n", status.scount);
#endif
	if (snd_pcm_channel_prepare(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK) < 0)
	 {
	   fprintf(stderr_file, "Alsa error: underrun playback channel prepare error\n");
	   return -1;
	 }
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
  fprintf(stdout, "Alsa Cards:\n");
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
  unsigned int size;	/* queue size */
  int err;
  snd_pcm_channel_params_t params;
  snd_pcm_channel_setup_t setup;

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
  bps >>= 4;		/* ok.. this buffer should be 1/16 sec */
  if (bps < 16)
    bps = 16;
  size = 1;
  while ((size << 1) < bps)
    size <<= 1;

  snd_pcm_channel_flush(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK);

  /* Configure the device with the format specified */
  memset(&params, 0, sizeof(params));
  params.channel = SND_PCM_CHANNEL_PLAYBACK;
  params.mode = SND_PCM_MODE_STREAM;
  memcpy(&params.format, format, sizeof(snd_pcm_format_t));
  params.start_mode = SND_PCM_START_FULL;
  params.stop_mode = SND_PCM_STOP_STOP;
  params.buf.stream.queue_size = size;
  params.buf.stream.fill = SND_PCM_FILL_SILENCE;
  params.buf.stream.max_fill = size;

  if ((err = snd_pcm_channel_params(priv->pcm_handle, &params)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to set channel params: %s\n", snd_strerror(err));
     return 0;
   }

  if ((err = snd_pcm_channel_prepare(priv->pcm_handle, SND_PCM_CHANNEL_PLAYBACK)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to prepare channel: %s\n", snd_strerror(err));
     return 0;
   }

  /* obtain current PCM setup */
  memset(&setup, 0, sizeof(setup));
  setup.mode = SND_PCM_MODE_STREAM;
  setup.channel = SND_PCM_CHANNEL_PLAYBACK;

  if ((err = snd_pcm_channel_setup(priv->pcm_handle, &setup)) < 0)
   {
     fprintf(stderr_file, "Alsa error: unable to obtain setup: %s\n", snd_strerror(err));
     return 0;
   }

  fprintf(stdout_file, "queue_size = %d\n", setup.buf.stream.queue_size);

  return 1;
}
