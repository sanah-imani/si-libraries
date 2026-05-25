#ifndef SI_TUI_H
#define SI_TUI_H 

#ifndef SIT_FUNC_DEF 
#   if defined(SIT_STATIC)
#       define SIT_FUNC_DEF static
#   elif defined(_WIN32) && defined(SIT_DLL) && defined(SI_TUI_IMPL)
#       define SIT_FUNC_DEF __declspec(dllexport)
#   elif defined(_WIN32) && defined(SIT_DLL)
#       define SIT_FUNC_DEF __declspec(dllimport)
#   else
#       define SIT_FUNC_DEF extern
#   endif
#endif

#ifndef SI_ARENA_H
#   error "si_arena.h must be included before si_tui.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define SIT_TRUE  1
#define SIT_FALSE 0

typedef sia_i32 sit_b32;

/* =========================================================================
   Colors
   ========================================================================= */

typedef struct {
    sia_u8 r, g, b;
} sit_color;

#define SIT_RGB(r, g, b) ((sit_color){ (r), (g), (b) })

#define SIT_BLACK       ((sit_color){   0,   0,   0 })
#define SIT_WHITE       ((sit_color){ 255, 255, 255 })
#define SIT_RED         ((sit_color){ 255,   0,   0 })
#define SIT_GREEN       ((sit_color){   0, 255,   0 })
#define SIT_BLUE        ((sit_color){   0,   0, 255 })
#define SIT_YELLOW      ((sit_color){ 255, 255,   0 })
#define SIT_CYAN        ((sit_color){   0, 255, 255 })
#define SIT_MAGENTA     ((sit_color){ 255,   0, 255 })
#define SIT_GRAY        ((sit_color){ 128, 128, 128 })

#define SIT_ATTR_NONE      0x00
#define SIT_ATTR_BOLD      0x01
#define SIT_ATTR_DIM       0x02
#define SIT_ATTR_UNDERLINE 0x04
#define SIT_ATTR_REVERSE   0x08

/* =========================================================================
   Cell & Canvas
   ========================================================================= */

typedef struct {
    sia_u32 ch;
    sit_color fg;
    sit_color bg;
    sia_u8 attrs;
} sit_cell;

typedef struct {
    si_arena* arena;
    sia_u32 width, height;
    sit_cell* cells;
    sit_cell* prev;
    sit_color clear_fg;
    sit_color clear_bg;
} sit_canvas;

/* =========================================================================
   Box drawing styles
   ========================================================================= */

typedef enum {
    SIT_BOX_SINGLE,
    SIT_BOX_DOUBLE,
    SIT_BOX_ROUND,
    SIT_BOX_HEAVY
} sit_box_style;

/* =========================================================================
   Table data
   ========================================================================= */

typedef struct {
    const char** headers;
    const char** rows;
    sia_u32 num_cols;
    sia_u32 num_rows;
} sit_table_data;

SIT_FUNC_DEF sit_canvas* sit_canvas_create(si_arena* arena, sia_u32 width, sia_u32 height);
SIT_FUNC_DEF void sit_clear(sit_canvas* canvas);
SIT_FUNC_DEF void sit_flush(sit_canvas* canvas);
SIT_FUNC_DEF void sit_flush_full(sit_canvas* canvas);

// Drawing Primitives 

SIT_FUNC_DEF void sit_put(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 ch, sit_color fg, sit_color bg, sia_u8 attrs);
SIT_FUNC_DEF void sit_hline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 len, sit_color fg, sit_color bg, sia_u8 attrs);
SIT_FUNC_DEF void sit_vline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 len, sit_color fg, sit_color bg, sia_u8 attrs);
SIT_FUNC_DEF void sit_fill_rect(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, sit_color fg, sit_color bg, sia_u8 attrs);
SIT_FUNC_DEF void sit_box(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h,
    sit_box_style style, sit_color fg, sit_color bg);
SIT_FUNC_DEF void sit_text(sit_canvas* c, sia_u32 x, sia_u32 y, const char* str,
     sit_color fg, sit_color bg, sia_u8 attrs);
SIT_FUNC_DEF void sit_textf(sit_canvas* c, sia_u32 x, sia_u32 y, sit_color fg, sit_color bg,
      sia_u8 attrs, const char* fmt, ...);
