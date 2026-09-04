#include "wsdl_wnd.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "wsdl_surface_canvas.h"
#include "wsdl_images.h"
#include "ft_raster.h"
#include "wsdl_video_yuv.h"
#include "wsdl_surface_canvas_opt.h"
#include "SDL.h"
#include <assert.h>


#define DIR_PATH_TMPIMAGE           "demores/images/tmpimage/"


typedef struct wsdl_wnd_t_
{
    bool                    open;                   ///< 是否打开窗口

    wsdl_images_t*          p_images;
    ft_raster_t*            p_ft_raster;

    // 窗口/render
    struct
    {
        SDL_Window*         p_window;               ///< 窗口
        SDL_Renderer*       p_render;               ///< 渲染器

        int                 window_w;               ///< 窗口宽
        int                 window_h;               ///< 窗口高
    };

    // gui/font
    struct
    {
        SDL_Texture*        p_tex_ui;               ///< 主GUI纹理
        klb_canvas_t*       p_canvas_ui;            ///< 主GUI: wsdl_surface_canvas_create
        SDL_Surface*        p_surface_ui;           ///< 主GUI: SDL_Surface
    };

    // 视频
    struct
    {
        wsdl_video_yuv_t*   p_video;                ///< 视频
    };

    // 其他
    struct
    {
        sds                 base_path;              ///< 当前路径
    };
}wsdl_wnd_t;

//////////////////////////////////////////////////////////////////////////
static int wsdl_wnd_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
static int wsdl_wnd_refresh(klb_canvas_t* p_canvas,
                            const klb_rect_t dst[KLB_CANVAS_LAYER_max],
                            klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],
                            const klb_rect_t src[KLB_CANVAS_LAYER_max],
                            int layer_count);

static int wsdl_wnd_refresh_layer(klb_canvas_t* p_canvas, int refresh_opt, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count);
static sds get_basepath_wsdl_wnd();


wsdl_wnd_t* wsdl_wnd_create()
{
    wsdl_wnd_t* p_wnd = KLB_MALLOCZ(wsdl_wnd_t, 1, 0);

    p_wnd->base_path = get_basepath_wsdl_wnd();

    p_wnd->open = false;

    return p_wnd;
}

void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd)
{
    // 关闭窗口
    wsdl_wnd_close(p_wnd);

    KLB_FREE_BY(p_wnd->base_path, sdsfree);
    KLB_FREE(p_wnd);
}

int wsdl_wnd_open(wsdl_wnd_t* p_wnd, klb_gui_t* p_gui, int w, int h, const char* p_title)
{

#if 0
    p_wnd->p_window = SDL_CreateWindow(p_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w/*960*/, h/*540*/, SDL_WINDOW_SHOWN);
    assert(NULL != p_wnd->p_window);
    p_wnd->p_render = SDL_CreateRenderer(p_wnd->p_window, -1, 0);
#else
    SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_SHOWN, &p_wnd->p_window, &p_wnd->p_render);
    assert(NULL != p_wnd->p_window);
    assert(NULL != p_wnd->p_render);

    SDL_SetWindowTitle(p_wnd->p_window, p_title);
