///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    ft_raster.h
/// @brief   freetype2 文本绘制
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __FT_RASTER_H__
#define __FT_RASTER_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct ft_raster_t_ ft_raster_t;


ft_raster_t* ft_raster_create();
void ft_raster_destroy(ft_raster_t* p_ft);


int ft_raster_load_font(ft_raster_t* p_ft, const char* p_font_path);
int ft_raster_unload_font(ft_raster_t* p_ft);

/// @brief 字库是否已通过 load_font 加载
bool ft_raster_is_font_loaded(ft_raster_t* p_ft);


typedef struct ft_raster_pixels_t_
{
    uint8_t*    p_pixels;
    int64_t     pitch;
    int         w;
    int         h;
    int         bpp;
    int         color_fmt;
}ft_raster_pixels_t;


// 绘制文本
int ft_raster_text(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int* p_out_w, int* p_out_h, bool* p_out_draw_all);

// 绘制文本, 带换行
int ft_raster_text_ex(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int line_spacing);

// 测算绘制文本, 所需要的宽度
int ft_raster_text_size(ft_raster_t* p_ft, const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h);


#ifdef __cplusplus
}
#endif

#endif // __FT_RASTER_H__
//end
