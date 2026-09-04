#include "wsdl.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_rect.h"
#include "SDL.h"
#include "wsdl_extension.h"
#include <assert.h>


// 获取SDL版本信息
static int wsdl_SDL_GetVersion(lua_State* L)
{
    SDL_version ver = { 0 };
    SDL_GetVersion(&ver);

    lua_pushfstring(L, "V%d.%d.%d", ver.major, ver.minor, ver.patch);
    return 1;
}

// 获取SDL-git版本信息
static int wsdl_SDL_GetRevision(lua_State* L)
{
    lua_pushstring(L, SDL_GetRevision());
    return 1;
}

// 获取可执行文件的路径
static int wsdl_SDL_GetBasePath(lua_State* L)
{
    char* p_base_path = SDL_GetBasePath();

    lua_pushstring(L, p_base_path);

    KLB_FREE_BY(p_base_path, SDL_free);
    return 1;
}

// 获取系统分配的应用配置路径
static int wsdl_SDL_GetPrefPath(lua_State* L)
{
    const char* p_org = luaL_checkstring(L, 1);
    const char* p_app = luaL_checkstring(L, 2);

    char* p_pref_path = SDL_GetPrefPath(p_org, p_app);

    lua_pushstring(L, p_pref_path);

    KLB_FREE_BY(p_pref_path, SDL_free);
    return 1;
}

// 打开窗口
static int wsdl_OpenWnd(lua_State* L)
{
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    const char* p_title = luaL_checkstring(L, 3);

    // 打开窗口
    kluaex_wsdl_open_wnd(kluaex_get_wsdl_by_L(L), w, h, p_title);

    return 0;
}

// 关闭窗口
static int wsdl_CloseWnd(lua_State* L)
{
    // 关闭窗口
    kluaex_wsdl_close_wnd(kluaex_get_wsdl_by_L(L));

    return 0;
}

int luaopen_wsdl(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "get_version",        wsdl_SDL_GetVersion },
        { "get_revision",       wsdl_SDL_GetRevision },

        { "get_base_path",      wsdl_SDL_GetBasePath },
        { "get_pref_path",      wsdl_SDL_GetPrefPath },

        // 窗口
        { "open_wnd",           wsdl_OpenWnd },
        { "close_wnd",          wsdl_CloseWnd },


        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
