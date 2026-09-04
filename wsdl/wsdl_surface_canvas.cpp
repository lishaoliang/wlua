// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "wsdl_surface_canvas.h"
#include "wsdl_surface_canvas_opt.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////

static void wsdl_surface_canvas_destroy(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    KLB_FREE_BY(p_canvas->p_render, SDL_DestroyRenderer);
    KLB_FREE_BY(p_canvas->p_surface, SDL_FreeSurface);

    KLB_FREE(p_canvas);
}

static void copy_rect_wsdl_surface_canvas(wsdl_surface_canvas_t* p_canvas, SDL_Rect* p_dst, const klb_rect_t* p_src)
{
    p_dst->x = p_src->x - p_canvas->canvas.rect.x;
    p_dst->y = p_src->y - p_canvas->canvas.rect.y;
    p_dst->w = p_src->w;
    p_dst->h = p_src->h; 
}

// 
static void update_xy_wsdl_surface_canvas(wsdl_surface_canvas_t* p_canvas, int* p_x, int* p_y)
{
    *p_x -= p_canvas->canvas.rect.x;
    *p_y -= p_canvas->canvas.rect.y;
}

/// @brief 设置绘制颜色
static int wsdl_surface_canvas_set_draw_color(klb_canvas_t* ptr, uint32_t color)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    p_canvas->canvas.draw_color = color;
    p_canvas->canvas.real_draw_color = color;

    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color) & 0xFF;

    SDL_SetRenderDrawColor(p_canvas->p_render, r, g, b, a);

    return 0;
}

/// @brief 获取绘制颜色
static uint32_t wsdl_surface_canvas_get_draw_color(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return p_canvas->canvas.draw_color;
}

/// @brief 设置字体高度
static int wsdl_surface_canvas_set_font_height(klb_canvas_t* ptr, int h)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    p_canvas->canvas.font_h = h;

    return 0;
}

/// @brief 获取字体高度
static int wsdl_surface_canvas_get_font_height(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return p_canvas->canvas.font_h;
}

/// @brief 加载字库
static int wsdl_surface_canvas_load_font(klb_canvas_t* ptr, const char* p_font_path)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (NULL == p_font_path)
    {
        return 1;
    }

    if (NULL == p_canvas->p_ft)
    {
        return 1;
    }

    return ft_raster_load_font(p_canvas->p_ft, p_font_path);
}

/// @brief 卸载字库
static int wsdl_surface_canvas_unload_font(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (NULL == p_canvas->p_ft)
    {
        return 1;
    }

    return ft_raster_unload_font(p_canvas->p_ft);
}

/// @brief 加载图片
static int wsdl_surface_canvas_load_image(klb_canvas_t* ptr, const char* p_key, const char* p_path, int* p_out_w, int* p_out_h)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    wsdl_images_load(p_canvas->p_imgs, p_key, p_path);

    wsdl_images_size(p_canvas->p_imgs, p_key, p_out_w, p_out_h);

    return 0;
}

/// @brief 获取图片大小
static int wsdl_surface_canvas_image_size(klb_canvas_t* ptr, const char* p_key, int* p_out_w, int* p_out_h)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    wsdl_images_size(p_canvas->p_imgs, p_key, p_out_w, p_out_h);

    return 0;
}

/// @brief 清空所有图片资源
static int wsdl_surface_canvas_clear_image(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    wsdl_images_clear(p_canvas->p_imgs);

    return 0;
}

/// @brief 使用单色清空屏幕
static int wsdl_surface_canvas_draw_clear(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    SDL_RenderClear(p_canvas->p_render);

    return 0;
}

/// @brief 绘制点
static int wsdl_surface_canvas_draw_point(klb_canvas_t* ptr, int x, int y)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    update_xy_wsdl_surface_canvas(p_canvas, &x, &y);

    SDL_RenderDrawPoint(p_canvas->p_render, x, y);
    return 0;
}

