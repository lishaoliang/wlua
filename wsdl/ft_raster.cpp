// Doc Encode : UTF-8 BOM, Unix(LF)
#include "ft_raster.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"
#include "ft_cache.h"
#include <math.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


// 使用字符缓存优化
#define FT_USE_CACHE        1
//#define FT_USE_CACHE        0


// 32位数26.6, 前26位整数部分; 后6位小数部分
#define FT_int32(i32_)      ((i32_) << 6)

// dpi
#define FT_dpi              72

// 忽略嵌入点阵, 统一轮廓抗锯齿; simsun 16px CJK sbit 为 MONO, 直接画会花屏
#define FT_LOAD_GUI             (FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP)
#define FT_LOAD_GUI_RENDER      (FT_LOAD_RENDER | FT_LOAD_NO_BITMAP)

//utf8 -> unicode
#define FT_utf8_to_unicode(UTF8_, UNICODE_, LEN_) \
{ \
    int nnn = 0; \
    unsigned char uft8_ch = UTF8_[nnn++]; \
    if((uft8_ch & 0x80) == 0) \
    { \
        UNICODE_ = uft8_ch; \
        LEN_ = 1; \
    } \
    else if((uft8_ch & 0xe0) == 0xe0) \
    { \
        UNICODE_ = (uft8_ch & 0x1F) << 12; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 3; \
    } \
    else \
    { \
        UNICODE_ = (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 2; \
    } \
}

#define FT_ARGB8888_TO_1555(color) \
    ((uint16_t) \
    (   (((color >> 24) & 0xFF) ? 0x8000 : 0x0) | \
    ((( ((color >> 16) & 0xFF) & 0xFF) >> 3) << 10) | \
    ((( ((color >> 8) & 0xFF)  & 0xFF) >> 3) << 5) | \
    ((  (color & 0xFF)         & 0xFF) >> 3)))

typedef struct ft_raster_t_
{
    FT_Library      library;
    FT_Face         face;

    FT_GlyphSlot    slot;

    ft_cache_t*     p_cache;            // 缓存
    bool            font_loaded;        ///< 字库已加载, load_font 之后方可绘制/测算
}ft_raster_t;


static bool ft_raster_font_ready(const ft_raster_t* p_ft)
{
    return (NULL != p_ft) && p_ft->font_loaded;
}


ft_raster_t* ft_raster_create()
{
    ft_raster_t* p_ft = KLB_MALLOCZ(ft_raster_t, 1, 0);

    FT_Error err = FT_Init_FreeType(&p_ft->library);                /* initialize library */
    assert(0 == err);

    p_ft->p_cache = ft_cache_create();

    return p_ft;
}

int ft_raster_load_font(ft_raster_t* p_ft, const char* p_font_path)
{
    assert(NULL != p_ft);
    assert(NULL != p_font_path);

    if (NULL != p_ft->face)
    {
        p_ft->font_loaded = true;
        return 0;
    }

    FT_Error err = FT_New_Face(p_ft->library, p_font_path, 0, &p_ft->face);  /* create face object */
    if (0 != err)
    {
        p_ft->font_loaded = false;
        return 1;
    }

    p_ft->slot = p_ft->face->glyph;

    if (NULL == p_ft->p_cache)
    {
        p_ft->p_cache = ft_cache_create();
    }

    p_ft->font_loaded = true;
    return 0;
}

int ft_raster_unload_font(ft_raster_t* p_ft)
{
    assert(NULL != p_ft);

    if (NULL == p_ft->face)
    {
        p_ft->font_loaded = false;
        return 0;
    }

    KLB_FREE_BY(p_ft->p_cache, ft_cache_destroy);
    p_ft->p_cache = NULL;

    KLB_FREE_BY(p_ft->face, FT_Done_Face);
    p_ft->slot = NULL;
    p_ft->font_loaded = false;

    return 0;
}

bool ft_raster_is_font_loaded(ft_raster_t* p_ft)
{
    return ft_raster_font_ready(p_ft);
}

void ft_raster_destroy(ft_raster_t* p_ft)
{
    assert(NULL != p_ft);

    ft_raster_unload_font(p_ft);

    KLB_FREE_BY(p_ft->library, FT_Done_FreeType);
    KLB_FREE(p_ft);
}

// 取覆盖值; MONO 为 1bit MSB; 其它按 GRAY, 行距用 pitch
static uint32_t ft_bitmap_cover(const FT_Bitmap* bitmap, FT_Int p, FT_Int q)
{
    const uint8_t* row = bitmap->buffer + q * bitmap->pitch;

    if (FT_PIXEL_MODE_MONO == bitmap->pixel_mode)
    {
        if (0 != (row[p >> 3] & (0x80 >> (p & 7))))
        {
            return 255;
        }

        return 0;
    }

    return row[p];
}

static void draw_bitmap(ft_raster_pixels_t* p_raster, FT_Bitmap* bitmap, FT_Int x, FT_Int y, uint32_t color, FT_Int right, FT_Int bottom)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;

    for (i = x, p = 0; i < x_max; i++, p++)
    {
        for (j = y, q = 0; j < y_max; j++, q++)
        {
            if (i < 0 || j < 0 ||
                i >= p_raster->w || j >= p_raster->h ||
                i >= right || j >= bottom)
            {
                continue;
            }

            uint32_t v = ft_bitmap_cover(bitmap, p, q);
            if (0 != v)
            {
                uint32_t r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = (color) & 0xFF;

#if 0
                // 直接替换文字颜色
                r = r * v / 255;
                g = g * v / 255;
                b = b * v / 255;

                uint32_t argb = KLB_ARGB8888(255, r, g, b);
                uint8_t* ptr = p_raster->p_pixels + p_raster->pitch * j + i * p_raster->bpp;

                // [j][i]
                if (4 == p_raster->bpp)
                {
                    memcpy(ptr, &argb, sizeof(uint32_t));
                }
                else
                {
                    uint16_t argb1555 = FT_ARGB8888_TO_1555(argb);
                    memcpy(ptr, &argb1555, sizeof(uint16_t));
                }
#else
                // 简易 Alpha融合 算法; eg. "https://zhuanlan.zhihu.com/p/661538470"
                // todo. 这里还需要优化执行效率
                // output = foreground * mask + background * (1 - mask)
                uint8_t* ptr = p_raster->p_pixels + p_raster->pitch * j + i * p_raster->bpp;

                if (4 == p_raster->bpp)
                {
                    uint32_t old_color = 0;
                    memcpy(&old_color, ptr, sizeof(uint32_t));

                    uint32_t old_a = (old_color >> 24) & 0xFF;
                    uint32_t old_r = (old_color >> 16) & 0xFF, old_g = (old_color >> 8) & 0xFF, old_b = (old_color) & 0xFF;

                    uint32_t new_r = r;
                    uint32_t new_g = g;
                    uint32_t new_b = b;

                    // 灰度值小于 255
                    if (v < 255)
                    {
                        if (0 < old_a)
                        {
                            new_r = (r * v + old_r * (255 - v)) / 255;
                            new_g = (g * v + old_g * (255 - v)) / 255;
                            new_b = (b * v + old_b * (255 - v)) / 255;
                        }
                        else
                        {
                            // 若底色 alpha 为0, 则 认为在固定 底色(255, 26, 26, 26) 上绘制
                            new_r = (r * v + 26 * (255 - v)) / 255;
                            new_g = (g * v + 26 * (255 - v)) / 255;
                            new_b = (b * v + 26 * (255 - v)) / 255;
                        }

                        if (255 < new_r) new_r = 255;
                        if (255 < new_g) new_g = 255;
                        if (255 < new_b) new_b = 255;
                    }

                    uint32_t argb = KLB_ARGB8888(255, new_r, new_g, new_b);

                    memcpy(ptr, &argb, sizeof(uint32_t));
                }
                else
                {
                    uint16_t old_color = 0;
                    memcpy(&old_color, ptr, sizeof(uint16_t));

                    uint32_t old_a = (old_color >> 15) & 0x1;
                    uint32_t old_r = (old_color >> 10) & 0x1F, old_g = (old_color >> 5) & 0x1F, old_b = (old_color) & 0x1F;

                    uint32_t new_r = r;
                    uint32_t new_g = g;
                    uint32_t new_b = b;

                    // 灰度值小于 255
                    if (v < 255)
                    {
                        if (0 < old_a)
                        {
                            new_r = (r * v + old_r * (255 - v)) / 255;
                            new_g = (g * v + old_g * (255 - v)) / 255;
                            new_b = (b * v + old_b * (255 - v)) / 255;
                        }
                        else
                        {
                            // 若底色 alpha 为0, 则 认为在固定 底色(255, 26, 26, 26) 上绘制
                            new_r = (r * v + 26 * (255 - v)) / 255;
                            new_g = (g * v + 26 * (255 - v)) / 255;
                            new_b = (b * v + 26 * (255 - v)) / 255;
                        }

                        if (255 < new_r) new_r = 255;
                        if (255 < new_g) new_g = 255;
                        if (255 < new_b) new_b = 255;
                    }

                    uint32_t argb = KLB_ARGB8888(255, new_r, new_g, new_b);
                    uint16_t argb1555 = FT_ARGB8888_TO_1555(argb);

                    memcpy(ptr, &argb1555, sizeof(uint16_t));
                }
#endif
            }
        }
    }
}

