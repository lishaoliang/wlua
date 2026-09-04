///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_wnd.h
/// @brief   windows sdl window, 窗口(对话框)部分
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_WND_H__
#define __WSDL_WND_H__


#include "klb_type.h"

#include "SDL.h"
#include "klbutil/klb_canvas.h"
#include "klbgui/klb_gui.h"
#include "ft_raster.h"
#include "wsdl_images.h"
#include "wsdl_video_yuv.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_wnd_t_ wsdl_wnd_t;


wsdl_wnd_t* wsdl_wnd_create();
void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd);


ft_raster_t* wsdl_wnd_get_ft_raster(wsdl_wnd_t* p_wnd);
int wsdl_wnd_load_font(wsdl_wnd_t* p_wnd, const char* p_font_path);


int wsdl_wnd_open(wsdl_wnd_t* p_wnd, klb_gui_t* p_gui, int w, int h, const char* p_title);
void wsdl_wnd_close(wsdl_wnd_t* p_wnd);

bool wsdl_wnd_is_open(wsdl_wnd_t* p_wnd);

klb_canvas_t* wsdl_wnd_get_ui_canvas(wsdl_wnd_t* p_wnd);

wsdl_video_yuv_t* wsdl_wnd_get_video_yuv(wsdl_wnd_t* p_wnd);

// 立即刷新显存
void wsdl_wnd_render_present(wsdl_wnd_t* p_wnd);

void wsdl_wnd_render_refresh(wsdl_wnd_t* p_wnd);


#ifdef __cplusplus
}
#endif


#endif // __WSDL_WND_H__
//end
