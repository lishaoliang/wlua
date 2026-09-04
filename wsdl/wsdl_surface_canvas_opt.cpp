#include "wsdl_surface_canvas_opt.h"
#include "klbgui/klb_wnd_ex.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////

static void wsdl_surface_canvas_opt_set_draw_color(wsdl_surface_canvas_t* p_canvas, uint32_t color)
{
    p_canvas->canvas.draw_color = color;
    p_canvas->canvas.real_draw_color = color;

    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color) & 0xFF;

    SDL_SetRenderDrawColor(p_canvas->p_render, r, g, b, a);
}

static uint32_t wsdl_surface_canvas_opt_get_image_color(SDL_Surface* p_img_surface, int x, int y)
{
    uint32_t color = 0xFF050505;

    if (0 <= x && x < p_img_surface->w &&
        0 <= y && y < p_img_surface->h)
    {
        int bpp = p_img_surface->format->BytesPerPixel;
        uint8_t* ptr = (uint8_t*)(p_img_surface->pixels) + p_img_surface->pitch * y + bpp * x;

        switch (p_img_surface->format->format)
        {
        case SDL_PIXELFORMAT_ARGB8888:
            {
                memcpy(&color, ptr, sizeof(uint32_t));
            }
            break;
        default:
            break;
        }
    }

    return color;
}

// 扩展绘制文本
static int wsdl_surface_canvas_drawopt_text(wsdl_surface_canvas_t* ptr, const klb_rect_t* p_rect, const char* p_utf8, int* p_utf8_len, uint32_t* p_color, int* p_font_h, int* p_out_w, int* p_out_h, bool* p_out_all)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    int utf8_len = (NULL != p_utf8_len) ? *p_utf8_len : 0;
    uint32_t draw_color = (NULL != p_color) ? *p_color : KLB_ARGB8888(255, 200, 200, 200);
    int font_h = (NULL != p_font_h) ? *p_font_h : 20;

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

    ft_raster_text(p_canvas->p_ft, &raster, rect.x, rect.y, rect.w, rect.h, p_utf8, utf8_len, draw_color, font_h, p_out_w, p_out_h, p_out_all);

    SDL_UnlockSurface(p_canvas->p_surface);

    return 0;
}


// 绘制文本，带自动换行; 支持"\n"换行
static int wsdl_surface_canvas_drawopt_text_lines(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int* p_utf8_len, uint32_t* p_color, int* p_font_h, int* p_line_spacing)
{
    if ((NULL == p_utf8) || (NULL == p_utf8_len) || (*p_utf8_len <= 0))
    {
        return 1;
    }

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

    ft_raster_text_ex(p_canvas->p_ft, &raster, rect.x, rect.y, rect.w, rect.h, p_utf8, *p_utf8_len, *p_color, *p_font_h, *p_line_spacing);

    SDL_UnlockSurface(p_canvas->p_surface);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 绘制线条, 包含斜线
static int wsdl_surface_canvas_drawopt_line(wsdl_surface_canvas_t* p_canvas, const int* p_x1, const int* p_y1, const int* p_x2, const int* p_y2, const uint32_t* p_color, const int* p_thick)
{
    if (p_x1 && p_y1 && p_x2 && p_y2)
    {
        if (p_color)
        {
            wsdl_surface_canvas_opt_set_draw_color(p_canvas, *p_color);
        }

        int x1 = *p_x1, y1 = *p_y1, x2 = *p_x2, y2 = *p_y2;
        update_xy_wsdl_surface_canvas(p_canvas, &x1, &y1);
        update_xy_wsdl_surface_canvas(p_canvas, &x2, &y2);

        SDL_RenderDrawLine(p_canvas->p_render, x1, y1, x2, y2);
    }

    return 0;
}

/// @brief 绘制线条, 包含斜线
static int wsdl_surface_canvas_drawopt_lines(wsdl_surface_canvas_t* p_canvas, const klb_point_t* p_pt1, const klb_point_t* p_pt2, const uint32_t* p_color, const int* p_count, const int* p_thick)
{
    int count = (NULL != p_count) ? *p_count : 0;

    for (int i = 0; i < count; i++)
    {
        wsdl_surface_canvas_opt_set_draw_color(p_canvas, p_color[i]);

        int x1 = p_pt1[i].x, y1 = p_pt1[i].y, x2 = p_pt2[i].x, y2 = p_pt2[i].y;
        update_xy_wsdl_surface_canvas(p_canvas, &x1, &y1);
        update_xy_wsdl_surface_canvas(p_canvas, &x2, &y2);

        SDL_RenderDrawLine(p_canvas->p_render, x1, y1, x2, y2);
    }

    return 0;
}

/// @brief 绘制矩形区域边框
static int wsdl_surface_canvas_drawopt_rect(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rect, const uint32_t* p_color, const int* p_thick)
{
    if (NULL == p_rect)
    {
        return 0;
    }

    uint32_t color = (NULL != p_color) ? *p_color : KLB_ARGB8888(255, 200, 200, 200);
    int thick = (NULL != p_thick) ? *p_thick : 1;

    SDL_Rect rect;
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_rect);

    SDL_RenderDrawRect(p_canvas->p_render, &rect);

    return 0;
}

