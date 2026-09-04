#include "wsdl_video_yuv.h"
#include "klbmem/klb_mem.h"


typedef struct wsdl_texure_yuv_t_
{
    SDL_Texture*    p_texture;
    int             w;
    int             h;

    int             chnn;

    klb_rect_t      dst;            ///< 显示目标区域: 屏幕区域
}wsdl_texure_yuv_t;


typedef struct wsdl_video_yuv_t_
{
    SDL_Renderer*       p_render;

    wsdl_texure_yuv_t   yuv[WSDL_VIDEO_YUV_max];

    int                 size;
}wsdl_video_yuv_t;


//////////////////////////////////////////////////////////////////////////
static void quit_texture_wsdl_video_yuv(wsdl_texure_yuv_t* p_yuv);


wsdl_video_yuv_t* wsdl_video_yuv_create(SDL_Renderer* p_render)
{
    wsdl_video_yuv_t* p_video = KLB_MALLOCZ(wsdl_video_yuv_t, 1, 0);

    p_video->p_render = p_render;
    p_video->size = 0;

    for (int i = 0; i < WSDL_VIDEO_YUV_max; i++)
    {
        p_video->yuv[i].chnn = -1;
    }

    return p_video;
}

void wsdl_video_yuv_destroy(wsdl_video_yuv_t* p_video)
{
    for (int i = 0; i < WSDL_VIDEO_YUV_max; i++)
    {
        quit_texture_wsdl_video_yuv(&p_video->yuv[i]);
    }

    KLB_FREE(p_video);
}

static void init_texture_wsdl_video_yuv(wsdl_texure_yuv_t* p_yuv, SDL_Renderer* p_render, int w, int h)
{
    assert(NULL == p_yuv->p_texture);

    p_yuv->p_texture = SDL_CreateTexture(p_render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);

    p_yuv->w = w;
    p_yuv->h = h;
}

static void quit_texture_wsdl_video_yuv(wsdl_texure_yuv_t* p_yuv)
{
    KLB_FREE_BY(p_yuv->p_texture, SDL_DestroyTexture);

    p_yuv->w = 0;
    p_yuv->h = 0;
}

int wsdl_video_yuv_update(wsdl_video_yuv_t* p_video, int idx, int w, int h, char* p_y, char* p_u, char* p_v)
{
    if (idx < 0 || idx >= WSDL_VIDEO_YUV_max)
    {
        return 1;
    }

    wsdl_texure_yuv_t* p_yuv = &p_video->yuv[idx];

    if (w != p_yuv->w || h != p_yuv->h)
    {
        quit_texture_wsdl_video_yuv(p_yuv);
        init_texture_wsdl_video_yuv(p_yuv, p_video->p_render, w, h);
    }

    int pitch_y = p_yuv->w;
    int pitch_u = p_yuv->w / 2;
    int pitch_v = p_yuv->w / 2;

    //SDL_Rect rect = { 0 };
    //rect.w = p_yuv->w;
    //rect.h = p_yuv->h;

    int ret = SDL_UpdateYUVTexture(p_yuv->p_texture, NULL, (const Uint8*)p_y, pitch_y, (const Uint8*)p_u, pitch_u, (const Uint8*)p_v, pitch_v);

    return ret;
}

int wsdl_video_yuv_update_rect(wsdl_video_yuv_t* p_video, int idx, int chnn, const klb_rect_t* p_rect)
{
    if (idx < 0 || idx >= WSDL_VIDEO_YUV_max)
    {
        return 1;
    }

    wsdl_texure_yuv_t* p_yuv = &p_video->yuv[idx];

    if (NULL != p_rect)
    {
        p_yuv->dst = *p_rect;
    }
    else
    {
        p_yuv->dst.x = 0;
        p_yuv->dst.y = 0;
        p_yuv->dst.w = 0;
        p_yuv->dst.h = 0;
    }

    p_yuv->chnn = chnn;

    return 0;
}

void wsdl_video_yuv_set_size(wsdl_video_yuv_t* p_video, int size)
{
    size = (size <= 0) ? 0 : size;
    size = (size >= WSDL_VIDEO_YUV_max) ? WSDL_VIDEO_YUV_max : size;

    p_video->size = size;
}

int wsdl_video_yuv_size(wsdl_video_yuv_t* p_video)
{
    return p_video->size;
}


SDL_Texture* wsdl_video_yuv_get(wsdl_video_yuv_t* p_video, int idx, klb_rect_t* p_out_dst)
{
    if (idx < 0 || idx >= WSDL_VIDEO_YUV_max)
    {
        return NULL;
    }

    wsdl_texure_yuv_t* p_yuv = &p_video->yuv[idx];

    if (p_yuv->chnn < 0)
    {
        return NULL;
    }

    if (NULL != p_out_dst)
    {
        *p_out_dst = p_yuv->dst;
    }

    return p_yuv->p_texture;
}
