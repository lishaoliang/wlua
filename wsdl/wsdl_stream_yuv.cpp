#include "wsdl_stream_yuv.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include <stdio.h>


static klb_buf_t* loadfile_streamyuv(const char* p_path, int w, int h, int* p_count)
{
    FILE* pf = fopen(p_path, "rb");

    if (NULL == pf)
    {
        return NULL;
    }

    int size = w * h + w * h / 2;

    fseek(pf, 0, SEEK_END);
    int filelen = ftell(pf);

    if (filelen < size)
    {
        fclose(pf);
        return NULL;
    }

    int count = (filelen + size - 1) / size;
    int len = size * count;

    klb_buf_t* p_buf = klb_buf_malloc(len, false);

    fseek(pf, 0, SEEK_SET);
    fread(p_buf->p_buf, 1, len, pf);

    p_buf->end = len;

    fclose(pf);

    if (NULL != p_count)
    {
        *p_count = count;
    }

    return p_buf;
}

wsdl_streamyuv_t* wsdl_streamyuv_load(const char* p_path, int w, int h)
{
    int size = 0;
    klb_buf_t* p_buf = loadfile_streamyuv(p_path, w, h, &size);

    if (NULL == p_buf)
    {
        return NULL;
    }

    wsdl_streamyuv_t* p_stream = KLB_MALLOCZ(wsdl_streamyuv_t, 1, 0);

    p_stream->p_data = p_buf;
    p_stream->size = size;
    p_stream->index = 0;

    p_stream->w = w;
    p_stream->h = h;
    p_stream->page = w * h + w *  h / 2;

    return p_stream;
}

void wsdl_streamyuv_destroy(wsdl_streamyuv_t* p_stream)
{
    KLB_FREE(p_stream->p_data);
    KLB_FREE(p_stream);
}

int wsdl_streamyuv_size(wsdl_streamyuv_t* p_stream)
{
    return p_stream->size;
}

int wsdl_streamyuv_read_next(wsdl_streamyuv_t* p_stream, char** p_y, char** p_u, char** p_v, int* p_w, int* p_h)
{
    if (p_stream->size <= 0)
    {
        return 1;
    }

    int idx = p_stream->index;
    int offset = p_stream->page * idx;

    char* ptr = p_stream->p_data->p_buf + offset;
    if (NULL != p_y) { *p_y = ptr; };

    ptr += p_stream->w * p_stream->h;
    if (NULL != p_u) { *p_u = ptr; };

    ptr += (p_stream->w * p_stream->h) / 4;
    if (NULL != p_v) { *p_v = ptr; };

    if (NULL != p_w) { *p_w = p_stream->w; };
    if (NULL != p_h) { *p_h = p_stream->h; };

    p_stream->index = ((idx + 1) < p_stream->size) ? (idx + 1) : 0;

    return 0;
}
