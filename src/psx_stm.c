#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if (HAVE_DECL_S_IRGRP == 0)
#include <sf_unistd.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* File stream based on SF_STREAM */

typedef struct PSF_POSIX_FILE_STREAM
{	SF_VIRTUAL_IO_EX *vt ;
	unsigned long ref_count ;
	int filedes ;
} PSF_POSIX_FILE_STREAM ;

static PSF_POSIX_FILE_STREAM *psf_file_stream_new (void) ;
static void psf_file_stream_delete (PSF_POSIX_FILE_STREAM *fs) ;
static int psf_file_stream_is_opened (PSF_POSIX_FILE_STREAM *fs) ;

static sf_count_t psf_file_stream_get_filelen (void *user_data) ;
static sf_count_t psf_file_stream_seek (sf_count_t offset, int whence, void *user_data) ;
static sf_count_t psf_file_stream_read (void *ptr, sf_count_t count, void *user_data) ;
static sf_count_t psf_file_stream_write (const void *ptr, sf_count_t count, void *user_data) ;
static sf_count_t psf_file_stream_tell (void *user_data) ;
static int psf_file_stream_truncate (void *user_data, sf_count_t len) ;
static int psf_file_stream_is_pipe (void *user_data) ;
static int psf_file_stream_sync (void *user_data) ;
static unsigned long psf_file_stream_ref (void *user_data) ;
static unsigned long psf_file_stream_unref (void *user_data) ;
static unsigned long psf_file_stream_no_close_unref (void *user_data) ;


/* SF_FILE_STREAM functions */

int psf_file_stream_open_posix (const char *path, int mode, SF_STREAM **stream)
{	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	PSF_POSIX_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED ;


	int omode = 0 ;
	int oflag = 0 ;

	switch (mode)
	{	case SFM_READ :
				oflag = O_RDONLY | O_BINARY ;
				omode = 0 ;
				break ;

		case SFM_WRITE :
				oflag = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;
				break ;

		case SFM_RDWR :
				oflag = O_RDWR | O_CREAT | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;
				break ;

		default :
				return - SFE_BAD_OPEN_MODE ;
				break ;
		} ;

	int fd = -1 ;

	if (mode == 0)
		fd = open (path, oflag) ;
	else
		fd = open (path, oflag, omode) ;

	if (fd < 0)
	{
		psf_file_stream_delete (fs) ;
		return SFE_BAD_OPEN_MODE ;
		}

	fs->filedes = fd ;

	fs->ref_count++ ;
	*stream = (SF_STREAM *) fs ;

	return SFE_NO_ERROR ;
}

#ifdef _WIN32

int psf_file_stream_open_posixW (const wchar_t *path, int mode, SF_STREAM **stream)
{	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	PSF_POSIX_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED ;


	int omode = 0 ;
	int oflag = 0 ;

	switch (mode)
	{	case SFM_READ :
				oflag = O_RDONLY | O_BINARY ;
				omode = 0 ;
				break ;

		case SFM_WRITE :
				oflag = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;
				break ;

		case SFM_RDWR :
				oflag = O_RDWR | O_CREAT | O_BINARY ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;
				break ;

		default :
				return - SFE_BAD_OPEN_MODE ;
				break ;
		} ;

	int fd = -1 ;

	if (mode == 0)
		fd = _wopen ((const wchar_t *) path, oflag) ;
	else
		fd = _wopen ((const wchar_t *) path, oflag, mode) ;

	if (fd < 0)
	{
		psf_file_stream_delete (fs) ;
		return SFE_BAD_OPEN_MODE ;
		}

	fs->filedes = fd ;

	fs->ref_count++ ;
	*stream = (SF_STREAM *) fs ;

	return SFE_NO_ERROR ;
}

#endif

int psf_file_stream_open_stdio_posix (int mode, SF_STREAM **stream)
{
	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	int	error = SFE_NO_ERROR ;

	switch (mode)
	{	case SFM_RDWR:
			error = SFE_OPEN_PIPE_RDWR ;
			break ;

		case SFM_READ:
			error = psf_file_stream_open_fd_posix (0, SF_TRUE, stream) ;
			break ;

		case SFM_WRITE:
			error = psf_file_stream_open_fd_posix (1, SF_TRUE, stream) ;
			break ;

		default:
			error = SFE_BAD_OPEN_MODE ;
			break ;
		}

	return error ;
}

int psf_file_stream_open_fd_posix (int fd, int do_not_close_descriptor, SF_STREAM **stream)
{	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	PSF_POSIX_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED ;

	fs->filedes = fd ;

	fs->ref_count++ ;
	if (do_not_close_descriptor == SF_TRUE)
		fs->vt->unref = psf_file_stream_no_close_unref ;

	*stream = (SF_STREAM *) fs ;

	return SFE_NO_ERROR ;
}

static PSF_POSIX_FILE_STREAM *psf_file_stream_new (void)
{	PSF_POSIX_FILE_STREAM *fs = calloc (1, sizeof (PSF_POSIX_FILE_STREAM)) ;
	if (fs == NULL)
		return NULL ;

	fs->vt = calloc (1, sizeof (SF_VIRTUAL_IO_EX)) ;
	if (fs->vt == NULL)
	{	psf_file_stream_delete (fs) ;
		return NULL ;
		}
	fs->vt->get_filelen	= psf_file_stream_get_filelen ;
	fs->vt->seek		= psf_file_stream_seek ;
	fs->vt->read		= psf_file_stream_read ;
	fs->vt->write		= psf_file_stream_write ;
	fs->vt->tell		= psf_file_stream_tell ;
	fs->vt->truncate	= psf_file_stream_truncate ;
	fs->vt->sync		= psf_file_stream_sync ;
	fs->vt->is_pipe		= psf_file_stream_is_pipe ;
	fs->vt->ref			= psf_file_stream_ref ;
	fs->vt->unref		= psf_file_stream_unref ;

	fs->filedes	= -1 ;

	return fs ;
}