/// @brief 绘制 多个 矩形区域边框
static int wsdl_surface_canvas_drawopt_rects(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rects, const uint32_t* p_color, const int* p_count, const int* p_thick)
{
    int count = (NULL != p_count) ? *p_count : 0;
    count = MIN(count, KLB_CANVAS_DRAW_RECTS_MAX);

    if (count <= 0)
    {
        return 0;
    }

    int thick = (NULL != p_thick) ? *p_thick : 1;

    SDL_Rect rects[KLB_CANVAS_DRAW_RECTS_MAX];
    uint32_t color = 0;

    for (int i = 0; i < count; i++)
    {
        copy_rect_wsdl_surface_canvas(p_canvas, &rects[i], &p_rects[i]);

        color = p_color[i]; // SDL 只提供了, 绘制多个矩形区域, 同一个颜色
    }

    // 颜色
    wsdl_surface_canvas_opt_set_draw_color(p_canvas, color);

    SDL_RenderDrawRects(p_canvas->p_render, rects, count);

    return 0;
}

/// @brief 填充(多个)矩形区域
static int wsdl_surface_canvas_drawopt_fill_rects(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rects, const uint32_t* p_color, const int* p_count)
{
    int count = (NULL != p_count) ? *p_count : 0;
    count = MIN(count, KLB_CANVAS_DRAW_FILL_RECTS_MAX);

    if (count <= 0)
    {
        return 0;
    }

    SDL_Rect rects[KLB_CANVAS_DRAW_FILL_RECTS_MAX];
    uint32_t color = 0;

    for (int i = 0; i < count; i++)
    {
        copy_rect_wsdl_surface_canvas(p_canvas, &rects[i], &p_rects[i]);

        color = p_color[i]; // SDL 只提供了, 绘制多个矩形区域, 同一个颜色
    }

    // 颜色
    wsdl_surface_canvas_opt_set_draw_color(p_canvas, color);

    // 绘制
    SDL_RenderFillRects(p_canvas->p_render, rects, count);

    return 0;
}

/// @brief 绘制图片, 缩放图片到目标矩形区域内
static int wsdl_surface_canvas_drawopt_image_resize(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_image_path)
{
    SDL_Surface* p_surface = wsdl_images_find(p_canvas->p_imgs, p_image_path);

    if (NULL == p_surface)
    {
        return 0;
    }

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_dst_rect);

    int image_w = 0;
    int image_h = 0;
    wsdl_images_size(p_canvas->p_imgs, p_image_path, &image_w, &image_h);

    if (0 >= image_w || 0 >= image_h || 0 >= rect.w || 0 >= rect.h)
    {
        return 0;
    }

    SDL_Rect src_rect = { 0, 0, image_w, image_h };
    SDL_Rect dst_rect = { rect.x, rect.y, rect.w, rect.h };

    SDL_SoftStretch(p_surface, &src_rect, p_canvas->p_surface, &dst_rect);

    return 0;
}

/// @brief 绘制图片, 使得图片居中拉伸
static int wsdl_surface_canvas_drawopt_image_color_key(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_image_path)
{
    SDL_Surface* p_surface = wsdl_images_find(p_canvas->p_imgs, p_image_path);

    if (NULL == p_surface)
    {
        return 0;
    }

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_rect);

    // default + color-key: 拉伸到目标矩形
    int image_w = 0;
    int image_h = 0;
    wsdl_images_size(p_canvas->p_imgs, p_image_path, &image_w, &image_h);

    if (0 >= image_w || 0 >= image_h || 0 >= rect.w || 0 >= rect.h)
    {
        return 0;
    }

    SDL_Rect src_rect = { 0, 0, image_w, image_h };
    SDL_Rect dst_rect = { rect.x, rect.y, rect.w, rect.h };

    SDL_SoftStretch(p_surface, &src_rect, p_canvas->p_surface, &dst_rect);

    return 0;
}

