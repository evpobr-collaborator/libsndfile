/*
** Copyright (C) 2002-2014 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2003 Ross Bencina <rbencina@iprimus.com.au>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
**	The file is split into three sections as follows:
**		- The top section (USE_WINDOWS_API == 0) for Linux, Unix and MacOSX
**			systems (including Cygwin).
**		- The middle section (USE_WINDOWS_API == 1) for microsoft windows
**			(including MinGW) using the native windows API.
**		- A legacy windows section which attempted to work around grevious
**			bugs in microsoft's POSIX implementation.
*/

/*
**	The header file sfconfig.h MUST be included before the others to ensure
**	that large file support is enabled correctly on Unix systems.
*/

#include "sfconfig.h"

#include <stdio.h>
#include <stdlib.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if (HAVE_DECL_S_IRGRP == 0)
#include <sf_unistd.h>
#endif

#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "sndfile.h"
#include "common.h"

#if USE_WINDOWS_API == 1
#include <windows.h>
#include <io.h>
#endif

/*
**	Neat solution to the Win32/OS2 binary file flage requirement.
**	If O_BINARY isn't already defined by the inclusion of the system
**	headers, set it to zero.
*/
#ifndef O_BINARY
#define O_BINARY 0
#endif

int
psf_fopen (SF_PRIVATE *psf)
{
	SF_STREAM *stream = NULL ;
#ifdef _WIN32
	if (psf->file.use_wchar)
		psf->error = psf_file_stream_openW (psf->file.path.wc, psf->file.mode, &stream) ;
	else
		psf->error = psf_file_stream_open (psf->file.path.c, psf->file.mode, &stream) ;
#else
	psf->error = psf_file_stream_open (psf->file.path.c, psf->file.mode, &stream) ;
#endif
	if (psf->error == SFE_BAD_OPEN_MODE)
	{	psf->error = SFE_SYSTEM ;
		psf_log_syserr (psf->syserr, SF_SYSERR_LEN) ;

		return psf->error ;
		}

	psf->file.stream = stream ;
	psf->file.virtual_io = SF_FALSE ;

	return psf->error ;
} /* psf_fopen */

int
psf_fclose (SF_PRIVATE *psf)
{	int retval = 0 ;

	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->unref)
		psf->file.stream->vt->unref (psf->file.stream) ;

	psf->file.stream = NULL ;

	return retval ;
} /* psf_fclose */

int
psf_file_valid (SF_PRIVATE *psf)
{
	return psf->file.stream ? SF_TRUE : SF_FALSE ;
} /* psf_set_file */

sf_count_t
psf_get_filelen (SF_PRIVATE *psf)
{	sf_count_t	filelen = -1 ;

	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->get_filelen)
		filelen = psf->file.stream->vt->get_filelen (psf->file.stream) ;

	switch (psf->file.mode)
	{	case SFM_WRITE :
			filelen = filelen - psf->fileoffset ;
			break ;

		case SFM_READ :
			if (psf->fileoffset > 0 && psf->filelength > 0)
				filelen = psf->filelength ;
			break ;

		case SFM_RDWR :
			/*
			** Cannot open embedded files SFM_RDWR so we don't need to
			** subtract psf->fileoffset. We already have the answer we
			** need.
			*/
			break ;

		default :
			/* Shouldn't be here, so return error. */
			filelen = -1 ;
		} ;

	return filelen ;
} /* psf_get_filelen */

void
psf_log_syserr (char *syserr, size_t len)
{
#if USE_WINDOWS_API == 0
	psf_posix_log_syserr (syserr, len) ;
#endif
} /* psf_log_syserr */

int
psf_set_stdio (SF_PRIVATE *psf)
{
	SF_STREAM *stream = NULL ;
	psf->error = psf_file_stream_open_stdio (psf->file.mode, &stream) ;
	if (psf->error == SFE_NO_ERROR)
	{
		psf->file.stream = stream ;
	}

	return psf->error ;
} /* psf_set_stdio */

void
psf_set_file (SF_PRIVATE *psf, int fd)
{
	SF_STREAM *stream = NULL ;

	int retval = psf_file_stream_open_fd (fd, SF_TRUE, &stream) ;
	psf->error = retval ;

	if (psf->error == SFE_NO_ERROR)
		psf->file.stream = stream ;

} /* psf_set_file */