// 获取 字符 对应的 图形glyph
static FT_Glyph get_glyph_ft_raster(ft_raster_t* p_ft, uint32_t ch, int font_h)
{
    // 从缓存中获取
    FT_Glyph glyph = ft_cache_get_glyph(p_ft->p_cache, ch, font_h);

    if (NULL == glyph)
    {
        // 若缓存中没有, 则 在 FT 中 获取 字符码 对应的 图形序号
        // 并加载 字符图形
        FT_UInt glyph_index = FT_Get_Char_Index(p_ft->face, ch);
        FT_Error err_load = FT_Load_Glyph(p_ft->face, glyph_index, FT_LOAD_GUI);

        if (0 == err_load)
        {
            // 获取一份字符图形拷贝
            FT_Glyph glyph_src = NULL;
            FT_Error err_get = FT_Get_Glyph(p_ft->face->glyph, &glyph_src);

            if (0 == err_get)
            {
                // step. 将矢量字符图形 栅格化 => 转换成 bmp 文件
                FT_Error err_bmp = FT_Glyph_To_Bitmap(&glyph_src, FT_RENDER_MODE_NORMAL, NULL, 1);

                // 若栅格化成功, 则放入缓存
                if (0 == err_bmp && 0 == ft_cache_push(p_ft->p_cache, ch, font_h, glyph_src))
                {
                    // 成功放入, 缓存 直接引用
                    glyph = glyph_src;
                }
                else
                {
                    // 放入失败; 释放
                    FT_Done_Glyph(glyph_src);
                }
            }
        }
    }

    return glyph;
}

