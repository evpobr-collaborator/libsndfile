#include "common.h"

#if USE_WINDOWS_API == 1

#include <windows.h>

#include <io.h>

/* File stream based on SF_STREAM */

typedef struct PSF_WIN_FILE_STREAM
{	SF_VIRTUAL_IO_EX *vt ;
	unsigned long ref_count ;
	HANDLE handle ;
	int fd;
	int do_not_close_descriptor ;
} PSF_WIN_FILE_STREAM ;

static PSF_WIN_FILE_STREAM *psf_file_stream_new (void) ;
static void psf_file_stream_delete (PSF_WIN_FILE_STREAM *fs) ;
static int psf_file_stream_is_opened (PSF_WIN_FILE_STREAM *fs) ;

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


/* SF_FILE_STREAM functions */

int psf_file_stream_open_win32 (const char *path, int mode, SF_STREAM **stream)
{	DWORD dwDesiredAccess ;
	DWORD dwShareMode ;
	DWORD dwCreationDistribution ;
	HANDLE handle ;

	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	PSF_WIN_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED ;

	switch (mode)
	{	case SFM_READ :
				dwDesiredAccess = GENERIC_READ ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = OPEN_EXISTING ;
				break ;

		case SFM_WRITE :
				dwDesiredAccess = GENERIC_WRITE ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = CREATE_ALWAYS ;
				break ;

		case SFM_RDWR :
				dwDesiredAccess = GENERIC_READ | GENERIC_WRITE ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = OPEN_ALWAYS ;
				break ;

		default :
				psf_file_stream_delete (fs) ;
				return SFE_BAD_OPEN_MODE ;
		} ;

		handle = CreateFileA (
				path,						/* pointer to name of the file */
				dwDesiredAccess,			/* access (read-write) mode */
				dwShareMode,				/* share mode */
				0,							/* pointer to security attributes */
				dwCreationDistribution,		/* how to create */
				FILE_ATTRIBUTE_NORMAL,		/* file attributes (could use FILE_FLAG_SEQUENTIAL_SCAN) */
				NULL						/* handle to file with attributes to copy */
				) ;

	if (handle == INVALID_HANDLE_VALUE) {
		psf_file_stream_delete (fs) ;
		return SFE_BAD_FILE ;
		}

	fs->handle = handle ;

	fs->ref_count++ ;
	*stream = (SF_STREAM *) fs ;

	return SFE_NO_ERROR ;	
}

int psf_file_stream_open_win32W (const wchar_t *path, int mode, SF_STREAM **stream)
{	DWORD dwDesiredAccess ;
	DWORD dwShareMode ;
	DWORD dwCreationDistribution ;
	HANDLE handle ;

	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL ;

	PSF_WIN_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED ;

	switch (mode)
	{	case SFM_READ :
				dwDesiredAccess = GENERIC_READ ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = OPEN_EXISTING ;
				break ;

		case SFM_WRITE :
				dwDesiredAccess = GENERIC_WRITE ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = CREATE_ALWAYS ;
				break ;

		case SFM_RDWR :
				dwDesiredAccess = GENERIC_READ | GENERIC_WRITE ;
				dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE ;
				dwCreationDistribution = OPEN_ALWAYS ;
				break ;

		default :
				psf_file_stream_delete (fs) ;
				return SFE_BAD_OPEN_MODE ;
		} ;

		handle = CreateFileW (
				path,						/* pointer to name of the file */
				dwDesiredAccess,			/* access (read-write) mode */
				dwShareMode,				/* share mode */
				0,							/* pointer to security attributes */
				dwCreationDistribution,		/* how to create */
				FILE_ATTRIBUTE_NORMAL,		/* file attributes (could use FILE_FLAG_SEQUENTIAL_SCAN) */
				NULL						/* handle to file with attributes to copy */
				) ;

	if (handle == INVALID_HANDLE_VALUE) {
		psf_file_stream_delete (fs) ;
		return SFE_BAD_FILE ;
		}

	fs->handle = handle ;

	fs->ref_count++ ;
	*stream = (SF_STREAM *) fs ;

	return SFE_NO_ERROR ;	
}