/// @brief 绘制多个点
static int wsdl_surface_canvas_draw_points(klb_canvas_t* ptr, const klb_point_t* p_points, int count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

/// @brief 绘制线段
static int wsdl_surface_canvas_draw_line(klb_canvas_t* ptr, int x1, int y1, int x2, int y2)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    update_xy_wsdl_surface_canvas(p_canvas, &x1, &y1);
    update_xy_wsdl_surface_canvas(p_canvas, &x2, &y2);

    SDL_RenderDrawLine(p_canvas->p_render, x1, y1, x2, y2);

    return 0;
}

/// @brief 绘制多个线段
static int wsdl_surface_canvas_draw_lines(klb_canvas_t* ptr, const klb_point_t* p_points, int count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

/// @brief 绘制空心矩形
static int wsdl_surface_canvas_draw_rect(klb_canvas_t* ptr, const klb_rect_t* p_rect)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    klb_rect_t* p_src = (NULL != p_rect) ? (klb_rect_t*)p_rect : &(p_canvas->canvas.rect);

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_src);

    SDL_RenderDrawRect(p_canvas->p_render, &rect);

    return 0;
}

/// @brief 绘制多个空心矩形
static int wsdl_surface_canvas_draw_rects(klb_canvas_t* ptr, const klb_rect_t* p_rects, int count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

/// @brief 使用单色填充绘制单个区域
static int wsdl_surface_canvas_draw_fill_rect(klb_canvas_t* ptr, const klb_rect_t* p_rect)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    klb_rect_t* p_src = (NULL != p_rect) ? (klb_rect_t*)p_rect : &(p_canvas->canvas.rect);

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_src);

    SDL_RenderFillRect(p_canvas->p_render, &rect);

    return 0;
}

/// @brief 使用单色填充多个区域
static int wsdl_surface_canvas_draw_fill_rects(klb_canvas_t* ptr, const klb_rect_t* p_rects, int count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

/// @brief 绘制utf8文本
static int wsdl_surface_canvas_draw_text(klb_canvas_t* ptr, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (0 != SDL_LockSurface(p_canvas->p_surface))
    {
        return 1;
    }

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_rect);

    ft_raster_pixels_t raster = { 0 };

    raster.p_pixels = (uint8_t*)p_canvas->p_surface->pixels;
    raster.pitch = p_canvas->p_surface->pitch;
    raster.w = p_canvas->p_surface->w;
    raster.h = p_canvas->p_surface->h;
    raster.bpp = p_canvas->canvas.bpp;
    raster.color_fmt = p_canvas->canvas.color_fmt;

    ft_raster_text(p_canvas->p_ft, &raster, rect.x, rect.y, rect.w, rect.h, p_utf8, utf8_len, p_canvas->canvas.draw_color, p_canvas->canvas.font_h, NULL, NULL, NULL);

    SDL_UnlockSurface(p_canvas->p_surface);

    return 0;
}

/// @brief 以当前字体大小, 绘制utf8文本所需要的宽高
static int wsdl_surface_canvas_text_size(klb_canvas_t* ptr, const char* p_utf8, int utf8_len, int* p_out_w, int* p_out_h)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    ft_raster_text_size(p_canvas->p_ft, p_utf8, utf8_len, p_canvas->canvas.font_h, p_out_w, p_out_h);

    return 0;
}

