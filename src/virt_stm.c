#include "common.h"

#include <assert.h>

typedef struct PSF_VIRTUAL_STREAM
{
	SF_VIRTUAL_IO_EX *vt;
	unsigned long ref_count;
	void *user_data;
	SF_VIRTUAL_IO *sfvirtual;
} PSF_VIRTUAL_STREAM;

static PSF_VIRTUAL_STREAM *psf_file_stream_new(void);
static void psf_file_stream_delete(PSF_VIRTUAL_STREAM *fs);
static int psf_file_stream_is_opened(PSF_VIRTUAL_STREAM *fs);

static sf_count_t psf_file_stream_get_filelen(void *user_data);
static sf_count_t psf_file_stream_seek(sf_count_t offset, int whence, void *user_data);
static sf_count_t psf_file_stream_read(void *ptr, sf_count_t count, void *user_data);
static sf_count_t psf_file_stream_write(const void *ptr, sf_count_t count, void *user_data);
static sf_count_t psf_file_stream_tell(void *user_data);
static int psf_file_stream_truncate(void *user_data, sf_count_t len);
static int psf_file_stream_is_pipe(void *user_data);
static int psf_file_stream_sync(void *user_data);
static unsigned long psf_file_stream_ref(void *user_data);
static unsigned long psf_file_stream_no_close_unref(void *user_data);

int psf_stream_open_virtual(SF_VIRTUAL_IO *sfvirtual, int mode, void *user_data, SF_STREAM **stream)
{
	if (stream == NULL)
		return SFE_BAD_FILE_PTR;

	*stream = NULL;

	/* Make sure we have a valid set ot virtual pointers. */
	if (sfvirtual->get_filelen == NULL || sfvirtual->seek == NULL || sfvirtual->tell == NULL)
	{
		return SFE_BAD_VIRTUAL_IO;
	};

	if ((mode == SFM_READ || mode == SFM_RDWR) && sfvirtual->read == NULL)
	{
		return SFE_BAD_VIRTUAL_IO;
	};

	if ((mode == SFM_WRITE || mode == SFM_RDWR) && sfvirtual->write == NULL)
	{
		return SFE_BAD_VIRTUAL_IO;
	};

	PSF_VIRTUAL_STREAM *vs = psf_file_stream_new();
	if (!vs)
	{
		return SFE_MALLOC_FAILED;
	}

	vs->user_data = user_data;
	vs->sfvirtual = sfvirtual;

	vs->ref_count++;

	*stream = (SF_STREAM *)vs;

	return SFE_NO_ERROR;
}

PSF_VIRTUAL_STREAM *psf_file_stream_new()
{
	PSF_VIRTUAL_STREAM *vs = calloc(1, sizeof(PSF_VIRTUAL_STREAM));
	if (vs == NULL)
		return NULL;

	vs->vt = calloc(1, sizeof(SF_VIRTUAL_IO_EX));
	if (vs->vt == NULL)
	{
		psf_file_stream_delete(vs);
		return NULL;
	}

	vs->vt->get_filelen = psf_file_stream_get_filelen;
	vs->vt->seek = psf_file_stream_seek;
	vs->vt->read = psf_file_stream_read;
	vs->vt->write = psf_file_stream_write;
	vs->vt->tell = psf_file_stream_tell;
	vs->vt->truncate = psf_file_stream_truncate;
	vs->vt->sync = psf_file_stream_sync;
	vs->vt->is_pipe = psf_file_stream_is_pipe;
	vs->vt->ref = psf_file_stream_ref;
	vs->vt->unref = psf_file_stream_no_close_unref;

	return vs;
}

static void psf_file_stream_delete(PSF_VIRTUAL_STREAM *vs)
{
	if (vs)
	{
		if (vs->vt)
		{
			free(vs->vt);
			vs->vt = NULL;
		}

		vs->sfvirtual = NULL;
		vs->user_data = NULL;

		free(vs);
	}
}

int psf_file_stream_is_opened(PSF_VIRTUAL_STREAM *vs)
{
	if (!vs || !vs->vt || !vs->user_data || !vs->sfvirtual)
		return SF_FALSE;
	else
		return SF_TRUE;
}

static sf_count_t psf_file_stream_get_filelen(void *user_data)
{
	PSF_VIRTUAL_STREAM *vs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(vs) == SF_FALSE)
		return -1;

	assert(vs->sfvirtual->get_filelen != NULL);

	return vs->sfvirtual->get_filelen(vs->user_data);
}

static sf_count_t psf_file_stream_seek(sf_count_t offset, int whence, void *user_data)
{
	PSF_VIRTUAL_STREAM *vs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(vs) == SF_FALSE)
		return -1;

	assert(vs->sfvirtual->seek != NULL);

	return vs->sfvirtual->seek(offset, whence, vs->user_data);
}

static sf_count_t psf_file_stream_read(void *ptr, sf_count_t count, void *user_data)
{
	PSF_VIRTUAL_STREAM *vs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(vs) == SF_FALSE)
		return 0;

	assert(vs->sfvirtual->read != NULL);

	return vs->sfvirtual->read(ptr, count, vs->user_data);
}

static sf_count_t psf_file_stream_write(const void *ptr, sf_count_t count, void *user_data)
{
	PSF_VIRTUAL_STREAM *vs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(vs) == SF_FALSE)
		return 0;

	assert(vs->sfvirtual->write != NULL);

	return vs->sfvirtual->write(ptr, count, vs->user_data);
}

static sf_count_t psf_file_stream_tell(void *user_data)
{
	PSF_VIRTUAL_STREAM *vs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(vs) == SF_FALSE)
		return -1;

	assert(vs->sfvirtual->tell != NULL);

	return vs->sfvirtual->tell(vs->user_data);
}

static int psf_file_stream_truncate(void * UNUSED (user_data), sf_count_t UNUSED (len))
{
	return -1;
}
static int psf_file_stream_is_pipe(void * UNUSED (user_data))
{
	return SF_FALSE;
}

static int psf_file_stream_sync(void * UNUSED (user_data))
{
	return 0;
}

static unsigned long psf_file_stream_ref(void *user_data)
{
	PSF_VIRTUAL_STREAM *fs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return (sf_count_t)0;

	return ++fs->ref_count;
}

static unsigned long psf_file_stream_no_close_unref(void *user_data)
{
	PSF_VIRTUAL_STREAM *fs = (PSF_VIRTUAL_STREAM *)user_data;
	if (psf_file_stream_is_opened(fs) == SF_FALSE)
		return 0;

	if (fs->ref_count > 0) {
		fs->ref_count--;
	}

	unsigned long ref_count = fs->ref_count;

	if (ref_count == 0) {
		fs->user_data = NULL;
		psf_file_stream_delete(fs);
	}

	return ref_count;
}
