#ifndef __WSDL_STREAM_YUV_H__
#define __WSDL_STREAM_YUV_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_streamyuv_t_
{
    klb_buf_t*  p_data;
    int         size;
    int         index;

    int         w;
    int         h;
    int         page;
}wsdl_streamyuv_t;


// YUV420P
wsdl_streamyuv_t* wsdl_streamyuv_load(const char* p_path, int w, int h);
void wsdl_streamyuv_destroy(wsdl_streamyuv_t* p_stream);

int wsdl_streamyuv_size(wsdl_streamyuv_t* p_stream);


int wsdl_streamyuv_read_next(wsdl_streamyuv_t* p_stream, char** p_y, char** p_u, char** p_v, int* p_w, int* p_h);



#if defined(__cplusplus)
}
#endif

#endif // __WSDL_STREAM_YUV_H__
//end