int ft_raster_text(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int* p_out_w, int* p_out_h, bool* p_out_draw_all)
{
    if (!ft_raster_font_ready(p_ft))
    {
        if (NULL != p_out_w)
        {
            *p_out_w = 0;
        }

        if (NULL != p_out_h)
        {
            *p_out_h = 0;
        }

        if (NULL != p_out_draw_all)
        {
            *p_out_draw_all = false;
        }

        return 1;
    }

    FT_GlyphSlot slot = p_ft->face->glyph;

    /* use font_h pt at 72dpi */
    int error = FT_Set_Char_Size(p_ft->face, FT_int32(font_h), FT_int32(font_h), FT_dpi, FT_dpi);     /* set character size */

    FT_Matrix   matrix;         /* transformation matrix */

    double angle = 0.0;
    matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

    // freetype 采用的英文基准线
    int offset_y = 4;
    if (28 < font_h)
    {
        offset_y = 6;
    }

    FT_Vector   pen;            /* untransformed origin  */

    /* the pen position in 26.6 cartesian space coordinates; */
    /* start at (x,y+h) relative to the upper left corner  */
    pen.x = FT_int32(x);
    //pen.y = FT_int32(p_raster->h - (y + h) + offset_y);
    pen.y = FT_int32(p_raster->h - (y + h) + offset_y + (h - font_h) / 2); // 居中

#if FT_USE_CACHE
    // freetype2接口 高阶用法
    // 翻译: https://zhuanlan.zhihu.com/p/84779973
    // 原文: https://freetype.org/freetype2/docs/tutorial/step2.html

    FT_Vector zero = { 0 };
    FT_Set_Transform(p_ft->face, &matrix, &zero);

    for (int i = 0; i < utf8_len; )
    {
        // step1. 从 utf8 中读取 单个 unicode 字符码
        uint32_t ch = 0;
        int len = 0;
        FT_utf8_to_unicode((p_utf8 + i), ch, len);

        if (utf8_len < i + len)
        {
            break;
        }

        // 获取 字符码 对应的 字符图形
        FT_Glyph glyph = get_glyph_ft_raster(p_ft, ch, font_h); 
        if (NULL == glyph)
        {
            // 在字库中未找到字符, 则使用 ? 替代
            glyph = get_glyph_ft_raster(p_ft, '?', font_h);
        }
        
        if (NULL != glyph)
        {
            // step3. 不变换缓存字形; 画笔 + bearing 定位
            {
                FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

                // bitmap_glyph->left / top 相对 (0,0); 真坐标 = 画笔 + 偏移
                int left = (pen.x >> 6) + bitmap_glyph->left;
                int top = p_raster->h - ((pen.y >> 6) + bitmap_glyph->top);

                // 绘制字符 
                // 参数: x + w    X轴最大边界
                // 参数: y + h    Y轴最大边界
                draw_bitmap(p_raster, &bitmap_glyph->bitmap, left, top, color, x + w, y + h);
            }

            // stepN. 移动画笔
            pen.x += (glyph->advance.x >> 10);
            pen.y += (glyph->advance.y >> 10);
        }

        // 偏移位置
        i += len;
    }

#else

    // 简易 文本绘制

    for (int i = 0; i < utf8_len; )
    {
        /* set transformation */
        FT_Set_Transform(p_ft->face, &matrix, &pen);

        uint32_t ch = 0;
        int len = 0;
        FT_utf8_to_unicode((p_utf8 + i), ch, len);

        if (utf8_len < i + len)
        {
            break;
        }

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(p_ft->face, ch, FT_LOAD_GUI_RENDER);
        if (error)
        {
            i += len;
            continue;                 /* ignore errors */
        }

        /* now, draw to our target surface (convert position) */
        draw_bitmap(p_raster, &slot->bitmap, slot->bitmap_left, p_raster->h - slot->bitmap_top, color, x + w, y + h);

        /* increment pen position */
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;

        i += len;
    }

#endif

    int x2 = (pen.x >> 6);
    int w2 = (x < x2) ? (x2 - x) : 0;

    // 这里简单处理: 若 w < w2, 则 认为没有完整的将所有 字符输出
    bool is_draw_all = true;
    if (w < w2)
    {
        // 没有将所有文本 都绘制
        is_draw_all = false;

        // 限制宽度 最大 为 w
        w2 = w;
    }

    if (NULL != p_out_w) { *p_out_w = w2; }
    if (NULL != p_out_h) { *p_out_h = font_h; }
    if (NULL != p_out_draw_all) { *p_out_draw_all = is_draw_all;}

    return 0;
}

