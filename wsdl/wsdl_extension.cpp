// Doc Encode : UTF-8 BOM, Unix(LF)
#include "wsdl_extension.h"
#include "SDL.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_canvas.h"
#include "klua/klua_gui.h"
#include "klbutil/klb_log.h"
#include "klbgui/klb_gui.h"
#include "klua/klua_gui.h"
#include "ft_raster.h"
#include "wsdl_images.h"
#include "wsdl_wnd.h"
#include "wsdl_ui_video.h"
#include "wsdl_video.h"
#include "wsdl_audio.h"
#include <assert.h>


#define WSDL_EXTENSION      "WSDL_EXTENSION"


typedef struct wsdl_extension_t_
{
    klua_env_t*         p_env;              ///< 附加环境: env
    klb_gui_t*          p_gui;              ///< gui

    int                 w;
    int                 h;

    ft_raster_t*        p_ft;               ///< 字体
    wsdl_images_t*      p_imgs;             ///< 图片

    wsdl_wnd_t*         p_wnd;              ///< 系统窗口(对话框)
    klb_canvas_t        canvas;             ///< gui 画布

    sds                 base_path;          ///< 执行文件路径

    wsdl_video_t*       p_video;            ///< 视频处理
    wsdl_audio_t*       p_audio;            ///< 音频处理

    // 鼠标状态
    struct
    {
        bool            btn_left;           ///< 鼠标左键
        bool            is_drag;            ///< 是否为拖拽

        int             drag_sx;
        int             drag_sy;

        int             x;                  ///< 当前鼠标X
        int             y;                  ///< 当前鼠标Y
    }mouse;

    bool                refresh;
}wsdl_extension_t;


//////////////////////////////////////////////////////////////////////////

static void wsdl_extension_init(wsdl_extension_t* p_ex, const char* p_font_path)
{
    // 基础路径
    char* p_base_path = SDL_GetBasePath();
    p_ex->base_path = sdsnew(p_base_path);
    KLB_FREE_BY(p_base_path, SDL_free);

    // 图片
    p_ex->p_imgs = wsdl_images_create();

    // 字体 (仅创建; 加载走 canvas load_font)
    (void)p_font_path;
    p_ex->p_ft = ft_raster_create();
}

static void wsdl_extension_quit(wsdl_extension_t* p_ex)
{
    KLB_FREE_BY(p_ex->p_ft, ft_raster_destroy);
    KLB_FREE_BY(p_ex->p_imgs, wsdl_images_destroy);
    KLB_FREE_BY(p_ex->base_path, sdsfree);
}

void* wsdl_extension_create(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = KLB_MALLOCZ(wsdl_extension_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->p_wnd = wsdl_wnd_create();

    p_ex->p_video = wsdl_video_create();

    p_ex->refresh = false;

    return p_ex;
}

void wsdl_extension_destroy(void* ptr)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;


    kluaex_wsdl_close_wnd(p_ex);

    KLB_FREE_BY(p_ex->p_video, wsdl_video_destroy);

    KLB_FREE_BY(p_ex->p_wnd, wsdl_wnd_destroy);
    KLB_FREE(p_ex);
}

// SDL滚轮无窗口坐标; 取当前鼠标位置, 规范化方向后投入 KLBUI_mousewheel
static void wsdl_extension_push_mousewheel(wsdl_extension_t* p_ex, const SDL_MouseWheelEvent* p_wheel)
{
    int mx = 0;
    int my = 0;
    int dy = p_wheel->y;
    float precise_y = p_wheel->preciseY;

    SDL_GetMouseState(&mx, &my);
    p_ex->mouse.x = mx;
    p_ex->mouse.y = my;

    if (SDL_MOUSEWHEEL_FLIPPED == p_wheel->direction)
    {
        dy = -dy;
        precise_y = -precise_y;
    }

    if (0 == dy)
    {
        if (precise_y > 0.0f)
        {
            dy = 1;
        }
        else if (precise_y < 0.0f)
        {
            dy = -1;
        }
    }

    if (0 != dy)
    {
        klb_gui_push_msg(p_ex->p_gui, KLBUI_mousewheel, mx, my, 0, 0, dy, 0);
    }
}