int psf_file_stream_open_stdio_win32 (int mode, SF_STREAM **stream)
{
	if (stream == NULL)
		return SFE_BAD_FILE_PTR ;

	*stream = NULL;

	PSF_WIN_FILE_STREAM *fs = psf_file_stream_new () ;
	if (fs == NULL)
		return SFE_MALLOC_FAILED;

	int	error = SFE_NO_ERROR ;

	switch (mode)
	{
	case SFM_RDWR:
		error = SFE_OPEN_PIPE_RDWR;
		break;

	case SFM_READ:
		fs->handle = GetStdHandle(STD_INPUT_HANDLE);
		break;

	case SFM_WRITE:
		fs->handle = GetStdHandle(STD_OUTPUT_HANDLE);
		break;

	default:
		error = SFE_BAD_OPEN_MODE;
		break;
	};

	if (error == SFE_NO_ERROR) {
		fs->ref_count++;
		*stream = (SF_STREAM *)fs;
	}
	else {
		psf_file_stream_delete(fs);
	}

	return error;
}

int psf_file_stream_open_fd_win32(int fd, int do_not_close_descriptor, SF_STREAM **stream)
{
	if (stream == NULL)
		return SFE_BAD_FILE_PTR;

	*stream = NULL;

	PSF_WIN_FILE_STREAM *fs = psf_file_stream_new();
	if (fs == NULL)
		return SFE_MALLOC_FAILED;

	intptr_t osfhandle = _get_osfhandle(fd);
	fs->handle = (HANDLE)osfhandle;
	fs->fd = fd;
	fs->do_not_close_descriptor = do_not_close_descriptor ;

	fs->ref_count++;

	*stream = (SF_STREAM *)fs;

	return SFE_NO_ERROR;
}

static PSF_WIN_FILE_STREAM *psf_file_stream_new(void)
{	PSF_WIN_FILE_STREAM *fs = calloc (1, sizeof(PSF_WIN_FILE_STREAM)) ;
	if (fs == NULL)
		return NULL ;

	fs->vt = calloc (1, sizeof(SF_VIRTUAL_IO_EX));
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

	fs->handle = INVALID_HANDLE_VALUE;
	fs->fd = -1;

	return fs;
}

static void psf_file_stream_delete (PSF_WIN_FILE_STREAM *fs)
{	if (fs)
	{
		if (fs->vt)
		{	free (fs->vt) ;
			fs->vt = NULL ;
			}

		// Stream was opened with file descriptor, call close()
		if (fs->fd > -1)
		{
			close(fs->fd);
		}
		else if (fs->handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle (fs->handle) ;
			fs->handle = INVALID_HANDLE_VALUE;
		}

		free (fs) ;
		}
}

int psf_file_stream_is_opened (PSF_WIN_FILE_STREAM *fs)
{	if (!fs || !fs->vt || fs->handle == INVALID_HANDLE_VALUE)
		return SF_FALSE ;
	else
		return TRUE ;
}

static sf_count_t psf_file_stream_get_filelen (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	sf_count_t filelen ;
	DWORD dwFileSizeLow, dwFileSizeHigh, dwError = NO_ERROR ;

	dwFileSizeLow = GetFileSize (fs->handle, &dwFileSizeHigh) ;

	if (dwFileSizeLow == 0xFFFFFFFF)
		dwError = GetLastError () ;

	if (dwError != NO_ERROR)
		return (sf_count_t) -1 ;

	filelen = dwFileSizeLow + ((__int64) dwFileSizeHigh << 32) ;

	return filelen ;
}

static sf_count_t psf_file_stream_seek (sf_count_t offset, int whence, void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	DWORD dwMoveMethod ;

	switch (whence)
	{	case SEEK_SET :
				dwMoveMethod = FILE_BEGIN ;
				break ;

		case SEEK_END :
				dwMoveMethod = FILE_END ;
				break ;

		default :
				dwMoveMethod = FILE_CURRENT ;
				break ;
		} ;

	LONG lDistanceToMove = (DWORD) (offset & 0xFFFFFFFF) ;
	LONG lDistanceToMoveHigh = (DWORD) ((offset >> 32) & 0xFFFFFFFF) ;

	DWORD dwResult = SetFilePointer (fs->handle, lDistanceToMove, &lDistanceToMoveHigh, dwMoveMethod) ;

	DWORD dwError = NO_ERROR ; 
	if (dwResult == 0xFFFFFFFF)
		dwError = GetLastError () ;
	else
		dwError = NO_ERROR ;

	if (dwError != NO_ERROR)
		return -1 ;

	sf_count_t new_position = dwResult + ((__int64) lDistanceToMoveHigh << 32) ;

	return new_position ;
}

