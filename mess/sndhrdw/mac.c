/***************************************************************************
	mac.c
	Sound handler
****************************************************************************/
#include "driver.h"

#include "includes/mac.h"

static int mac_stream;
static int sample_enable = 0;
static UINT16 *mac_snd_buf_ptr;


#define MAC_MAIN_SND_BUF_OFFSET	0x0300
#define MAC_ALT_SND_BUF_OFFSET	0x5F00
#define MAC_SND_BUF_SIZE		370			/* total number of scan lines */
#define MAC_SAMPLE_RATE			( MAC_SND_BUF_SIZE * 60 /*22255*/ )	/* scan line rate, should be 22254.5 Hz */


/* intermediate buffer */
#define SND_CACHE_SIZE 128

static UINT8 *snd_cache;
static int snd_cache_len;
static int snd_cache_head;
static int snd_cache_tail;


/************************************/
/* Stream updater                   */
/************************************/

static void mac_sound_update(int num, INT16 *buffer, int length)
{
	INT16 last_val = 0;

	/* if we're not enabled, just fill with 0 */
	if (Machine->sample_rate == 0)
	{
		memset(buffer, 0, length * sizeof(INT16));
		return;
	}

	/* fill in the sample */
	while (length && snd_cache_len)
	{
		*buffer++ = last_val = ((snd_cache[snd_cache_head] << 8) ^ 0x8000) & 0xff00;
		snd_cache_head++;
		snd_cache_head %= SND_CACHE_SIZE;
		snd_cache_len--;
		length--;
	}

	while (length--)
	{	/* should never happen */
		*buffer++ = last_val;
	}
}



/************************************/
/* Sound handler start              */
/************************************/

int mac_sh_start(const struct MachineSound *msound)
{
	snd_cache = auto_malloc(SND_CACHE_SIZE * sizeof(*snd_cache));
	if (!snd_cache)
		return 1;

	mac_stream = stream_init("Mac Sound", 100, MAC_SAMPLE_RATE, 0, mac_sound_update);
	if (mac_stream < 0)
		return 1;

	snd_cache_head = snd_cache_len = snd_cache_tail = 0;
	return 0;
}



/************************************/
/* Sound handler stop               */
/************************************/

void mac_sh_stop(void)
{
}



/************************************/
/* Sound handler update 			*/
/************************************/

void mac_sh_update(void)
{
}



/*
	Set the sound enable flag (VIA port line)
*/
void mac_enable_sound(int on)
{
	sample_enable = on;
}



/*
	Set the current sound buffer (one VIA port line)
*/
void mac_set_sound_buffer(int buffer)
{
	if (buffer)
		mac_snd_buf_ptr = (UINT16 *) (mess_ram + mess_ram_size - MAC_MAIN_SND_BUF_OFFSET);
	else
		mac_snd_buf_ptr = (UINT16 *) (mess_ram + mess_ram_size - MAC_ALT_SND_BUF_OFFSET);
}



/*
	Set the current sound volume (3 VIA port line)
*/
void mac_set_volume(int volume)
{
	stream_update(mac_stream, 0);

	volume = (100 / 7) * volume;

	mixer_set_volume(mac_stream, volume);
}



/*
	Fetch one byte from sound buffer and put it to sound output (called every scanline)
*/
void mac_sh_updatebuffer(void)
{
	static int indexx;
	UINT16 *base = mac_snd_buf_ptr;

	indexx++;
	indexx %= 370;

	if (snd_cache_len >= SND_CACHE_SIZE)
	{
		/* clear buffer */
		stream_update(mac_stream, 0);
	}

	if (snd_cache_len >= SND_CACHE_SIZE)
		/* should never happen */
		return;

	snd_cache[snd_cache_tail] = sample_enable ? (base[indexx] >> 8) & 0xff : 0;
	snd_cache_tail++;
	snd_cache_tail %= SND_CACHE_SIZE;
	snd_cache_len++;
}