#endif

    // 初始化屏幕颜色
    {
        SDL_SetRenderDrawColor(p_wnd->p_render, 10, 10, 10, 255);
        SDL_RenderClear(p_wnd->p_render);
        SDL_RenderPresent(p_wnd->p_render);
    }

    p_wnd->window_w = w;
    p_wnd->window_h = h;


    {
        p_wnd->p_images = wsdl_images_create();
        p_wnd->p_ft_raster = ft_raster_create();
    }

    // ui
    {
        p_wnd->p_tex_ui = SDL_CreateTexture(p_wnd->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

        p_wnd->p_canvas_ui = wsdl_surface_canvas_create(w, h, KLB_COLOR_FMT_ARGB8888, p_wnd->p_images, p_wnd->p_ft_raster, wsdl_wnd_refresh_rect, wsdl_wnd_refresh, wsdl_wnd_refresh_layer, p_wnd);
        p_wnd->p_surface_ui = wsdl_surface_canvas_get_surface(p_wnd->p_canvas_ui);
    }

    // 视频
    p_wnd->p_video = wsdl_video_yuv_create(p_wnd->p_render);


    // 初始化其他
    {
        sds tmp = sdsnew("");

        // 从 demores/images/tmpimage 加载, 模拟临时图片 "~/tmpimage"
        sdsclear(tmp);
        tmp = sdscatfmt(tmp, "%s%stmpimage.bmp", p_wnd->base_path, DIR_PATH_TMPIMAGE);
        if (0 != wsdl_images_load_tmpimage(p_wnd->p_images, tmp))
        {
            sdsclear(tmp);
            tmp = sdscatfmt(tmp, "%s%stmpimage.png", p_wnd->base_path, DIR_PATH_TMPIMAGE);
            wsdl_images_load_tmpimage(p_wnd->p_images, tmp);
        }

        KLB_FREE_BY(tmp, sdsfree);
    }

    // ok
    p_wnd->open = true;

    // test
#if 0
    wsdl_video_yuv_set_size(p_wnd->p_video, 1);

    klb_rect_t rect = { 0 };
    rect.w = 640;
    rect.h = 360;
    wsdl_video_yuv_update_rect(p_wnd->p_video, 0, &rect);
#endif

    return 0;
}

void wsdl_wnd_close(wsdl_wnd_t* p_wnd)
{
    if (p_wnd->open)
    {
        KLB_FREE_BY(p_wnd->p_video, wsdl_video_yuv_destroy);

        KLB_FREE_BY(p_wnd->p_ft_raster, ft_raster_destroy);
        KLB_FREE_BY(p_wnd->p_images, wsdl_images_destroy);

        KLB_FREE_BY(p_wnd->p_canvas_ui, klb_canvas_destroy);

        KLB_FREE_BY(p_wnd->p_tex_ui, SDL_DestroyTexture);
        KLB_FREE_BY(p_wnd->p_render, SDL_DestroyRenderer);
        KLB_FREE_BY(p_wnd->p_window, SDL_DestroyWindow);

        p_wnd->open = false;
    }
}

bool wsdl_wnd_is_open(wsdl_wnd_t* p_wnd)
{
    return p_wnd->open;
}

klb_canvas_t* wsdl_wnd_get_ui_canvas(wsdl_wnd_t* p_wnd)
{
    return p_wnd->p_canvas_ui;
}

wsdl_video_yuv_t* wsdl_wnd_get_video_yuv(wsdl_wnd_t* p_wnd)
{
    return p_wnd->p_video;
}

void wsdl_wnd_render_present(wsdl_wnd_t* p_wnd)
{
    if (p_wnd->open)
    {
        SDL_SetTextureBlendMode(p_wnd->p_tex_ui, SDL_BLENDMODE_NONE);
        SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, NULL, NULL);

        SDL_RenderPresent(p_wnd->p_render);
    }
}

//////////////////////////////////////////////////////////////////////////

// 获取默认启动文件路径
static sds get_basepath_wsdl_wnd()
{
    // 路径
    char* p_base_path = SDL_GetBasePath();
    sds path = sdsnew(p_base_path);
    SDL_free(p_base_path);

    return path;
}