static sf_count_t psf_file_stream_read (void *ptr, sf_count_t count, void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return 0 ;

	if (count <= 0)
		return 0 ;

	sf_count_t total = 0 ;
	DWORD dwNumberOfBytesToRead = 0 ;
	DWORD dwNumberOfBytesRead = 0;

	do 
	{	dwNumberOfBytesToRead = count - total < SENSIBLE_SIZE ? (DWORD) count : SENSIBLE_SIZE ;
		BOOL fRet = ReadFile (fs->handle, ((char*) ptr) + total, dwNumberOfBytesToRead, &dwNumberOfBytesRead, 0) ;

		if ((fRet == FALSE) || (dwNumberOfBytesRead == 0))
			break ;
		else
			total += (sf_count_t) dwNumberOfBytesRead ;

		} while (total != count) ;

	return total ;
}

static sf_count_t psf_file_stream_write (const void *ptr, sf_count_t count, void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	if (count <= 0)
		return 0 ;

	sf_count_t total = 0 ;
	DWORD dwNumberOfBytesToWrite = 0 ;
	DWORD dwNumberOfBytesWritten = 0 ;

	do
	{	
		dwNumberOfBytesToWrite = count - total < SENSIBLE_SIZE ? count : SENSIBLE_SIZE;
		BOOL fRet = WriteFile (fs->handle, ((const char*) ptr) + total, dwNumberOfBytesToWrite, &dwNumberOfBytesWritten, 0) ;
		if ((fRet == FALSE) || (dwNumberOfBytesWritten == 0))
			break ;
		else
			total += (sf_count_t) dwNumberOfBytesWritten ;

		} while (total != count) ;

	return total ;
}

static sf_count_t psf_file_stream_tell (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	LONG lDistanceToMoveLow = 0 ;
	LONG lDistanceToMoveHigh = 0 ;
	DWORD dwResult = SetFilePointer (fs->handle, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_CURRENT) ;
	if (dwResult == 0xFFFFFFFF)
		return -1 ;

	sf_count_t pos = (dwResult + ((__int64) lDistanceToMoveHigh << 32)) ;

	return pos ;
}

static int psf_file_stream_truncate (void *user_data, sf_count_t len)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return 1 ;


	int retval = 0 ;
	DWORD dwResult, dwError = NO_ERROR ;

	/* This implementation trashes the current file position.
	** should it save and restore it? what if the current position is past
	** the new end of file?
	*/

	LONG lDistanceToMoveLow = (DWORD) (len & 0xFFFFFFFF) ;
	LONG lDistanceToMoveHigh = (DWORD) ((len >> 32) & 0xFFFFFFFF) ;

	dwResult = SetFilePointer (fs->handle, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_BEGIN) ;

	if (dwResult == 0xFFFFFFFF)
	{	retval = -1 ;
	}
	else
	{	/* Note: when SetEndOfFile is used to extend a file, the contents of the
		** new portion of the file is undefined. This is unlike chsize(),
		** which guarantees that the new portion of the file will be zeroed.
		** Not sure if this is important or not.
		*/
		if (SetEndOfFile (fs->handle) == 0)
			retval = -1 ;
		} ;

	return retval ;
}

static int psf_file_stream_is_pipe (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return SF_FALSE ;

	if (GetFileType (fs->handle) == FILE_TYPE_DISK)
		return SF_FALSE ;
	else
		return SF_TRUE ;
}

static int psf_file_stream_sync (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) -1 ;

	FlushFileBuffers (fs->handle) ;

	return 0 ;
}

static unsigned long psf_file_stream_ref (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	return ++fs->ref_count ;
}

static unsigned long psf_file_stream_unref (void *user_data)
{	PSF_WIN_FILE_STREAM *fs = (PSF_WIN_FILE_STREAM *) user_data ;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t) 0 ;

	if (fs->ref_count > 0) {
		fs->ref_count-- ;
	}

	unsigned long ref_count = fs->ref_count ;

	if (ref_count == 0)
	{
		if (fs->do_not_close_descriptor)
		{
			fs->fd = -1;
			fs->handle = INVALID_HANDLE_VALUE ;
		}

		psf_file_stream_delete (fs) ;
	}

	return ref_count ;
}

#endif
