///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_surface_canvas.h
/// @brief   windows sdl surface canvas, sdl画布表面(surface)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_SURFACE_CANVAS_H__
#define __WSDL_SURFACE_CANVAS_H__


#include "klb_type.h"
#include "klbutil/klb_canvas.h"
#include "klbutil/klb_color.h"
#include "SDL.h"
#include "wsdl_images.h"
#include "ft_raster.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef int(*wsdl_surface_canvas_refresh_rect_cb)(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
typedef int(*wsdl_surface_canvas_refresh_cb)(klb_canvas_t* p_canvas,
                                            const klb_rect_t dst[KLB_CANVAS_LAYER_max],
                                            klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],
                                            const klb_rect_t src[KLB_CANVAS_LAYER_max],
                                            int layer_count);
typedef int(*wsdl_surface_canvas_refresh_layer_cb)(klb_canvas_t* p_canvas, int refresh_opt, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count);
                                            
/// @struct sdl_surface_canvas_t
/// @brief  画布
typedef struct wsdl_surface_canvas_t_
{
    klb_canvas_t        canvas;             ///< 首位, 用于"继承" "klb_canvas_*"系列函数

    // sdl 相关
    struct
    {
        SDL_Renderer*   p_render;           ///< 与表面关联的 软render
        SDL_Surface*    p_surface;          ///< 表面
    };

    //
    struct
    {
        wsdl_images_t*  p_imgs;
        ft_raster_t*    p_ft;
    };

    struct
    {
        wsdl_surface_canvas_refresh_rect_cb     cb_refresh_rect;
        wsdl_surface_canvas_refresh_cb          cb_refresh;
        wsdl_surface_canvas_refresh_layer_cb    cb_refresh_layer;
    };
}wsdl_surface_canvas_t;


klb_canvas_t* wsdl_surface_canvas_create(int w, int h, klb_color_fmt_e fmt, wsdl_images_t* p_imgs, ft_raster_t* p_ft, wsdl_surface_canvas_refresh_rect_cb cb_refresh_rect, wsdl_surface_canvas_refresh_cb cb_refresh, wsdl_surface_canvas_refresh_layer_cb cb_refresh_layer, void* ptr);


SDL_Surface* wsdl_surface_canvas_get_surface(klb_canvas_t* ptr);


#ifdef __cplusplus
}
#endif


#endif // __WSDL_SURFACE_CANVAS_H__
//end