static int compute_string_bbox(FT_Face face, int w, int h, const uint32_t* p_unicode, int unicode_len, FT_BBox *box)
{
	int error, max_len;
	FT_BBox bbox;						/* 当前整行外框 */
	FT_BBox glyph_bbox;					/* 当前字符外框 */
	FT_Vector pen;						/* 当前字符glyph image的origin在笛卡尔坐标系下的位置 */
	FT_Glyph glyph;
	FT_GlyphSlot slot = face->glyph;	/* 插槽slot */

	pen.x = 0;
	pen.y = 0;
	bbox.xMin = bbox.yMin = 32000;
	bbox.xMax = bbox.yMax = -32000;

	/* 循环计算每个字符的外框(bounding box), 然后更新整行的外框 */
	for (max_len = 0; max_len < unicode_len; )
	{
		/* 将当前字符的origin移动到pen */
		FT_Set_Transform(face, 0, &pen); // 矩阵传入0, 表示不旋转

		/* load glyph image into the slot (erase previous one) */
		error = FT_Load_Char(face, p_unicode[max_len], FT_LOAD_GUI_RENDER);
		if (error)
		{
			max_len++;
			continue;                 /* ignore errors */
		}

		/* get glyph image */
		error = FT_Get_Glyph(face->glyph, &glyph);
		if (error)
		{
			max_len++;
			continue;                 /* ignore errors */
		}

		/* 从当前字符对应的glyph image得到当前字符的外框glyph_bbox */
		FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_TRUNCATE, &glyph_bbox);

		/* free glyph image */
		FT_Done_Glyph(glyph);

		if (glyph_bbox.xMax - bbox.xMin > w)
		{
			break;
		}

		if ((0x000a == p_unicode[max_len])
			|| (0x000d == p_unicode[max_len]))	//换行符
		{
			max_len++;
			break;
		}

		/* 更新整行外框bbox */
		if (glyph_bbox.xMin < bbox.xMin)
			bbox.xMin = glyph_bbox.xMin;

		if (glyph_bbox.yMin < bbox.yMin)
			bbox.yMin = glyph_bbox.yMin;

		if (glyph_bbox.xMax > bbox.xMax)
			bbox.xMax = glyph_bbox.xMax;

		if (glyph_bbox.yMax > bbox.yMax)
			bbox.yMax = glyph_bbox.yMax;

		/* 计算同一行下个字符的origin位置: + advance */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;

		max_len++;
	}

	/* 保存结果: 整行外框 */
	if (box)
	{
		*box = bbox;
	}

	return max_len;//返回一行能显示的最大长度
}

