#ifndef __WSDL_VIDEO_YUV_H__
#define __WSDL_VIDEO_YUV_H__

#include "klb_type.h"
#include "klbutil/klb_rect.h"
#include "SDL.h"

#if defined(__cplusplus)
extern "C" {
#endif
    

#define WSDL_VIDEO_YUV_max          64


typedef struct wsdl_video_yuv_t_ wsdl_video_yuv_t;


// YUV视频
wsdl_video_yuv_t* wsdl_video_yuv_create(SDL_Renderer* p_render);
void wsdl_video_yuv_destroy(wsdl_video_yuv_t* p_video);


int wsdl_video_yuv_update(wsdl_video_yuv_t* p_video, int idx, int w, int h, char* p_y, char* p_u, char* p_v);
int wsdl_video_yuv_update_rect(wsdl_video_yuv_t* p_video, int idx, int chnn, const klb_rect_t* p_rect);

void wsdl_video_yuv_set_size(wsdl_video_yuv_t* p_video, int size);
int wsdl_video_yuv_size(wsdl_video_yuv_t* p_video);

SDL_Texture* wsdl_video_yuv_get(wsdl_video_yuv_t* p_video, int idx, klb_rect_t* p_out_dst);


#if defined(__cplusplus)
}
#endif

#endif // __WSDL_VIDEO_YUV_H__
//end
