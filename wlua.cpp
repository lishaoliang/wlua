#include "SDL.h"
#include <string.h>
#include <stdlib.h>
#include "klua/klua.h"
#include "klbbase/klb_base.h"
#include "klbapp/klb_app.h"
#include "wlua_config.h"
#include "wsdl.h"

//#if defined(WIN32) && defined(DEBUG_VLD)
//#include "vld.h"
//#endif

static int wlua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);

    klua_loadlib(L, luaopen_wsdl, "wsdl");

    return 0;
}

// Win32: 当前目录及 "./lib" "./libs"; Linux: LD_LIBRARY_PATH 各段
static void wlua_setup_plugins_paths(void)
{
    klb_app_enable_plugins(true);

#if defined(_WIN32) || defined(WIN32)
    klb_app_push_plugins_path(".");
    klb_app_push_plugins_path("./lib");
    klb_app_push_plugins_path("./libs");
#else
    const char* ld_path = getenv("LD_LIBRARY_PATH");
    if (NULL != ld_path && '\0' != ld_path[0])
    {
        char* dup = strdup(ld_path);
        if (NULL != dup)
        {
            char* token = dup;
            while (NULL != token && '\0' != *token)
            {
                char* colon = strchr(token, ':');
                if (NULL != colon)
                {
                    *colon = '\0';
                }

                if ('\0' != token[0])
                {
                    klb_app_push_plugins_path(token);
                }

                token = (NULL != colon) ? colon + 1 : NULL;
            }

            free(dup);
        }
    }
#endif
}

int main(int argc, char** argv)
{
    klb_base_init(NULL);            // klb
    SDL_Init(SDL_INIT_EVERYTHING);  // SDL

    wlua_setup_plugins_paths();

    int ret = klb_app_main(argc, argv, wlua_openlibs);

    SDL_Quit();
    klb_base_quit();

    return ret;
}

// end