static int display_string(ft_raster_pixels_t* p_raster, FT_Face face, FT_Vector* p_pen, int x, int y, int w, int h, const uint32_t* p_unicode, int unicode_len, uint32_t color)
{
	FT_GlyphSlot slot = face->glyph;

	for (int i = 0; i < unicode_len; )
	{
		/* set transformation */
		FT_Set_Transform(face, 0, p_pen);

		/* load glyph image into the slot (erase previous one) */
		int error = FT_Load_Char(face, p_unicode[i], FT_LOAD_GUI_RENDER);
		if (error)
		{
			i++;
			continue;                 /* ignore errors */
		}

		/* now, draw to our target surface (convert position) */
		draw_bitmap(p_raster, &slot->bitmap, slot->bitmap_left, p_raster->h - slot->bitmap_top, color, x + w, y + h);

		/* increment pen position */
		p_pen->x += slot->advance.x;
		p_pen->y += slot->advance.y;

		i++;
	}

	return 0;
}

static bool is_need_warp(uint32_t ch)
{
	bool b = false;
	if ((0x0020 == ch)
		|| (0x002c == ch)
		|| (0x002e == ch))
	{
		b = true;
	}

	return b;
}

static bool is_force_warp(uint32_t ch)
{
	bool b = false;
	if ((0x000a == ch)
		|| (0x000d == ch))
	{
		b = true;
	}

	return b;
}

