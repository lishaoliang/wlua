#ifndef __WSDL_SURFACE_CANVAS_OPT_H__
#define __WSDL_SURFACE_CANVAS_OPT_H__


#include "wsdl_surface_canvas.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief UI扩展绘图底层实现
int wsdl_surface_canvas_draw_opt1(klb_canvas_t* ptr, int opt, const void* ptr1);
int wsdl_surface_canvas_draw_opt2(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2);
int wsdl_surface_canvas_draw_opt3(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3);
int wsdl_surface_canvas_draw_opt4(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4);
int wsdl_surface_canvas_draw_opt5(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5);
int wsdl_surface_canvas_draw_opt6(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6);
int wsdl_surface_canvas_draw_opt7(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7);
int wsdl_surface_canvas_draw_opt8(klb_canvas_t* ptr, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8);


/// @brief 用户自定义 直接操控画布设备函数
int wsdl_surface_canvas_ioctrl_opt8(klb_canvas_t* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);


#if defined(__cplusplus)
}
#endif

#endif // __WSDL_SURFACE_CANVAS_OPT_H__
//end
