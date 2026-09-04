// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "ft_cache.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nvector.h"


#define FT_CACHE_ITEM_MAX       3000        // 缓存字符 最大数目


typedef struct ft_cache_item_t_
{
    // Bug. 字形记录 还与字体大小有关
    // 1. 字体大小;  存放 高32位
    // 2. unicode 字符ID; 存放 低32位
    uint64_t    ch_font;        // 字体大小 - unicode 字符ID
    FT_Glyph    glyph;          // freetype2 字形

    int         use_count;      // 使用次数记录
}ft_cache_item_t;


/// @struct ft_cache_t
/// @brief  cache 模块
///  1. 缓存字符 图形
///  2. 更换维护缓存
typedef struct ft_cache_t_
{
    klb_hlist_t*    p_glyph_hlist;      // 图形 hash list
}ft_cache_t;


static int cb_ft_cache_clear(void* p_obj, void* p_data)
{
    ft_cache_item_t* p_item = (ft_cache_item_t*)p_data;

    (void)p_obj;

    FT_Done_Glyph(p_item->glyph);
    KLB_FREE(p_item);

    return 0;
}


static uint64_t ft_cache_make_key(uint32_t ch, int font_h)
{
    uint64_t h = (uint64_t)font_h;

    return (h << 32) | ((uint64_t)ch);
}


static int cb_sort_ft_cache_item(const void* p_data1, const void* p_data2, void* ptr1, void* ptr2)
{
    const ft_cache_item_t* p_a = (const ft_cache_item_t*)p_data1;
    const ft_cache_item_t* p_b = (const ft_cache_item_t*)p_data2;

    (void)ptr1;
    (void)ptr2;

    if (p_a->use_count < p_b->use_count)
    {
        return -1;
    }
    else if (p_a->use_count > p_b->use_count)
    {
        return 1;
    }

    return 0;
}


ft_cache_t* ft_cache_create()
{
    ft_cache_t* p_cache = KLB_MALLOCZ(ft_cache_t, 1, 0);

    p_cache->p_glyph_hlist = klb_hlist_create(0);

    return p_cache;
}

void ft_cache_destroy(ft_cache_t* p_cache)
{
    if (NULL != p_cache->p_glyph_hlist)
    {
        klb_hlist_clear(p_cache->p_glyph_hlist, cb_ft_cache_clear, p_cache);

        klb_hlist_destroy(p_cache->p_glyph_hlist);
        p_cache->p_glyph_hlist = NULL;
    }

    KLB_FREE(p_cache);
}

FT_Glyph ft_cache_get_glyph(ft_cache_t* p_cache, uint32_t ch, int font_h)
{
    uint64_t key = ft_cache_make_key(ch, font_h);
    ft_cache_item_t* p_item = (ft_cache_item_t*)klb_hlist_find(p_cache->p_glyph_hlist, &key, (int)sizeof(key));

    if (NULL != p_item)
    {
        p_item->use_count += 1;

        return p_item->glyph;
    }

    return NULL;
}

static void limit_ft_cache(ft_cache_t* p_cache)
{
    klb_hlist_t* p_hlist = p_cache->p_glyph_hlist;
    int size = klb_hlist_size(p_hlist);

    if (size <= 0)
    {
        return;
    }

    // todo. 替换
    klb_nvector_t* p_vector = klb_nvector_create();

    while (0 < klb_hlist_size(p_hlist))
    {
        ft_cache_item_t* p_item = (ft_cache_item_t*)klb_hlist_pop_head(p_hlist);

        klb_nvector_push_tail(p_vector, p_item);
    }

    klb_nvector_sort(p_vector, cb_sort_ft_cache_item, NULL, NULL);

    int vector_size = klb_nvector_size(p_vector);
    int pos = vector_size / 2;
    int use_count = ((ft_cache_item_t*)klb_nvector_get(p_vector, pos))->use_count;

    for (int i = 0; i < pos; i++)
    {
        ft_cache_item_t* p_item = (ft_cache_item_t*)klb_nvector_get(p_vector, i);

        FT_Done_Glyph(p_item->glyph);
        KLB_FREE(p_item);
    }

    for (int k = pos; k < vector_size; k++)
    {
        ft_cache_item_t* p_item = (ft_cache_item_t*)klb_nvector_get(p_vector, k);
        uint64_t key = p_item->ch_font;

        p_item->use_count -= use_count;

        klb_hlist_push_tail(p_hlist, &key, (int)sizeof(key), p_item);
    }

    klb_nvector_clear(p_vector, NULL, NULL);
    klb_nvector_destroy(p_vector);
}

int ft_cache_push(ft_cache_t* p_cache, uint32_t ch, int font_h, FT_Glyph glyph)
{
    if (NULL == glyph)
    {
        return 2; // 错误参数
    }

    uint64_t key = ft_cache_make_key(ch, font_h);

    ft_cache_item_t* p_item = (ft_cache_item_t*)klb_hlist_find(p_cache->p_glyph_hlist, &key, (int)sizeof(key));
    if (NULL != p_item)
    {
        return 1; // 已存在
    }

    p_item = KLB_MALLOCZ(ft_cache_item_t, 1, 0);
    p_item->ch_font = key;
    p_item->glyph = glyph;
    p_item->use_count = 0;

    if (NULL == klb_hlist_push_tail(p_cache->p_glyph_hlist, &key, (int)sizeof(key), p_item))
    {
        FT_Done_Glyph(glyph);
        KLB_FREE(p_item);
        return 1;
    }

    if (FT_CACHE_ITEM_MAX < klb_hlist_size(p_cache->p_glyph_hlist))
    {
        limit_ft_cache(p_cache);
    }

    return 0;
}

// end