/// @brief 绘制图片
static int wsdl_surface_canvas_draw_image(klb_canvas_t* ptr, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    SDL_Surface* p_surface = wsdl_images_find(p_canvas->p_imgs, p_path);

    if (NULL == p_surface)
    {
        return 0;
    }

    if (NULL == p_dst_rect)
    {
        return 0;
    }

    int x = p_dst_rect->x;
    int y = p_dst_rect->y;
    update_xy_wsdl_surface_canvas(p_canvas, &x, &y);

    // 按原始图片拷贝, 超出目标矩形则裁剪
    int image_w = 0;
    int image_h = 0;
    wsdl_images_size(p_canvas->p_imgs, p_path, &image_w, &image_h);

    int src_x = 0;
    int src_y = 0;
    int src_w = image_w;
    int src_h = image_h;

    if (NULL != p_src_rect)
    {
        src_x = p_src_rect->x;
        src_y = p_src_rect->y;
        src_w = p_src_rect->w;
        src_h = p_src_rect->h;
    }

    if (src_w > p_dst_rect->w)
    {
        src_w = p_dst_rect->w;
    }

    if (src_h > p_dst_rect->h)
    {
        src_h = p_dst_rect->h;
    }

    if (0 >= src_w || 0 >= src_h)
    {
        return 0;
    }

    SDL_Rect src_rect = { src_x, src_y, src_w, src_h };
    SDL_Rect dst_rect = { x, y, src_w, src_h };

    SDL_BlitSurface(p_surface, &src_rect, p_canvas->p_surface, &dst_rect);

    return 0;
}

/// @brief 拷贝绘制画布
static int wsdl_surface_canvas_draw_copy(klb_canvas_t* ptr, int x, int y, const klb_canvas_t* p_src, const klb_rect_t* p_src_rect)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;
    wsdl_surface_canvas_t* p_src_canvas = (wsdl_surface_canvas_t*)p_src;

    SDL_Rect src_rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &src_rect, p_src_rect);

    update_xy_wsdl_surface_canvas(p_canvas, &x, &y);

    SDL_Rect dst_rect = { 0 };
    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.w = p_src_rect->w;
    dst_rect.h = p_src_rect->h;

    SDL_BlitSurface(p_src_canvas->p_surface, &src_rect, p_canvas->p_surface, &dst_rect);

    return 0;
}

/// @brief 刷新画布到显存(屏幕)
static int wsdl_surface_canvas_refresh_rect(klb_canvas_t* ptr, const klb_rect_t* p_rect)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (p_canvas->cb_refresh_rect)
    {
        p_canvas->cb_refresh_rect(ptr, p_rect);
    }

    return 0;
}

/// @brief 刷新画布到显存(屏幕)
static int wsdl_surface_canvas_refresh_rects(klb_canvas_t* ptr, const klb_rect_t* p_rects, int count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

/// @brief 刷新画布到显存(屏幕), 支持多图层合并刷新
static int wsdl_surface_canvas_refresh(klb_canvas_t* ptr,
                                        const klb_rect_t dst[KLB_CANVAS_LAYER_max],
                                        klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],
                                        const klb_rect_t src[KLB_CANVAS_LAYER_max],
                                        int layer_count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (p_canvas->cb_refresh)
    {
        p_canvas->cb_refresh(ptr, dst, p_src_canvas, src, layer_count);
    }

    return 0;
}

/// @brief 刷新画布到显存(屏幕), 支持多个图层合并刷新
static int wsdl_surface_canvas_refresh_layer(klb_canvas_t* ptr, int refresh_opt, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    if (p_canvas->cb_refresh_layer)
    {
        return p_canvas->cb_refresh_layer(ptr, refresh_opt, layers, layer_count);
    }

    return 0;
}

/// @brief 移动画布位置
static int wsdl_surface_canvas_move(klb_canvas_t* ptr, int x, int y)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    p_canvas->canvas.rect.x = x;
    p_canvas->canvas.rect.y = y;

    return 0;
}

/// @brief 重新设置画布宽高
static int wsdl_surface_canvas_resize(klb_canvas_t* ptr, int w, int h)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    // Win版本, 不支持变更画布大小
    // 在创建时, 直接使用最大值来模拟

    p_canvas->canvas.rect.w = w;
    p_canvas->canvas.rect.h = h;

    assert(p_canvas->canvas.rect.w <= p_canvas->p_surface->w);
    assert(p_canvas->canvas.rect.h <= p_canvas->p_surface->h);

    return 0;
}