static bool is_unicode_CJK(uint32_t ch)
{
	bool b = false;
	if ((ch >= 0x3400 && ch <= 0x4db5)
		|| ((ch >= 0x4e00 && ch <= 0x9fa5))
		|| ((ch >= 0x9fa6 && ch <= 0x9fbb))
		|| ((ch >= 0xf900 && ch <= 0xfa2d))
		|| ((ch >= 0xfa30 && ch <= 0xfa6a))
		|| ((ch >= 0xfa70 && ch <= 0xfad9))
		|| ((ch >= 0x20000 && ch <= 0x2a6d6))
		|| ((ch >= 0x2f800 && ch <= 0x2fa1d)))
	{
		b = true;
	}
	else if (ch >= 0xff00 && ch <= 0xffef)
	{
		b = true;
	}
	else if (ch >= 0x2e80 && ch <= 0x2eff)
	{
		b = true;
	}
	else if (ch >= 0x3000 && ch <= 0x303f)
	{
		b = true;
	}
	else if (ch >= 0x31c0 && ch <= 0x31ef)
	{
		b = true;
	}

	return b;
}

static int search_suitable_string(FT_Face face, int w, int h, const uint32_t* p_unicode, int unicode_len, FT_BBox *box)
{
	int len = compute_string_bbox(face, w, h, p_unicode, unicode_len, box);

	if ((len > 0) && (len < unicode_len))
	{
		if (is_force_warp(p_unicode[len - 1]))
		{
			return len;
		}

		if (!is_unicode_CJK(p_unicode[len])	//下一个unicode不为CJK
			&& !is_need_warp(p_unicode[len]))	//下一个unicode不为空格/,/. 
		{
            int suitable_len = len;
			while (suitable_len > 0)
			{
				if (is_unicode_CJK(p_unicode[suitable_len - 1])
					|| is_need_warp(p_unicode[suitable_len - 1]))
				{
					break;
				}

                suitable_len--;
			}

            // 若找到 需要换行点, 则换行;
            // 若未找到 需要换行点, 则完整输出整个文字
            if (0 < suitable_len)
            {
                len = suitable_len;
            }
		}
	}

	return len;
}

int ft_raster_text_ex(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int line_spacing)
{
    if (!ft_raster_font_ready(p_ft))
    {
        return 1;
    }

	int error;
	FT_BBox bbox;
	FT_Vector pen;
	FT_GlyphSlot slot = p_ft->face->glyph;

	//utf-8 -----> unicode
	uint32_t* p_unicode = KLB_MALLOCZ(uint32_t, utf8_len, 0);
	int unicode_len = 0;
	assert(NULL != p_unicode);

	for (int i = 0; i < utf8_len; )
	{
		uint32_t ch = 0;
		int len = 0;
		FT_utf8_to_unicode((p_utf8 + i), ch, len);
		if (utf8_len < i + len)
		{
			break;
		}

		p_unicode[unicode_len] = ch;
		unicode_len++;
		i += len;
	}

	/* use font_h pt at 72dpi */
	error = FT_Set_Char_Size(p_ft->face, FT_int32(font_h), FT_int32(font_h), FT_dpi, FT_dpi);     /* set character size */

	int len = 0;
	bool first = true;
	while (len < unicode_len)
	{
		int cur_len = 0;
		cur_len = search_suitable_string(p_ft->face, w, h, p_unicode + len, unicode_len - len, &bbox);
		if (0 == cur_len)
		{
			break;
		}

		/* calculate origin */
		if (first)
		{
			pen.x = FT_int32(x - bbox.xMin);
			pen.y = FT_int32(p_raster->h - y - bbox.yMax);
			first = false;
		}
		else
		{
			pen.x = FT_int32(x - bbox.xMin);
			pen.y = FT_int32((pen.y >> 6) - font_h - line_spacing);
		}
		display_string(p_raster, p_ft->face, &pen, x, y, w, h, p_unicode + len, cur_len, color);

		len += cur_len;
	}

	KLB_FREE(p_unicode);
	return 0;
}

