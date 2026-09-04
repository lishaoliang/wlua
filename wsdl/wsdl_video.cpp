#include "wsdl_video.h"
#include "klbmem/klb_mem.h"


wsdl_video_t* wsdl_video_create()
{
    wsdl_video_t* p_video = KLB_MALLOCZ(wsdl_video_t, 1, 0);



    return p_video;
}

void wsdl_video_destroy(wsdl_video_t* p_video)
{
    KLB_FREE(p_video);
}

int wsdl_video_decode(wsdl_video_t* p_video, int chnn, int sidx, klb_buf_t* p_media, AVFrame** p_out)
{
    (void)p_video;
    (void)chnn;
    (void)sidx;
    (void)p_media;
    (void)p_out;
    return 1;
}

// end