sf_count_t
psf_fseek (SF_PRIVATE *psf, sf_count_t offset, int whence)
{	sf_count_t new_position = -1 ;

	if (whence == SEEK_SET)
		offset += psf->fileoffset ;

		if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->seek)
			new_position = psf->file.stream->vt->seek (offset, whence, psf->file.stream) ;

		if (new_position >= 0)
		{	assert (new_position >= psf->fileoffset) ;

			new_position -= psf->fileoffset ;
		}

		return new_position ;
} /* psf_fseek */

sf_count_t
psf_fread (void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;
	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->read)
		total = psf->file.stream->vt->read (ptr, items, psf->file.stream) ;

	if (total == -1)
	{
		psf->error = SFE_SYSTEM ;
		psf_log_syserr (psf->syserr, SF_SYSERR_LEN) ;

		return 0 ;
		}


	if (psf->is_pipe)
		psf->pipeoffset += total ;

	return total / bytes ;
} /* psf_fread */

sf_count_t
psf_fwrite (const void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;

	assert (ptr != NULL) ;
	assert (bytes >= 0) ;
	assert (items >= 0) ;
	assert (psf != NULL) ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->write)
			total = psf->file.stream->vt->write (ptr, items, psf->file.stream) ;

	if (total == -1)
	{
		psf->error = SFE_SYSTEM ;
		psf_log_syserr (psf->syserr, SF_SYSERR_LEN) ;

		return 0 ;
		}

	if (psf->is_pipe)
		psf->pipeoffset += total ;

	return total / bytes ;
} /* psf_fwrite */

sf_count_t
psf_ftell (SF_PRIVATE *psf)
{	assert (psf != NULL) ;

	sf_count_t pos = -1 ;

	if (psf->is_pipe)
		return psf->pipeoffset ;

	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->tell)
	{	pos = psf->file.stream->vt->tell (psf->file.stream) ;
		assert (pos >= psf->fileoffset) ;
		pos -= psf->fileoffset ;
		}

	return pos ;
} /* psf_ftell */

int
psf_is_pipe (SF_PRIVATE *psf)
{	int retval = SF_FALSE ;

	SF_STREAM *stream = (SF_STREAM *) psf->file.stream ;
		if (stream && stream->vt && stream->vt->is_pipe)
			return stream->vt->is_pipe (stream) ;

	return retval ;
} /* psf_is_pipe */

void
psf_fsync (SF_PRIVATE *psf)
{	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->sync)
		psf->file.stream->vt->sync (psf->file.stream) ;
} /* psf_fsync */


int
psf_ftruncate (SF_PRIVATE *psf, sf_count_t len)
{	if (psf->file.stream && psf->file.stream->vt && psf->file.stream->vt->truncate)
		return psf->file.stream->vt->truncate (psf->file.stream, len) ;
	else
		return -1 ;
} /* psf_ftruncate */

int psf_file_stream_open_fd (int fd, int do_not_close_descriptor, SF_STREAM **stream)
{
#if USE_WINDOWS_API == 1
	return psf_file_stream_open_fd_win32 (fd, do_not_close_descriptor, stream) ;
#else
	return psf_file_stream_open_fd_posix (fd, do_not_close_descriptor, stream) ;
#endif
}

int psf_file_stream_open (const char *path, int mode, SF_STREAM **stream)
{
#if USE_WINDOWS_API == 0
	return psf_file_stream_open_posix(path, mode, stream) ;
#else
	return psf_file_stream_open_win32(path, mode, stream) ;
#endif
}

#ifdef _WIN32

int psf_file_stream_openW (const wchar_t *path, int mode, SF_STREAM **stream)
{
#if USE_WINDOWS_API == 0
	return psf_file_stream_open_posixW(path, mode, stream) ;
#else
	return psf_file_stream_open_win32W(path, mode, stream) ;
#endif
}

#endif

int psf_file_stream_open_stdio (int mode, SF_STREAM **stream)
{
#if USE_WINDOWS_API == 1
	return psf_file_stream_open_stdio_win32 (mode, stream) ;
#else
	return psf_file_stream_open_stdio_posix (mode, stream) ;
#endif
}
