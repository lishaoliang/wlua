// Doc Encode : UTF-8 BOM, Unix(LF)
#include "wsdl_images.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_color.h"
#include "klbgui/klb_wnd_ex.h"
#include "klbformat/klb_bitmap.h"
#include "klbformat/klb_png.h"
#include <string.h>


typedef struct wsdl_img_item_t_
{
    SDL_Surface* p_surface;
    //SDL_Texture* p_texture;
}wsdl_img_item_t;

typedef struct wsdl_images_t_
{
    klb_hlist_t*        p_hlist;

    wsdl_img_item_t*    p_tmpimage;     ///< 临时图片
}wsdl_images_t;


wsdl_images_t* wsdl_images_create()
{
    wsdl_images_t* p_imgs = KLB_MALLOCZ(wsdl_images_t, 1, 0);

    p_imgs->p_hlist = klb_hlist_create(0);

    p_imgs->p_tmpimage = NULL;

    return p_imgs;
}

static void free_wsdl_img_item(wsdl_img_item_t* p_item)
{
    //KLB_FREE_BY(p_item->p_texture, SDL_DestroyTexture);
    KLB_FREE_BY(p_item->p_surface, SDL_FreeSurface);
    KLB_FREE(p_item);
}

void wsdl_images_destroy(wsdl_images_t* p_imgs)
{
    wsdl_images_clear(p_imgs);

    KLB_FREE_BY(p_imgs->p_tmpimage, free_wsdl_img_item);
    KLB_FREE_BY(p_imgs->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_imgs);
}

static bool is_ext_wsdl_images(const char* p_path, const char* p_ext)
{
    if (NULL == p_path || NULL == p_ext)
    {
        return false;
    }

    int path_len = (int)strlen(p_path);
    int ext_len = (int)strlen(p_ext);
    if (path_len < ext_len)
    {
        return false;
    }

    const char* p = p_path + (path_len - ext_len);
    for (int i = 0; i < ext_len; i++)
    {
        char a = p[i];
        char b = p_ext[i];
        if ('A' <= a && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }

        if ('A' <= b && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }

        if (a != b)
        {
            return false;
        }
    }

    return true;
}


static SDL_Surface* canvas_to_surface_wsdl_images(const klb_canvas_t* p_canvas)
{
    if (NULL == p_canvas || NULL == p_canvas->p_addr)
    {
        return NULL;
    }

    int w = p_canvas->rect.w;
    int h = p_canvas->rect.h;
    SDL_Surface* p_surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (NULL == p_surface || NULL == p_surface->pixels)
    {
        KLB_FREE_BY(p_surface, SDL_FreeSurface);
        return NULL;
    }

    int src_pitch = (int)p_canvas->pitch;
    int dst_pitch = p_surface->pitch;
    int row_bytes = w * 4;
    if (src_pitch < row_bytes)
    {
        row_bytes = src_pitch;
    }

    if (dst_pitch < row_bytes)
    {
        row_bytes = dst_pitch;
    }

    for (int y = 0; y < h; y++)
    {
        memcpy((uint8_t*)p_surface->pixels + (int64_t)y * dst_pitch, p_canvas->p_addr + (int64_t)y * src_pitch, (size_t)row_bytes);
    }

    return p_surface;
}


static SDL_Surface* load_surface_wsdl_images(const char* p_path)
{
    if (NULL == p_path)
    {
        return NULL;
    }

    klb_canvas_t* p_canvas = NULL;
    if (is_ext_wsdl_images(p_path, ".png"))
    {
        p_canvas = klb_png_read(p_path, KLB_COLOR_FMT_ARGB8888);
    }
    else if (is_ext_wsdl_images(p_path, ".bmp"))
    {
        p_canvas = klb_bitmap_read(p_path, KLB_COLOR_FMT_ARGB8888);
    }
    else
    {
        return NULL;
    }

    if (NULL == p_canvas)
    {
        return NULL;
    }

    SDL_Surface* p_surface = canvas_to_surface_wsdl_images(p_canvas);
    klb_canvas_destroy(p_canvas);
    return p_surface;
}