SIT_FUNC_DEF void sit_line(sit_canvas* c, sia_u32 x0, sia_u32 y0, sia_u32 x1, sia_u32 y1,
     sia_u32 ch, sit_color fg, sit_color bg);

// widgets 

SIT_FUNC_DEF void sit_progress_bar(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, float progress);
SIT_FUNC_DEF void sit_bar_chart(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, const float* values, sia_u32 num_values);
SIT_FUNC_DEF void sit_sparkline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, const float* values, sia_u32 num_values);
SIT_FUNC_DEF void sit_table(sit_canvas* c, sia_u32 x, sia_u32 y,
    const sit_table_data* data, sit_box_style border,
    sit_color header_fg, sit_color cell_fg, sit_color border_fg, sit_color bg);
SIT_FUNC_DEF void sit_separator(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 width,
        const char* label, sit_color fg, sit_color bg);
SIT_FUNC_DEF void sit_gradient_rect(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h,
            sia_u32 ch, sit_color left, sit_color right);


SIT_FUNC_DEF void sit_enter_alt_screen(void);
SIT_FUNC_DEF void sit_leave_alt_screen(void);
SIT_FUNC_DEF void sit_get_term_size(sia_u32* out_w, sia_u32* out_h);
SIT_FUNC_DEF void sit_hide_cursor(void);
SIT_FUNC_DEF void sit_show_cursor(void);

#ifdef __cplusplus
}

#endif 

#endif // SI_TUI_H


#ifdef SI_TUI_IMPL

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

sit_canvas* sit_canvas_create(si_arena* arena, sia_u32 width, sia_u32 height) {
    sit_canvas* canvas = SIA_PUSH_ZERO_STRUCT(arena, sit_canvas);
    canvas->arena = arena;
    canvas->width = width;
    canvas->height = height;
    canvas->cells = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, width * height);
    canvas->prev = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, width * height);
    canvas->clear_fg = SIT_WHITE;
    canvas->clear_bg = SIT_BLACK;
    return canvas;
}

void sit_clear(sit_canvas* canvas){
    sia_u32 total = canvas->width * canvas->height;
    for (sia_u32 i = 0; i < total; i++){
        canvas->cells[i].ch = ' ';
        canvas->cells[i].fg = canvas->clear_fg;
        canvas->cells[i].bg = canvas->clear_bg;
        canvas->cells[i].attrs = SIT_ATTR_NONE;
    }
}

void sit_put(sit_canvas* canvas, sia_u32 x, sia_u32 y,  sia_u32 ch,
    sit_color fg, sit_color bg, sia_u8 attrs){

    if (x >= canvas->width || y >= canvas->height){
        return;
    }

    sia_u32 index = y * canvas->width + x;
    canvas->cells[index].ch = ch;
    canvas->cells[index].fg = fg;
    canvas->cells[index].bg = bg;
    canvas->cells[index].attrs = attrs;
}

void sit_text(sit_canvas* c, sia_u32 x, sia_u32 y, const char* str,
    sit_color fg, sit_color bg, sia_u8 attrs) {
    sia_u32 cx = x;
    while (*str) {
    if (cx >= c->width) break;
    sit_put(c, cx, y, (sia_u32)(sia_u8)*str, fg, bg, attrs);
    cx++;
    str++;
    }
}

typedef struct {
    sia_u32 tl, tr, bl, br, h, v;
} _sit_box_chars;

static const _sit_box_chars _sit_box_table[4] = {
    [SIT_BOX_SINGLE] = { 0x250C, 0x2510, 0x2514, 0x2518, 0x2500, 0x2502 },
    [SIT_BOX_DOUBLE] = { 0x2554, 0x2557, 0x255A, 0x255D, 0x2550, 0x2551 },
    [SIT_BOX_ROUND]  = { 0x256D, 0x256E, 0x2570, 0x256F, 0x2500, 0x2502 },
    [SIT_BOX_HEAVY]  = { 0x250F, 0x2513, 0x2517, 0x251B, 0x2501, 0x2503 },
};