/// @brief 申请画布
static klb_canvas_t* wsdl_surface_canvas_malloc(klb_canvas_t* ptr, int idx, int rsv, int layer_type)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    // 画布表面宽高
    int w = p_canvas->p_surface->w;
    int h = p_canvas->p_surface->h;

    if(KLB_CANVAS_LAYER_popup == layer_type)
    {
        klb_canvas_t* p_new_canvas = wsdl_surface_canvas_create(w, h, KLB_COLOR_FMT_ARGB8888, p_canvas->p_imgs, p_canvas->p_ft, NULL, NULL, NULL, NULL);

        return p_new_canvas;
    }
    else if(KLB_CANVAS_LAYER_msgbox == layer_type)
    {
        klb_canvas_t* p_new_canvas = wsdl_surface_canvas_create(w / 2, h / 2, KLB_COLOR_FMT_ARGB8888, p_canvas->p_imgs, p_canvas->p_ft, NULL, NULL, NULL, NULL);

        return p_new_canvas;
    } 
    else if(KLB_CANVAS_LAYER_udata == layer_type)
    {
        klb_canvas_t* p_new_canvas = wsdl_surface_canvas_create(w / 2, h / 2, KLB_COLOR_FMT_ARGB8888, p_canvas->p_imgs, p_canvas->p_ft, NULL, NULL, NULL, NULL);

        return p_new_canvas;
    }
    else if (KLB_CANVAS_LAYER_wait == layer_type)
    {
        klb_canvas_t* p_new_canvas = wsdl_surface_canvas_create(w / 2, h / 2, KLB_COLOR_FMT_ARGB8888, p_canvas->p_imgs, p_canvas->p_ft, NULL, NULL, NULL, NULL);

        return p_new_canvas;
    }
    else if (KLB_CANVAS_LAYER_tip == layer_type)
    {
        klb_canvas_t* p_new_canvas = wsdl_surface_canvas_create(w, 128, KLB_COLOR_FMT_ARGB8888, p_canvas->p_imgs, p_canvas->p_ft, NULL, NULL, NULL, NULL);

        return p_new_canvas;
    }

    return NULL;
}

/// @brief 释放画布
static void wsdl_surface_canvas_free(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_destroy(ptr);
}