static wsdl_img_item_t* load_image_wsdl_images(const char* p_path)
{
    SDL_Surface* p_surface = load_surface_wsdl_images(p_path);

    if (NULL == p_surface)
    {
        return NULL;
    }

    SDL_Surface* p_dst = SDL_ConvertSurfaceFormat(p_surface, SDL_PIXELFORMAT_ARGB8888, 0);
    KLB_FREE_BY(p_surface, SDL_FreeSurface);

    if (NULL == p_dst)
    {
        return NULL;
    }

    wsdl_img_item_t* p_item = KLB_MALLOCZ(wsdl_img_item_t, 1, 0);
    p_item->p_surface = p_dst;

    return p_item;
}

int wsdl_images_load(wsdl_images_t* p_imgs, const char* p_key, const char* p_path)
{
    int key_len = strlen(p_key);

    wsdl_img_item_t* p_item = load_image_wsdl_images(p_path);
    if (NULL == p_item)
    {
        return 1;
    }

    if (NULL == klb_hlist_push_tail(p_imgs->p_hlist, p_key, key_len, p_item))
    {
        wsdl_img_item_t* p_old = (wsdl_img_item_t*)klb_hlist_update(p_imgs->p_hlist, p_key, key_len, p_item);
        KLB_FREE_BY(p_old, free_wsdl_img_item);
    }

    return 0;
}

int wsdl_images_load_tmpimage(wsdl_images_t* p_imgs, const char* p_path)
{
    KLB_FREE_BY(p_imgs->p_tmpimage, free_wsdl_img_item);

    p_imgs->p_tmpimage = load_image_wsdl_images(p_path);
    if (NULL == p_imgs->p_tmpimage)
    {
        return 1;
    }

    return 0;
}

static int cb_wsdl_images_clear(void* p_obj, void* p_data)
{
    wsdl_images_t* p_imgs = (wsdl_images_t*)p_obj;
    wsdl_img_item_t* p_item = (wsdl_img_item_t*)p_data;

    KLB_FREE_BY(p_item, free_wsdl_img_item);

    return 0;
}

int wsdl_images_clear(wsdl_images_t* p_imgs)
{
    klb_hlist_clear(p_imgs->p_hlist, cb_wsdl_images_clear, p_imgs);

    return 0;
}

SDL_Surface* wsdl_images_find(wsdl_images_t* p_imgs, const char* p_path)
{
    wsdl_img_item_t* p_item = (wsdl_img_item_t*)klb_hlist_find(p_imgs->p_hlist, p_path, strlen(p_path));

    if (NULL != p_item)
    {
        return p_item->p_surface;
    }

    // 临时图片
    if ( NULL != p_imgs->p_tmpimage && 0 == strcmp(KLB_CANVAS_PATH_TMPIMAGE, p_path))
    {
        return p_imgs->p_tmpimage->p_surface;
    }

    return NULL;
}

int wsdl_images_size(wsdl_images_t* p_imgs, const char* p_path, int* p_out_w, int* p_out_h)
{
    wsdl_img_item_t* p_item = (wsdl_img_item_t*)klb_hlist_find(p_imgs->p_hlist, p_path, strlen(p_path));

    if (NULL == p_item)
    {
        // 临时图片
        if (NULL != p_imgs->p_tmpimage && 0 == strcmp(KLB_CANVAS_PATH_TMPIMAGE, p_path))
        {
            p_item = p_imgs->p_tmpimage;
        }
    }

    if (NULL != p_out_w)
    {
        *p_out_w = (NULL != p_item) ? p_item->p_surface->w : 0;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = (NULL != p_item) ? p_item->p_surface->h : 0;
    }

    return 0;
}