static void psf_file_stream_delete (PSF_POSIX_FILE_STREAM *fs)
{	if (fs)
	{
		if (fs->vt)
		{	free (fs->vt) ;
			fs->vt = NULL ;
			}

		if (fs->filedes > -1)
		{	close (fs->filedes) ;
			fs->filedes = -1 ;
			}

		free (fs) ;
		}
}

int psf_file_stream_is_opened (PSF_POSIX_FILE_STREAM *fs)
{	if (!fs || !fs->vt || fs->filedes == -1)
		return SF_FALSE ;
	else
		return SF_TRUE ;
}

static sf_count_t psf_file_stream_get_filelen (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return (sf_count_t) -1 ;

#if (SIZEOF_OFF_T == 4 && SIZEOF_SF_COUNT_T == 8 && defined (HAVE_FSTAT64))
	struct stat64 statbuf ;

	if (fstat64 (fs->filedes, &statbuf) == -1)
		return (sf_count_t) -1 ;

	return statbuf.st_size ;
#elif defined _WIN32
	struct _stat64 statbuf ;

	if (_fstat64 (fs->filedes, &statbuf) == -1)
		return (sf_count_t) -1 ;

	return statbuf.st_size ;
#else
	struct stat statbuf ;

	if (fstat (fs->filedes, &statbuf) == -1)
		return (sf_count_t) -1 ;

	return statbuf.st_size ;
#endif
}

static sf_count_t psf_file_stream_seek (sf_count_t offset, int whence, void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	sf_count_t new_position = -1 ;

#ifdef _WIN32
	new_position = _lseeki64 (fs->filedes, offset, whence) ;
#else
	new_position = lseek (fs->filedes, offset, whence) ;
#endif

	return new_position ;
}

static sf_count_t psf_file_stream_read (void *ptr, sf_count_t count, void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return 0 ;

	if (count <= 0)
		return 0 ;

	sf_count_t total = 0 ;
	size_t bytes_to_read = 	0 ;
	ssize_t bytes_read = 0 ;

	do
	{	bytes_to_read = count - total < SENSIBLE_SIZE ? (size_t) count : SENSIBLE_SIZE ;
		bytes_read = read (fs->filedes, ((char*) ptr) + total, bytes_to_read) ;

		if (bytes_read == -1)
			return -1 ;
		else if (bytes_read == 0)
			return total ;
		else
			total += (sf_count_t) bytes_read ;

		} while (total != count) ;

	return total ;
}

static sf_count_t psf_file_stream_write (const void *ptr, sf_count_t count, void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	if (count <= 0)
		return 0 ;

	sf_count_t total = 0 ;
	size_t bytes_to_write = 0 ;
	ssize_t bytes_written = 0 ;

	do
	{	bytes_to_write = count - total < SENSIBLE_SIZE ? (size_t) count : SENSIBLE_SIZE ;
		bytes_written = write (fs->filedes, ptr, bytes_to_write) ;
		if (bytes_written < 0)
			return -1 ;
		else if (bytes_written == 0)
			break ;
		else
			total += (sf_count_t) bytes_written ;

		} while (total != count) ;

	return total ;
}

static sf_count_t psf_file_stream_tell (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	sf_count_t pos = -1 ;

#ifdef _WIN32
	pos = _lseeki64 (fs->filedes, 0, SEEK_CUR) ;
#else
	pos = lseek (fs->filedes, 0, SEEK_CUR) ;
#endif

	return pos ;
}

static int psf_file_stream_truncate (void *user_data, sf_count_t len)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	int retval = -1 ;

	if (len < 0)
		return -1 ;

	if ((sizeof (off_t) < sizeof (sf_count_t)) && len > 0x7FFFFFFF)
		return -1 ;

#ifdef _WIN32
	retval = _chsize_s (fs->filedes, len) ;
#else
	retval = ftruncate (fs->filedes, len) ;
#endif

	return retval ;
}

static int psf_file_stream_is_pipe (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return SF_FALSE ;

	struct stat statbuf ;
	if (fstat (fs->filedes, &statbuf) == 0)
	{
		if (S_ISFIFO (statbuf.st_mode) || S_ISSOCK (statbuf.st_mode))
		{	return SF_TRUE ;
			}
		}

	return SF_FALSE ;
}

static int psf_file_stream_sync (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return -1 ;

	int retval = -1 ;

#ifdef _WIN32
	retval = _commit (fs->filedes) ;
#else
	retval = fsync (fs->filedes) ;
#endif

	return retval ;
}

static unsigned long psf_file_stream_ref (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return 0 ;

	fs->ref_count++ ;

	return fs->ref_count ;
}

static unsigned long psf_file_stream_unref (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return 0 ;

	if (fs->ref_count > 0)
		fs->ref_count-- ;

	unsigned long ref_count = fs->ref_count ;

	if (ref_count == 0)
		psf_file_stream_delete (fs) ;

	return ref_count ;
}

static unsigned long psf_file_stream_no_close_unref (void *user_data)
{	PSF_POSIX_FILE_STREAM *fs = (PSF_POSIX_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened (fs) == SF_FALSE)
		return 0 ;

	if (fs->ref_count > 0)
		fs->ref_count-- ;

	unsigned long ref_count = fs->ref_count ;

	if (ref_count == 0)
	{	fs->filedes = -1 ;
		psf_file_stream_delete (fs) ;
		}

	return ref_count ;
}

void psf_posix_log_syserr (char *syserr, size_t len)
{	snprintf (syserr, len, "System error : %s.", strerror (errno)) ;
}
