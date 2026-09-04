#ifndef __FT_CACHE_H__
#define __FT_CACHE_H__


#include <stdint.h>     /// int*_t uint*_t
#include <stddef.h>     /// intptr_t uintptr_t size_t
#include <stdbool.h>    /// bool false true

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct ft_cache_t_ ft_cache_t;


/// @brief 创建/销毁
ft_cache_t* ft_cache_create();
void ft_cache_destroy(ft_cache_t* p_cache);


/// @brief 获取字符ch 的 图形 数据
FT_Glyph ft_cache_get_glyph(ft_cache_t* p_cache, uint32_t ch, int font_h);


/// @brief 放入图形数据
///  经优化: 调用FT_Glyph_To_Bitmap()函数 栅格化 之后 的 glyph 缓存起来
int ft_cache_push(ft_cache_t* p_cache, uint32_t ch, int font_h, FT_Glyph glyph);


#if defined(__cplusplus)
}
#endif

#endif // __FT_CACHE_H__
//end