/// @brief 绘制图片, 九宫格居中拉伸; 角块限制在目标矩形内
static int wsdl_surface_canvas_drawopt_image_scale9(wsdl_surface_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_image_path)
{ 
    SDL_Surface* p_img_surface = wsdl_images_find(p_canvas->p_imgs, p_image_path);
    if (NULL == p_img_surface)
    {
        return 1;
    }

    SDL_Rect rect = { 0 };
    copy_rect_wsdl_surface_canvas(p_canvas, &rect, p_rect);

    SDL_Surface* p_surface = p_canvas->p_surface;

    int img_w = p_img_surface->w;
    int img_h = p_img_surface->h;

    if (0 >= img_w || 0 >= img_h || 0 >= rect.w || 0 >= rect.h)
    {
        return 0;
    }

    // dest 太小无法切九宫格时退回整图拉伸
    if (2 > rect.w || 2 > rect.h)
    {
        SDL_Rect src_all = { 0, 0, img_w, img_h };
        SDL_Rect dst_all = { rect.x, rect.y, rect.w, rect.h };
        SDL_SoftStretch(p_img_surface, &src_all, p_surface, &dst_all);
        return 0;
    }

    int src_cw = img_w / 2;
    int src_ch = img_h / 2;
    if (1 > src_cw)
    {
        src_cw = 1;
    }
    if (1 > src_ch)
    {
        src_ch = 1;
    }

    int corner_w = src_cw;
    int corner_h = src_ch;
    if (corner_w > rect.w / 2)
    {
        corner_w = rect.w / 2;
    }
    if (corner_h > rect.h / 2)
    {
        corner_h = rect.h / 2;
    }

    int mid_w = rect.w - corner_w * 2;
    int mid_h = rect.h - corner_h * 2;

    uint32_t color = wsdl_surface_canvas_opt_get_image_color(p_img_surface, src_cw, src_ch);

    // 上边中间区域
    if (0 < mid_w && 0 < corner_h)
    {
        SDL_Rect rect_dst = { rect.x + corner_w, rect.y, mid_w, corner_h };
        SDL_Rect rect_src = { src_cw, 0, 1, src_ch };

        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 下边中间区域
    if (0 < mid_w && 0 < corner_h)
    {
        SDL_Rect rect_dst = { rect.x + corner_w, rect.y + rect.h - corner_h, mid_w, corner_h };
        SDL_Rect rect_src = { src_cw, src_ch, 1, src_ch };

        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 左边区域
    if (0 < mid_h && 0 < corner_w)
    {
        SDL_Rect rect_dst = { rect.x, rect.y + corner_h, corner_w, mid_h };
        SDL_Rect rect_src = { 0, src_ch, src_cw, 1 };

        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 右边区域
    if (0 < mid_h && 0 < corner_w)
    {
        SDL_Rect rect_dst = { rect.x + rect.w - corner_w, rect.y + corner_h, corner_w, mid_h };
        SDL_Rect rect_src = { src_cw, src_ch, src_cw, 1 };

        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 中间矩形区域: 写 surface, 避免 renderer 未 flush 残留
    if (0 < mid_w && 0 < mid_h)
    {
        SDL_Rect rect1 = { rect.x + corner_w, rect.y + corner_h, mid_w, mid_h };
        uint8_t a = (uint8_t)((color >> 24) & 0xFF);
        uint8_t r = (uint8_t)((color >> 16) & 0xFF);
        uint8_t g = (uint8_t)((color >> 8) & 0xFF);
        uint8_t b = (uint8_t)(color & 0xFF);
        uint32_t pixel = SDL_MapRGBA(p_surface->format, r, g, b, a);

        SDL_FillRect(p_surface, &rect1, pixel);
    }

    // 左上角
    {
        SDL_Rect rect_dst = { rect.x, rect.y, corner_w, corner_h };
        SDL_Rect rect_src = { 0, 0, src_cw, src_ch };
        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 右上角
    {
        SDL_Rect rect_dst = { rect.x + rect.w - corner_w, rect.y, corner_w, corner_h };
        SDL_Rect rect_src = { src_cw, 0, src_cw, src_ch };
        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 左下角
    {
        SDL_Rect rect_dst = { rect.x, rect.y + rect.h - corner_h, corner_w, corner_h };
        SDL_Rect rect_src = { 0, src_ch, src_cw, src_ch };
        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    // 右下角
    {
        SDL_Rect rect_dst = { rect.x + rect.w - corner_w, rect.y + rect.h - corner_h, corner_w, corner_h };
        SDL_Rect rect_src = { src_cw, src_ch, src_cw, src_ch };
        SDL_BlitScaled(p_img_surface, &rect_src, p_surface, &rect_dst);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 接口

int wsdl_surface_canvas_draw_opt1(klb_canvas_t* ptr, int opt, const void* ptr1)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

int wsdl_surface_canvas_draw_opt2(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_IMAGE_RESIZE:
        return wsdl_surface_canvas_drawopt_image_resize(p_canvas, (const klb_rect_t*)ptr1, (const char*)ptr2);
        break;

    case KLB_CANVAS_DRAW_OPT_IMAGE_COLOR_KEY:
        return wsdl_surface_canvas_drawopt_image_color_key(p_canvas, (const klb_rect_t*)ptr1, (const char*)ptr2);
        break;

    case KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9:
    case KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9_COLOR_KEY:
        return wsdl_surface_canvas_drawopt_image_scale9(p_canvas, (const klb_rect_t*)ptr1, (const char*)ptr2);
        break;

    default:
        break;
    }

    return 0;
}

int wsdl_surface_canvas_draw_opt3(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_RECT:
        return wsdl_surface_canvas_drawopt_rect(p_canvas, (const klb_rect_t*)ptr1, (const uint32_t*)ptr2, (const int*)ptr3);
        break;

    case KLB_CANVAS_DRAW_OPT_FILL_RECTS:
        return wsdl_surface_canvas_drawopt_fill_rects(p_canvas, (const klb_rect_t*)ptr1, (const uint32_t*)ptr2, (const int*)ptr3);
        break;

    default:
        break;
    }

    return 0;
}

int wsdl_surface_canvas_draw_opt4(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_RECTS:
        return wsdl_surface_canvas_drawopt_rects(p_canvas, (const klb_rect_t*)ptr1, (const uint32_t*)ptr2, (const int*)ptr3, (const int*)ptr4);
        break;

    default:
        break;
    }

    return 0;
}

int wsdl_surface_canvas_draw_opt5(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_LINES:
        return wsdl_surface_canvas_drawopt_lines(p_canvas, (const klb_point_t*)ptr1, (const klb_point_t*)ptr2, (const uint32_t*)ptr3, (const int*)ptr4, (const int*)ptr5);
        break;

    default:
        break;
    }

    return 0;
}

int wsdl_surface_canvas_draw_opt6(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_LINE:
        return wsdl_surface_canvas_drawopt_line(p_canvas, (const int*)ptr1, (const int*)ptr2, (const int*)ptr3, (const int*)ptr4, (const uint32_t*)ptr5, (const int*)ptr6);
        break;

    case KLB_CANVAS_DRAW_OPT_TEXT_LINES:
        return wsdl_surface_canvas_drawopt_text_lines(p_canvas, (const klb_rect_t*)ptr1, (const char*)ptr2, (int*)ptr3, (uint32_t*)ptr4, (int*)ptr5, (int*)ptr6);
        break;

    default:
        break;
    }

    return 0;
}

int wsdl_surface_canvas_draw_opt7(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    return 0;
}

int wsdl_surface_canvas_draw_opt8(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_DRAW_OPT_TEXT:
        return wsdl_surface_canvas_drawopt_text(p_canvas, (const klb_rect_t*)ptr1, (const char*)ptr2, (int*)ptr3, (uint32_t*)ptr4, (int*)ptr5, (int*)ptr6, (int*)ptr7, (bool*)ptr8);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 用户自定义 直接操控画布设备函数

/// @brief 获取图片信息
static int wsdl_surface_canvas_ioctrl_get_image(wsdl_surface_canvas_t* p_canvas, const char* p_key, void* p_out)
{
    return 1;
}

/// @brief 获取临时图片信息
static int wsdl_surface_canvas_ioctrl_get_tmpimage(wsdl_surface_canvas_t* p_canvas, int* p_w, int* p_h, void* p_out)
{
    return 0;
}

int wsdl_surface_canvas_ioctrl_opt8(klb_canvas_t* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8)
{
    wsdl_surface_canvas_t* p_canvas = (wsdl_surface_canvas_t*)ptr;

    switch (opt)
    {
    case KLB_CANVAS_IOCTRL_OPT_IMAGE:
        return wsdl_surface_canvas_ioctrl_get_image(p_canvas, (const char*)ptr1, ptr2);
        break;

    case KLB_CANVAS_IOCTRL_OPT_TMPIMAGE:
        return wsdl_surface_canvas_ioctrl_get_tmpimage(p_canvas, (int*)ptr1, (int*)ptr2, ptr3);
        break;

    default:
        break;
    }

    return 0;
}