int wsdl_extension_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;

    if (!wsdl_wnd_is_open(p_ex->p_wnd))
    {
        return 0;
    }

    int done = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
    {
        if (event.type == SDL_QUIT)
        {
            done = 1;
        }

        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                //done = 1;
            }

            switch (event.key.keysym.sym)
            {
            case SDLK_F1:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 1, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F2:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 2, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F3:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 3, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F4:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 4, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F5:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 5, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F6:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 6, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F7:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 7, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F8:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 8, 0, 0, 0, 0, 0, 0);
                break;
            case SDLK_F9:
                klb_gui_push_msg(p_ex->p_gui, KLBUI_event_user + 9, 0, 0, 0, 0, 0, 0);
                break;
            default:
                break;
            }
        }

        if (SDL_WINDOWEVENT == event.type)
        {
            if (SDL_WINDOWEVENT_EXPOSED == event.window.event)
            {
                wsdl_wnd_render_present(p_ex->p_wnd);
            }
        }

        if (NULL != p_ex->p_gui)
        {
            if (SDL_MOUSEMOTION == event.type)
            {
                // 鼠标移动
                p_ex->mouse.x = event.motion.x;
                p_ex->mouse.y = event.motion.y;
                klb_gui_push_msg(p_ex->p_gui, KLBUI_mousemove, event.motion.x, event.motion.y, 0, 0, 0, 0);

                if (p_ex->mouse.btn_left)
                {
                    // 拖拽事件开始
                    if (!p_ex->mouse.is_drag)
                    {
                        p_ex->mouse.is_drag = true;
                        p_ex->mouse.drag_sx = event.motion.x;
                        p_ex->mouse.drag_sy = event.motion.y;

                        klb_gui_push_msg(p_ex->p_gui, KLBUI_mousedrag, event.motion.x, event.motion.y, p_ex->mouse.drag_sx, p_ex->mouse.drag_sy, KLBUI_MOUSEDRAG_start, KLBUI_MOUSE_left);
                    }
                    else
                    {
                        klb_gui_push_msg(p_ex->p_gui, KLBUI_mousedrag, event.motion.x, event.motion.y, p_ex->mouse.drag_sx, p_ex->mouse.drag_sy, KLBUI_MOUSEDRAG_move, KLBUI_MOUSE_left);
                    }
                }
            }
            else if (SDL_MOUSEBUTTONDOWN == event.type)
            {
                if (1 == event.button.button)
                {
                    p_ex->mouse.btn_left = true;

                    // 左键
                    klb_gui_push_msg(p_ex->p_gui, KLBUI_click, event.motion.x, event.motion.y, 0, 0, 0, 0);
                }
                else if (3 == event.button.button)
                {
                    // 右键
                    klb_gui_push_msg(p_ex->p_gui, KLBUI_mousedown, event.motion.x, event.motion.y, 0, 0, KLBUI_MOUSE_right, 0);
                }
            }
            else if (SDL_MOUSEBUTTONUP == event.type)
            {
                if (1 == event.button.button)
                {
                    p_ex->mouse.btn_left = false;

                    if (p_ex->mouse.is_drag)
                    {
                        p_ex->mouse.is_drag = false;
                        klb_gui_push_msg(p_ex->p_gui, KLBUI_mousedrag, event.motion.x, event.motion.y, p_ex->mouse.drag_sx, p_ex->mouse.drag_sy, KLBUI_MOUSEDRAG_end, KLBUI_MOUSE_left);
                    }
                }
            }
            else if (SDL_MOUSEWHEEL == event.type)
            {
                wsdl_extension_push_mousewheel(p_ex, &event.wheel);
            }
        }
    }

    if (p_ex->refresh)
    {
        p_ex->refresh = false;
    }

    if (0 != done)
    {
        klua_env_exit(p_ex->p_env);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int kluaex_register_wsdl(klua_env_t* p_env)
{
    // 注册 lua env 扩展
    klua_env_extension_t ex = { 0 };

    ex.cb_create = wsdl_extension_create;
    ex.cb_destroy = wsdl_extension_destroy;
    ex.cb_loop_once = wsdl_extension_loop_once;

    klua_env_register_extension(p_env, WSDL_EXTENSION, &ex);

    return 0;
}

wsdl_extension_t* kluaex_get_wsdl(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    if (NULL == p_ex)
    {
        kluaex_register_wsdl(p_env);
        p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    }

    return p_ex;
}

wsdl_extension_t* kluaex_get_wsdl_by_L(lua_State* L)
{
    return kluaex_get_wsdl(klua_env_get_by_L(L));
}

//////////////////////////////////////////////////////////////////////////

int kluaex_wsdl_open_wnd(wsdl_extension_t* p_ex, int w, int h, const char* p_title)
{
    // 初始化
    p_ex->w = w;
    p_ex->h = h;

    // gui
    p_ex->p_gui = klua_gui_get(p_ex->p_env);

    // 打开窗口
    wsdl_wnd_open(p_ex->p_wnd, p_ex->p_gui, w, h, p_title);

    // 附加画布
    klb_gui_attach_canvas(p_ex->p_gui, wsdl_wnd_get_ui_canvas(p_ex->p_wnd));

    // 刷新
    p_ex->refresh = true;

    return 0;
}

int kluaex_wsdl_close_wnd(wsdl_extension_t* p_ex)
{
    // 退出窗口
    wsdl_wnd_close(p_ex->p_wnd);

    return 0;
}