static int wsdl_wnd_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_wnd_t* p_wnd = (wsdl_wnd_t*)p_canvas->p_obj;

    klb_rect_t* ptr = (NULL != p_rect) ? (klb_rect_t*)p_rect : &p_canvas->rect;

    SDL_Rect rect = { 0 };
    rect.x = ptr->x;
    rect.y = ptr->y;
    rect.w = ptr->w;
    rect.h = ptr->h;

    //// UI
    {
        SDL_Surface* p_surface = NULL;
        if (0 == SDL_LockTextureToSurface(p_wnd->p_tex_ui, NULL, &p_surface))
        {
            // 按原始拷贝, 可能含有透明像素
            SDL_SetSurfaceBlendMode(p_wnd->p_surface_ui, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(p_wnd->p_surface_ui, &rect, p_surface, &rect);

            SDL_UnlockTexture(p_wnd->p_tex_ui);
        }
    }

    // 拷贝至屏幕
    SDL_SetRenderTarget(p_wnd->p_render, NULL);

#if 1
    //  按原始拷贝
    SDL_SetTextureBlendMode(p_wnd->p_tex_ui, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, &rect, &rect);
#else
    //  按透明度拷贝
    {
        SDL_SetRenderDrawColor(p_wnd->p_render, 0, 0, 0, 0);
        SDL_RenderFillRect(p_wnd->p_render, &rect);
    }

    SDL_SetTextureBlendMode(p_wnd->p_tex_ui, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, &rect, &rect);
#endif

    SDL_RenderPresent(p_wnd->p_render);

    return 0;
}

static int wsdl_wnd_refresh(klb_canvas_t* p_canvas,
                            const klb_rect_t dst[KLB_CANVAS_LAYER_max],
                            klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],
                            const klb_rect_t src[KLB_CANVAS_LAYER_max],
                            int layer_count)
{
    wsdl_wnd_t* p_wnd = (wsdl_wnd_t*)p_canvas->p_obj;

    SDL_Rect rect_dst[KLB_CANVAS_LAYER_max] = { 0 };
    SDL_Rect rect_src[KLB_CANVAS_LAYER_max] = { 0 };

    // 更新UI图层数据
    SDL_Surface* p_dst_surface = NULL;
    if (0 == SDL_LockTextureToSurface(p_wnd->p_tex_ui, NULL, &p_dst_surface))
    {
        for (int i = 0; i < layer_count; i++)
        {
            SDL_Surface* p_src_surface = wsdl_surface_canvas_get_surface(p_src_canvas[i]);

            // 按原始拷贝, 可能含有透明像素
            SDL_SetSurfaceBlendMode(p_src_surface, SDL_BLENDMODE_NONE);

            // dst rect
            rect_dst[i].x = dst[i].x;
            rect_dst[i].y = dst[i].y;
            rect_dst[i].w = dst[i].w;
            rect_dst[i].h = dst[i].h;

            // src rect
            rect_src[i].x = src[i].x;
            rect_src[i].y = src[i].y;
            rect_src[i].w = src[i].w;
            rect_src[i].h = src[i].h;

            SDL_BlitSurface(p_src_surface, &rect_src[i], p_dst_surface, &rect_dst[i]);
        }

        SDL_UnlockTexture(p_wnd->p_tex_ui);
    }

#if 0
    // 拷贝UI 图层
    for (int i = 0; i < layer_count; i++)
    {
        //  按原始拷贝
        SDL_SetTextureBlendMode(p_wnd->p_tex_ui, SDL_BLENDMODE_NONE);
        SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, &rect_dst[i], &rect_dst[i]);
    }

    SDL_RenderPresent(p_wnd->p_render);
#else
    wsdl_wnd_render_refresh(p_wnd);
#endif

    return 0;
}

static void blit_surface_wsdl_wnd(klb_canvas_t* p_canvas, SDL_Surface* p_dst, klb_rect_t* p_rect)
{
    klb_rect_t rect = { 0 };
    if (!klb_rect_intersect(&rect, p_rect, &p_canvas->rect))
    {
        return; // 矩形无交集
    }

    SDL_Surface* p_src_surface = wsdl_surface_canvas_get_surface(p_canvas);

    // 按原始拷贝, 可能含有透明像素
    SDL_SetSurfaceBlendMode(p_src_surface, SDL_BLENDMODE_NONE);

    SDL_Rect rect_src = { 0 };
    rect_src.x = rect.x - p_canvas->rect.x;
    rect_src.y = rect.y - p_canvas->rect.y;
    rect_src.w = rect.w;
    rect_src.h = rect.h;

    SDL_Rect rect_dst = { 0 };
    rect_dst.x = rect.x;
    rect_dst.y = rect.y;
    rect_dst.w = rect.w;
    rect_dst.h = rect.h;

    SDL_BlitSurface(p_src_surface, &rect_src, p_dst, &rect_dst);
}