//////////////////////////////////////////////////////////////////////////
static void wsdl_surface_canvas_init(wsdl_surface_canvas_t* p_canvas)
{
    // 参数
    {
        p_canvas->canvas.bpp = 4;
        p_canvas->canvas.color_fmt = KLB_COLOR_FMT_ARGB8888;

        p_canvas->canvas.rect.w = p_canvas->p_surface->w;
        p_canvas->canvas.rect.h = p_canvas->p_surface->h;
    }

    // vtable
    klb_canvas_vtable_t* p_vtable = &(p_canvas->canvas.vtable);

    {
        p_vtable->lock = NULL;      ///< 预留
        p_vtable->unlock = NULL;    ///< 预留

        p_vtable->set_draw_color = wsdl_surface_canvas_set_draw_color;
        p_vtable->get_draw_color = wsdl_surface_canvas_get_draw_color;
        p_vtable->set_font_height = wsdl_surface_canvas_set_font_height;
        p_vtable->get_font_height = wsdl_surface_canvas_get_font_height;
        p_vtable->load_font = wsdl_surface_canvas_load_font;
        p_vtable->unload_font = wsdl_surface_canvas_unload_font;

        p_vtable->load_image = wsdl_surface_canvas_load_image;
        p_vtable->image_size = wsdl_surface_canvas_image_size;
        p_vtable->clear_image = wsdl_surface_canvas_clear_image;
        p_vtable->draw_clear = wsdl_surface_canvas_draw_clear;
        p_vtable->draw_point = wsdl_surface_canvas_draw_point;
        p_vtable->draw_points = wsdl_surface_canvas_draw_points;
        p_vtable->draw_line = wsdl_surface_canvas_draw_line;
        p_vtable->draw_lines = wsdl_surface_canvas_draw_lines;
        p_vtable->draw_rect = wsdl_surface_canvas_draw_rect;
        p_vtable->draw_rects = wsdl_surface_canvas_draw_rects;
        p_vtable->draw_fill_rect = wsdl_surface_canvas_draw_fill_rect;
        p_vtable->draw_fill_rects = wsdl_surface_canvas_draw_fill_rects;
        p_vtable->draw_text = wsdl_surface_canvas_draw_text;
        p_vtable->text_size = wsdl_surface_canvas_text_size;
        p_vtable->draw_image = wsdl_surface_canvas_draw_image;
        p_vtable->draw_copy = wsdl_surface_canvas_draw_copy;
        p_vtable->refresh_rect = wsdl_surface_canvas_refresh_rect;
        p_vtable->refresh_rects = wsdl_surface_canvas_refresh_rects;
        p_vtable->refresh = wsdl_surface_canvas_refresh;
        p_vtable->refresh_layer = wsdl_surface_canvas_refresh_layer;

        // 画布
        p_vtable->move = wsdl_surface_canvas_move;
        p_vtable->resize = wsdl_surface_canvas_resize;

        p_vtable->malloc = wsdl_surface_canvas_malloc;
        p_vtable->free = wsdl_surface_canvas_free;


        // 自定义绘图
        p_vtable->draw_opt1 = wsdl_surface_canvas_draw_opt1;
        p_vtable->draw_opt2 = wsdl_surface_canvas_draw_opt2;
        p_vtable->draw_opt3 = wsdl_surface_canvas_draw_opt3;
        p_vtable->draw_opt4 = wsdl_surface_canvas_draw_opt4;
        p_vtable->draw_opt5 = wsdl_surface_canvas_draw_opt5;
        p_vtable->draw_opt6 = wsdl_surface_canvas_draw_opt6;
        p_vtable->draw_opt7 = wsdl_surface_canvas_draw_opt7;
        p_vtable->draw_opt8 = wsdl_surface_canvas_draw_opt8;


        // 用户自定义 直接操控画布设备函数
        p_vtable->ioctrl_opt8 = wsdl_surface_canvas_ioctrl_opt8;
    }
}

//////////////////////////////////////////////////////////////////////////

SDL_Surface* wsdl_surface_canvas_get_surface(klb_canvas_t* ptr)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return p_canvas->p_surface;
}

//////////////////////////////////////////////////////////////////////////

klb_canvas_t* wsdl_surface_canvas_create(int w, int h, klb_color_fmt_e fmt, 
                                    wsdl_images_t* p_imgs, 
                                    ft_raster_t* p_ft, 
                                    wsdl_surface_canvas_refresh_rect_cb cb_refresh_rect, 
                                    wsdl_surface_canvas_refresh_cb cb_refresh,
                                    wsdl_surface_canvas_refresh_layer_cb cb_refresh_layer,
                                    void* ptr)
{
    wsdl_surface_canvas_t* p_canvas = KLB_MALLOCZ(wsdl_surface_canvas_t, 1, 0);

    //
    {
        p_canvas->p_imgs = p_imgs;
        p_canvas->p_ft = p_ft;

        p_canvas->canvas.p_obj = ptr;
        p_canvas->cb_refresh_rect = cb_refresh_rect;
        p_canvas->cb_refresh = cb_refresh;
        p_canvas->cb_refresh_layer = cb_refresh_layer;
    }

    // sdl
    {
        p_canvas->p_surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);

        p_canvas->p_render = SDL_CreateSoftwareRenderer(p_canvas->p_surface);
    }

    // init canvas
    wsdl_surface_canvas_init(p_canvas);

    return (klb_canvas_t*)p_canvas;
}
