/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

/*
 * $Logfile: /Freespace2/code/Sound/acm.h $
 * $Revision: 2.0 $
 * $Date: 2002-06-03 04:02:29 $
 * $Author: penguin $
 *
 * Header file for interface to Audio Compression Manager functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2002/05/02 18:03:13  mharris
 * Initial checkin - converted filenames and includes to lower case
 *
 * 
 * 2     10/07/98 10:53a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 5     2/18/98 5:49p Lawrance
 * Even if the ADPCM codec is unavailable, allow game to continue.
 * 
 * 4     11/28/97 2:09p Lawrance
 * Overhaul how ADPCM conversion works... use much less memory... safer
 * too.
 * 
 * 3     11/22/97 11:32p Lawrance
 * decompress ADPCM data into 8 bit (not 16bit) for regular sounds (ie not
 * music)
 * 
 * 2     5/29/97 12:03p Lawrance
 * creation of file to hold AudioCompressionManager specific code
 *
 * $NoKeywords: $
 */

#ifndef __FREESPACE_ACM_H__
#define __FREESPACE_ACM_H__

#include <mmreg.h>
#include <msacm.h>
#include "pstypes.h"

int	ACM_convert_ADPCM_to_PCM(WAVEFORMATEX *pwfxSrc, ubyte *src, int src_len, ubyte **dest, int max_dest_bytes, int *dest_len, unsigned int *src_bytes_used, unsigned short dest_bps=16);
int	ACM_init();
void	ACM_close();
int	ACM_is_inited();


int ACM_stream_open(WAVEFORMATEX *pwfxSrc, WAVEFORMATEX *pwfxDest, void **stream, int dest_bps=16);
int ACM_stream_close(void *stream);
int ACM_query_source_size(void *stream, int dest_len);
int ACM_query_dest_size(void *stream, int src_len);

int ACM_convert(void *stream, ubyte *src, int src_len, ubyte *dest, int max_dest_bytes, unsigned int *dest_len, unsigned int *src_bytes_used);


#endif /* __ACM_H__ */