static int wsdl_wnd_refresh_layer_copy(klb_canvas_t* p_canvas, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    // 完全刷新
    wsdl_wnd_t* p_wnd = (wsdl_wnd_t*)p_canvas->p_obj;

    // 更新UI图层数据
    SDL_Surface* p_dst_surface = NULL;
    if (0 == SDL_LockTextureToSurface(p_wnd->p_tex_ui, NULL, &p_dst_surface))
    {
        for (int i = 0; i < layer_count; i++)
        {
            if (layers[i].is_used)
            {
                klb_rect_t redraw_rect = layers[i].redraw_rect;
                blit_surface_wsdl_wnd(layers[i].p_canvas, p_dst_surface, &redraw_rect);
            }
        }

        SDL_UnlockTexture(p_wnd->p_tex_ui);
    }

    wsdl_wnd_render_refresh(p_wnd);

    return 0;
}

static int wsdl_wnd_refresh_layer_copy_bubble(klb_canvas_t* p_canvas, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    // 局部刷新
    wsdl_wnd_t* p_wnd = (wsdl_wnd_t*)p_canvas->p_obj;

    // 更新UI图层数据
    SDL_Surface* p_dst_surface = NULL;
    if (0 == SDL_LockTextureToSurface(p_wnd->p_tex_ui, NULL, &p_dst_surface))
    {
        for (int i = 0; i < layer_count; i++)
        {
            if (layers[i].is_used && layers[i].is_redraw)
            {
                klb_rect_t redraw_rect = layers[i].redraw_rect;

                blit_surface_wsdl_wnd(layers[i].p_canvas, p_dst_surface, &redraw_rect);

                for (int j = i + 1; j < layer_count; j++)
                {
                    if (layers[j].is_used)
                    {
                        blit_surface_wsdl_wnd(layers[j].p_canvas, p_dst_surface, &redraw_rect);
                    }
                }
            }
        }

        SDL_UnlockTexture(p_wnd->p_tex_ui);
    }

    wsdl_wnd_render_refresh(p_wnd);

    return 0;
}

static int wsdl_wnd_refresh_layer(klb_canvas_t* p_canvas, int refresh_opt, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    if (KLB_CANVAS_REFRESH_copy == refresh_opt)
    {
        return wsdl_wnd_refresh_layer_copy(p_canvas, layers, layer_count);
    }
    else if (KLB_CANVAS_REFRESH_copy_bubble == refresh_opt)
    {
        return wsdl_wnd_refresh_layer_copy_bubble(p_canvas, layers, layer_count);
    }

    return 0;
}

void wsdl_wnd_render_refresh(wsdl_wnd_t* p_wnd)
{
    // 清空屏幕
    SDL_SetRenderDrawColor(p_wnd->p_render, 5, 5, 5, 255);
    SDL_RenderClear(p_wnd->p_render);

    // 拷贝视频图
    int video_size = wsdl_video_yuv_size(p_wnd->p_video);
    for (int i = 0; i < video_size; i++)
    {
        klb_rect_t rect_video = { 0 };
        SDL_Texture* p_texture = wsdl_video_yuv_get(p_wnd->p_video, i, &rect_video);

        SDL_Rect r_video = { 0 };
        r_video.x = rect_video.x;  r_video.y = rect_video.y;
        r_video.w = rect_video.w;  r_video.h = rect_video.h;

        if (NULL != p_texture && 0 < r_video.w && 0 < r_video.h)
        {
            SDL_SetTextureBlendMode(p_texture, SDL_BLENDMODE_NONE);
            SDL_RenderCopy(p_wnd->p_render, p_texture, NULL, &r_video);
        }
    }

    // 拷贝UI 图层
    SDL_SetTextureBlendMode(p_wnd->p_tex_ui, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, NULL, NULL);

    SDL_RenderPresent(p_wnd->p_render);
}