int ft_raster_text_size(ft_raster_t* p_ft, const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h)
{
    if (!ft_raster_font_ready(p_ft))
    {
        if (NULL != p_out_w)
        {
            *p_out_w = 0;
        }

        if (NULL != p_out_h)
        {
            *p_out_h = 0;
        }

        return 1;
    }

    FT_GlyphSlot slot = p_ft->face->glyph;

    /* use font_h pt at 72dpi */
    int error = FT_Set_Char_Size(p_ft->face, FT_int32(font_h), FT_int32(font_h), FT_dpi, FT_dpi);     /* set character size */

    FT_Matrix   matrix;         /* transformation matrix */

    double angle = 0.0;
    matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

    // freetype 采用的英文基准线
    int offset_y = 4;
    if (28 < font_h)
    {
        offset_y = 6;
    }

    FT_Vector   pen;            /* untransformed origin  */

    int x = 0, y = font_h;

    /* the pen position in 26.6 cartesian space coordinates; */
    /* start at (x,y+h) relative to the upper left corner  */
    pen.x = FT_int32(x);
    //pen.y = FT_int32(p_raster->h - (y + h) + offset_y);
    pen.y = FT_int32(y);

#if FT_USE_CACHE
    // freetype2接口 高阶用法
    // 翻译: https://zhuanlan.zhihu.com/p/84779973
    // 原文: https://freetype.org/freetype2/docs/tutorial/step2.html

    FT_Vector zero = { 0 };
    FT_Set_Transform(p_ft->face, &matrix, &zero);

    for (int i = 0; i < utf8_len; )
    {
        // step1. 从 utf8 中读取 单个 unicode 字符码
        uint32_t ch = 0;
        int len = 0;
        FT_utf8_to_unicode((p_utf8 + i), ch, len);

        if (utf8_len < i + len)
        {
            break;
        }

        // step2. 在 缓存 中 获取 字符码 对应的 字符图形
        // 获取 字符码 对应的 字符图形
        FT_Glyph glyph = get_glyph_ft_raster(p_ft, ch, font_h);
        if (NULL == glyph)
        {
            // 在字库中未找到字符, 则使用 ? 替代字符
            glyph = get_glyph_ft_raster(p_ft, '?', font_h);
        }

        if (NULL != glyph)
        {
            // step3. 不变换缓存字形; 只累加 advance
            pen.x += (glyph->advance.x >> 10);
            pen.y += (glyph->advance.y >> 10);
        }

        // 偏移位置
        i += len;
    }

#else
    // 简易 文本绘制

    for (int i = 0; i < utf8_len; )
    {
        /* set transformation */
        FT_Set_Transform(p_ft->face, &matrix, &pen);

        uint32_t ch = 0;
        int len = 0;
        FT_utf8_to_unicode((p_utf8 + i), ch, len);

        if (utf8_len < i + len)
        {
            break;
        }

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(p_ft->face, ch, FT_LOAD_GUI_RENDER);
        if (error)
        {
            i += len;
            continue;                 /* ignore errors */
        }

        /* now, draw to our target surface (convert position) */
        //draw_bitmap(p_raster, &slot->bitmap, slot->bitmap_left, p_raster->h - slot->bitmap_top, color);

        /* increment pen position */
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;

        i += len;
    }
#endif

    if (NULL != p_out_w)
    {
        *p_out_w = (pen.x >> 6);
    }

    if (NULL != p_out_h)
    {
        *p_out_h = font_h;
    }

    return 0;
}
