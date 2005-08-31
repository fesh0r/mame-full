/****************************************************************************

	macbin.c

	MacBinary filter for use with Mac and ProDOS drivers

*****************************************************************************

  MacBinary file format

  Offset  Length  Description
  ------  ------  -----------
       0       1  [I]   Magic byte (0x00)
	   1      64  [I]   File name (Pascal String)
      65       4  [I]   File Type Code
	  69       4  [I]   File Creator Code
	  73       1  [I]   Finder Flags (bits 15-8)
	  74       1  [I]   Magic byte (0x00)
	  75       2  [I]   File Vertical Position
	  77       2  [I]   File Horizontal Position
	  79       2  [I]   Window/Folder ID
	  81       1  [I]   Protected (bit 0)
	  82       1  [I]   Magic byte (0x00)
	  83       4  [I]   Data Fork Length
	  87       4  [I]   Resource Fork Length
	  91       4  [I]   Creation Date
	  95       4  [I]   Last Modified Date
	  99       4  [I]   "Get Info" comment length
     101       1  [II]  Finder Flags (bits 7-0)
	 102       4  [III] MacBinary III Signature 'mBIN'
	 106       1  [III] Script of Filename
	 107       1  [III] Extended Finder Flags
	 116       4  [II]  Unpacked Length
	 120       2  [II]  Secondary Header Length
	 122       1  [II]  MacBinary II Version Number (II: 0x81, III: 0x82)
	 123       1  [II]  Minimum Compatible MacBinary II Version Number (0x81)
	 124       2  [II]  CRC of previous 124 bytes  
	
	For more information, consult http://www.lazerware.com/formats/macbinary.html

*****************************************************************************/

#include <string.h>
#include "imgtool.h"

static UINT32 mac_time(time_t t)
{
	/* not sure if this is correct... */
	return t
		+ (((1970 - 1904) * 365) + 17) * 24 * 60 * 60;
}



static imgtoolerr_t macbinary_readfile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf)
{
	static const UINT32 attrs[] =
	{
		IMGTOOLATTR_TIME_CREATED,
		IMGTOOLATTR_TIME_LASTMODIFIED,
		IMGTOOLATTR_INT_MAC_TYPE,
		IMGTOOLATTR_INT_MAC_CREATOR,
		0
	};
	imgtoolerr_t err;
	UINT8 header[126];
	const char *basename;
	int i, len;
	UINT32 type_code;
	UINT32 creator_code;
	imgtool_forkent fork_entries[4];
	const imgtool_forkent *data_fork = NULL;
	const imgtool_forkent *resource_fork = NULL;
	UINT32 creation_time;
	UINT32 lastmodified_time;
	imgtool_attribute attr_values[4];

	/* get the forks */
	err = img_listforks(image, filename, fork_entries, sizeof(fork_entries));
	if (err)
		return err;
	for (i = 0; fork_entries[i].type != FORK_END; i++)
	{
		if (fork_entries[i].type == FORK_DATA)
			data_fork = &fork_entries[i];
		else if (fork_entries[i].type == FORK_RESOURCE)
			resource_fork = &fork_entries[i];
	}

	/* get the attributes */
	err = img_module(image)->get_attrs(image, filename, attrs, attr_values);
	if (err)
		return err;
	creation_time = mac_time(attr_values[0].t);
	lastmodified_time = mac_time(attr_values[1].t);
	type_code = attr_values[2].i;
	creator_code = attr_values[3].i;

	memset(header, 0, sizeof(header));

	/* place filename */
	basename = filename;
	for (i = 0; filename[i]; i++)
	{
		if ((filename[i] == ':') || (filename[i] == '/'))
			basename = filename + 1;
	}
	len = MIN(&filename[i] - basename, 63);
	memcpy(&header[2], filename, len);
	header[1] = len;

	place_integer_be(header,  65, 4, type_code);
	place_integer_be(header,  69, 4, creator_code);
	place_integer_be(header,  83, 4, data_fork ? data_fork->size : 0);
	place_integer_be(header,  87, 4, resource_fork ? resource_fork->size : 0);
	place_integer_be(header,  91, 4, creation_time);
	place_integer_be(header,  95, 4, lastmodified_time);
	place_integer_be(header, 122, 1, 0x81);
	place_integer_be(header, 123, 1, 0x81);
	
	stream_write(destf, header, sizeof(header));
	
	if (data_fork)
	{
		err = img_module(image)->read_file(image, filename, "", destf);
		if (err)
			return err;
	}
	
	if (resource_fork)
	{
		err = img_module(image)->read_file(image, filename, "RESOURCE_FORK", destf);
		if (err)
			return err;
	}

	return IMGTOOLERR_SUCCESS;
}



void filter_macbinary_getinfo(UINT32 state, union filterinfo *info)
{
	switch(state)
	{
		case FILTINFO_STR_NAME:			info->s = "macbinary"; break;
		case FILTINFO_STR_HUMANNAME:	info->s = "MacBinary"; break;
		case FILTINFO_STR_EXTENSION:	info->s = "bin"; break;
		case FILTINFO_PTR_READFILE:		info->read_file = macbinary_readfile; break;
	}
}