void sit_box(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h,
    sit_box_style style, sit_color fg, sit_color bg) {
    
    if (w < 2 || h < 2) return;
    const _sit_box_chars* bc = &_sit_box_table[style];

    sit_put(c, x, y, bc->tl, fg, bg, SIT_ATTR_NONE);
    sit_put(c, x + w - 1, y, bc->tr, fg, bg, SIT_ATTR_NONE);
    sit_put(c, x, y + h - 1, bc->bl, fg, bg, SIT_ATTR_NONE);
    sit_put(c, x + w - 1, y + h - 1, bc->br, fg, bg, SIT_ATTR_NONE);

    for (sia_u32 i = 1; i < w - 1; i++){
        sit_put(c, x + i, y, bc->h, fg, bg, SIT_ATTR_NONE);
        sit_put(c, x + i, y + h - 1, bc->h, fg, bg, SIT_ATTR_NONE);
    }

    for (sia_u32 i = 1; i < h - 1; i++){
        sit_put(c, x, y + i, bc->v, fg, bg, SIT_ATTR_NONE);
        sit_put(c, x + w - 1, y + i, bc->v, fg, bg, SIT_ATTR_NONE);
    }
}

static void _sit_write_utf8(char* buf, sia_u32* pos, sia_u32 cp) {
    if (cp < 0x80) {
        buf[(*pos)++] = (char)cp;
    } else if (cp < 0x800) {
        buf[(*pos)++] = (char)(0xC0 | (cp >> 6));
        buf[(*pos)++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        buf[(*pos)++] = (char)(0xE0 | (cp >> 12));
        buf[(*pos)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[(*pos)++] = (char)(0x80 | (cp & 0x3F));
    } else {
        buf[(*pos)++] = (char)(0xF0 | (cp >> 18));
        buf[(*pos)++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[(*pos)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[(*pos)++] = (char)(0x80 | (cp & 0x3F));
    }
}

void sit_flush(sit_canvas* canvas){
    sia_temp scratch = sia_scratch_get(&canvas->arena, 1);

    sia_u64 buf_size = (sia_u64) canvas->width * canvas->height * 40 + 256;
    char *buf = (char*) sia_push(scratch.arena, buf_size);
    sia_u32 pos = 0;

    for (sia_u32 y = 0; y < canvas->height; y++){
        pos += snprintf(buf + pos, buf_size - pos, "\033[%u;1H", y + 1);

        for (sia_u32 x = 0; x < canvas->width; x++) {
            sia_u32 idx = y * canvas->width + x;
            sit_cell* cur  = &canvas->cells[idx];
            sit_cell* prev = &canvas->prev[idx];
            if (cur->ch == prev->ch &&
                cur->fg.r == prev->fg.r && cur->fg.g == prev->fg.g && cur->fg.b == prev->fg.b &&
                cur->bg.r == prev->bg.r && cur->bg.g == prev->bg.g && cur->bg.b == prev->bg.b &&
                cur->attrs == prev->attrs) {
                pos += snprintf(buf + pos, buf_size - pos, "\033[C");
                continue;
            }

            pos += snprintf(buf + pos, buf_size - pos, "\033[0m" );
            if (cur->attrs & SIT_ATTR_BOLD) pos += snprintf(buf + pos, buf_size - pos, "\033[1m");
            if (cur->attrs & SIT_ATTR_DIM)       pos += snprintf(buf + pos, buf_size - pos, "\033[2m");
            if (cur->attrs & SIT_ATTR_UNDERLINE) pos += snprintf(buf + pos, buf_size - pos, "\033[4m");
            if (cur->attrs & SIT_ATTR_REVERSE)   pos += snprintf(buf + pos, buf_size - pos, "\033[7m");
            
            pos += snprintf(buf + pos, buf_size - pos,
                "\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um",
                cur->fg.r, cur->fg.g, cur->fg.b,
                cur->bg.r, cur->bg.g, cur->bg.b);
            _sit_write_utf8(buf, &pos, cur->ch);

        }

    }

    pos += snprintf(buf + pos, buf_size - pos, "\033[0m");
    write(STDOUT_FILENO, buf, pos);
    

    sit_cell* tmp = canvas->prev;
    canvas->prev = canvas->cells;
    canvas->cells = tmp;
    sia_scratch_release(scratch);
}

void sit_hline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 len, sit_color fg, sit_color bg, sia_u8 attrs){
    for (sia_u32 i = 0; i < len; i++){
        sit_put(canvas, x + i, y, ' ', fg, bg, attrs);
    }
}

void sit_vline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 len, sit_color fg, sit_color bg, sia_u8 attrs){
     for (sia_u32 i = 0; i < len; i++){
        sit_put(canvas, x, y + i, ' ', fg, bg, attrs);
     }    
}

void sit_fill_rect(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, sit_color fg, sit_color bg, sia_u8 attrs){
    for (sia_u32 i = 0; i < h; i++){
        sit_hline(canvas, x, y + i, w, fg, bg, attrs);
    }
}

void sit_line(sit_canvas* c, sia_u32 x0, sia_u32 y0, sia_u32 x1, sia_u32 y1,
    sia_u32 ch, sit_color fg, sit_color bg) {
    
    int dx = (int)x1 - (int)x0;
    int dy = (int)y1 - (int)y0;

    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;

    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;

    int err = dx - dy;

    int cx = (int)x0;
    int cy = (int)y0;

    for (;;){
        if (cx >= 0 && cy >= 0) {
            sit_put(c, (sia_u32)cx, (sia_u32)cy, ch, fg, bg, SIT_ATTR_NONE);
        } 

        if (cx == (int)x1 && cy == (int)y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            cx += sx;
        }
        if (e2 < dx) {
            err += dx;
            cy += sy;
        }
    }
}

void sit_textf(sit_canvas* c, sia_u32 x, sia_u32 y, sit_color fg, sit_color bg, sia_u8 attrs, const char* fmt, ...){
    sia_temp scratch = sia_scratch_get(&c->arena, 1);

    va_list args;
    va_start(args, fmt);

    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len <= 0){
        sia_scratch_release(scratch);
        return;
    }

    char* buf = (char*)sia_push(scratch.arena, len+1);

    va_start(args, fmt);
    vsnprintf(buf, len+1, fmt, args);
    va_end(args);

    sit_text(c, x, y, buf, fg, bg, attrs);
    sia_scratch_release(scratch);
}

void sit_flush_full(sit_canvas* canvas){
    sia_temp scratch = sia_scratch_get(&canvas->arena, 1);
    sia_u64 buf_size = (sia_u64) canvas->width * canvas->height * 40 + 256;

    char *buf = (char*) sia_push(scratch.arena, buf_size);
    sia_u32 pos = 0;

    for (sia_u32 y = 0; y < canvas->height; y++){
        pos += snprintf(buf + pos, buf_size - pos, "\033[%u;1H", y + 1);
        for (sia_u32 x = 0; x < canvas->width; x++){
            sia_u32 idx = y * canvas->width + x;
            sit_cell* cur = &canvas->cells[idx];

            pos += snprintf(buf + pos, buf_size - pos, "\033[0m");

            if (cur->attrs & SIT_ATTR_BOLD) pos += snprintf(buf + pos, buf_size - pos, "\033[1m");
            if (cur->attrs & SIT_ATTR_DIM)       pos += snprintf(buf + pos, buf_size - pos, "\033[2m");
            if (cur->attrs & SIT_ATTR_UNDERLINE) pos += snprintf(buf + pos, buf_size - pos, "\033[4m");
            if (cur->attrs & SIT_ATTR_REVERSE)   pos += snprintf(buf + pos, buf_size - pos, "\033[7m");

            pos += snprintf(buf + pos, buf_size - pos,
                "\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um",
                cur->fg.r, cur->fg.g, cur->fg.b,
                cur->bg.r, cur->bg.g, cur->bg.b);
            _sit_write_utf8(buf, &pos, cur->ch);
        }

    }

    pos += snprintf(buf + pos, buf_size - pos, "\033[0m");
    write(STDOUT_FILENO, buf, pos);
    sit_cell* tmp = canvas->prev;
    canvas->prev = canvas->cells;
    canvas->cells = tmp;
    sia_scratch_release(scratch);
}
#endif /* SI_TUI_IMPL */

/*
License
=================================
  _    ___ ___ ___ _  _ ___ ___ 
 | |  |_ _/ __| __| \| / __| __|
 | |__ | | (__| _|| .` \__ \ _| 
 |____|___\___|___|_|\_|___/___|
                                
=================================

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/