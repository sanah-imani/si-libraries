#ifndef SI_TUI_H
#define SI_TUI_H 

#include <sys/_pthread/_pthread_t.h>
#include <sys/_types/_pid_t.h>
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

/* =========================================================================
   Input
   ========================================================================= */
typedef enum {
    SIT_KEY_NONE,
    SIT_KEY_CHAR,
    SIT_KEY_ESC,
    SIT_KEY_ENTER,
    SIT_KEY_BACKSPACE,
    SIT_KEY_TAB,
    SIT_KEY_UP,
    SIT_KEY_DOWN,
    SIT_KEY_LEFT,
    SIT_KEY_RIGHT,
    SIT_KEY_HOME,
    SIT_KEY_END,
    SIT_KEY_PAGE_UP,
    SIT_KEY_PAGE_DOWN,
    SIT_KEY_DELETE,
} sit_key_kind;


#define SIT_MOD_CTRL  0x01
#define SIT_MOD_ALT   0x02
#define SIT_MOD_SHIFT 0x04

typedef struct {
    sit_key_kind kind;
    char ch;
    sia_u8 mods;
} sit_key;



typedef struct {
    sit_b32 quit;
    sit_b32 resized;
    sit_key key;
} sit_event;


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

// event handling 

SIT_FUNC_DEF void sit_enter_raw_mode(void);
SIT_FUNC_DEF void sit_leave_raw_mode(void);
SIT_FUNC_DEF sit_b32 sit_poll_event(sit_event* out);

/* =========================================================================
   Clip rect (stack-based, up to 8 levels)
   ========================================================================= */
SIT_FUNC_DEF void sit_clip_push(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h);
SIT_FUNC_DEF void sit_clip_pop(sit_canvas* c);


/* =========================================================================
   Color / canvas utilities
========================================================================= */
SIT_FUNC_DEF sit_color sit_color_lerp(sit_color from, sit_color to, float t);
SIT_FUNC_DEF void sit_canvas_resize(si_arena* arena, sit_canvas* c, sia_u32 w, sia_u32 h);

/* =========================================================================
Text helpers
========================================================================= */
SIT_FUNC_DEF sia_u32 sit_text_width(const char* str);
SIT_FUNC_DEF void sit_text_clip(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 max_w,
    const char* str, sit_color fg, sit_color bg, sia_u8 attrs);
#ifdef __cplusplus
}

#endif 

#endif // SI_TUI_H


#ifdef SI_TUI_IMPL

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

static struct termios _sit_orig_termios;
static sit_b32 _sit_raw_mode = SIT_FALSE;
static volatile sit_b32 _sit_term_resized = SIT_FALSE;

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

void sit_progress_bar(sit_canvas* canvas, sia_u32 x, sia_u32 y,
    sia_u32 w, sia_u32 h, float progress) {
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    sia_u32 filled = (sia_u32)(progress * w);
    for (sia_u32 row = 0; row < h; row++) {
        for (sia_u32 col = 0; col < w; col++) {
            if (col < filled) {
            sit_put(canvas, x + col, y + row, 0x2588, SIT_GREEN, SIT_BLACK, SIT_ATTR_NONE);
            } else {
            sit_put(canvas, x + col, y + row, 0x2591, SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
            }
        }
    }
}

void sit_bar_chart(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, const float* values, sia_u32 num_values){

    if (num_values == 0 || h == 0 || w == 0) return;

    float max_val = values[0];

    for (sia_u32 i = 1; i < num_values; i++){
        if (values[i] > max_val) max_val = values[i];   
    }
    if (max_val <= 0.0f) return;
    

    sia_u32 bar_width = w / num_values;

    if (bar_width == 0) bar_width = 1;

    for (sia_u32 i = 0; i < num_values; i++){
        sia_u32 bar_x = x + i * bar_width;
        sia_u32 bar_h = (sia_u32)(values[i] / max_val * h);

        for (sia_u32 col = 0; col < bar_width; col++){
            for (sia_u32 row = 0; row < h; row++){
                sia_u32 py = y + (h - 1 - row);
                if (row < bar_h) {
                    sit_put(canvas, bar_x + col, py, 0x2588, SIT_GREEN, SIT_BLACK, SIT_ATTR_NONE);
                } else {
                    sit_put(canvas, bar_x + col, py, 0x2591, SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
                }
            }
        }
    }
}

void sit_table(sit_canvas* c, sia_u32 x, sia_u32 y, const sit_table_data* data, sit_box_style border, sit_color header_fg, sit_color cell_fg, sit_color border_fg, sit_color bg){
    if (data == NULL || data->num_cols == 0 || data->num_rows == 0) return;

    sia_temp scratch = sia_scratch_get(&c->arena, 1);

    sia_u32* col_w = SIA_PUSH_ARRAY(scratch.arena, sia_u32, data->num_cols);

    for (sia_u32 col = 0; col < data->num_cols; col++){
        if (data->headers != NULL) {
            sia_u32 len = 0;
            const char* s = data->headers[col];
            while (s && *s) { len++; s++; }
            if (len > col_w[col]) col_w[col] = len;
        }

        for (sia_u32 row = 0; row < data->num_rows; row++) {
            sia_u32 len = 0;
            const char* s = data->rows[row * data->num_cols + col];
            while (s && *s) { len++; s++; }
            if (len > col_w[col]) col_w[col] = len;
        }
        col_w[col] += 2; /* 1 char padding each side */

    }

    const _sit_box_chars* bc = &_sit_box_table[border];

    sia_u32 total_w = 0;

    for (sia_u32 col = 0; col < data->num_cols; col++){
        total_w += col_w[col]+1;
    }

    sia_u32 cy = y;

    sit_put(c, x, cy, bc->tl, border_fg, bg, SIT_ATTR_NONE);
    sia_u32 cx = x + 1;
    for (sia_u32 col = 0; col < data->num_cols; col++){
        for (sia_u32 i = 0; i < col_w[col]; i++){
            sit_put(c, cx++, cy, bc->h, border_fg, bg, SIT_ATTR_NONE);
        }
        if (col < data->num_cols - 1)
            sit_put(c, cx++, cy, 0x252C, border_fg, bg, SIT_ATTR_NONE); /* ┬ */
    }

    sit_put(c, cx, cy, bc->tr, border_fg, bg, SIT_ATTR_NONE);
    cy++;

    if (data->headers != NULL){
        sit_put(c, x, cy, bc->v, border_fg, bg, SIT_ATTR_NONE);
        cx = x + 1;

        for (sia_u32 col = 0; col < data->num_cols; col++){
            sit_put(c, cx, cy, ' ', header_fg, bg, SIT_ATTR_NONE);
            sit_text(c, cx + 1, cy, data->headers[col], header_fg, bg, SIT_ATTR_BOLD);
            cx += col_w[col];
            sit_put(c, cx++, cy, bc->v, border_fg, bg, SIT_ATTR_NONE);
        }
        cy++;

        sit_put(c, cx, cy, bc->v, border_fg, bg, SIT_ATTR_NONE);
        cx = x + 1;
        for (sia_u32 col = 0; col < data->num_cols; col++){
            for (sia_u32 i = 0; i < col_w[col]; i++){
                sit_put(c, cx++, cy, bc->h, border_fg, bg, SIT_ATTR_NONE);
            }
            if (col < data->num_cols - 1){
                sit_put(c, cx++, cy, 0x253C, border_fg, bg, SIT_ATTR_NONE); /* ┼ */
            }
        }
        sit_put(c, cx, cy, 0x2524, border_fg, bg, SIT_ATTR_NONE); /* ╤ */
        cy++;
    }


    /* Data rows*/

    for (sia_u32 row = 0; row < data->num_rows; row++){
        sit_put(c, x, cy, bc->v, border_fg, bg, SIT_ATTR_NONE);
        cx = x + 1;
        for (sia_u32 col = 0; col < data->num_cols; col++){
            sit_put(c, cx, cy, ' ', cell_fg, bg, SIT_ATTR_NONE);
            const char* cell = data->rows[row * data->num_cols + col];
            if (cell) sit_text(c, cx + 1, cy, cell, cell_fg, bg, SIT_ATTR_NONE);
            cx += col_w[col];
            sit_put(c, cx++, cy, bc->v, border_fg, bg, SIT_ATTR_NONE);
        }
        cy++;
    }

    /* Bottom border:  └────┴────┘ */
    sit_put(c, x, cy, bc->bl, border_fg, bg, SIT_ATTR_NONE);
    cx = x + 1;
    for (sia_u32 col = 0; col < data->num_cols; col++) {
        for (sia_u32 i = 0; i < col_w[col]; i++)
            sit_put(c, cx++, cy, bc->h, border_fg, bg, SIT_ATTR_NONE);
        if (col < data->num_cols - 1)
            sit_put(c, cx++, cy, 0x2534, border_fg, bg, SIT_ATTR_NONE); /* ┴ */
    }
    sit_put(c, cx, cy, bc->br, border_fg, bg, SIT_ATTR_NONE);
    sia_scratch_release(scratch);
}

static sit_color _sit_lerp_color(sit_color from, sit_color to, float t) {
    return (sit_color){
        .r = (sia_u8)(from.r + (to.r - from.r) * t),
        .g = (sia_u8)(from.g + (to.g - from.g) * t),
        .b = (sia_u8)(from.b + (to.b - from.b) * t),
    };
}

static void _sit_tty_cmd(const char* cmd) {
    size_t len = 0;
    while (cmd[len]) len++;
    write(STDOUT_FILENO, cmd, len);
}

void sit_separator(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 width,
        const char* label, sit_color fg, sit_color bg) {
    if (width == 0) return;

    sia_u32 label_len = 0;
    if (label) {
        while (label[label_len]) label_len++;
    }

    if (label_len == 0 || label_len + 2 >= width) {
        for (sia_u32 i = 0; i < width; i++)
            sit_put(c, x + i, y, 0x2500, fg, bg, SIT_ATTR_NONE);
        return;
    }

    sia_u32 dash_total = width - label_len - 2;
    sia_u32 dash_left = dash_total / 2;
    sia_u32 dash_right = dash_total - dash_left;
    sia_u32 cx = x;

    for (sia_u32 i = 0; i < dash_left; i++)
        sit_put(c, cx++, y, 0x2500, fg, bg, SIT_ATTR_NONE);
    sit_put(c, cx++, y, ' ', fg, bg, SIT_ATTR_NONE);
    sit_text(c, cx, y, label, fg, bg, SIT_ATTR_NONE);
    cx += label_len;
    sit_put(c, cx++, y, ' ', fg, bg, SIT_ATTR_NONE);
    for (sia_u32 i = 0; i < dash_right; i++)
        sit_put(c, cx++, y, 0x2500, fg, bg, SIT_ATTR_NONE);
}

void sit_gradient_rect(sit_canvas* c, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h,
        sia_u32 ch, sit_color left, sit_color right) {
    if (w == 0 || h == 0) return;

    for (sia_u32 row = 0; row < h; row++) {
        for (sia_u32 col = 0; col < w; col++) {
            float t = (w <= 1) ? 0.0f : (float)col / (float)(w - 1);
            sit_color fg = _sit_lerp_color(left, right, t);
            sit_put(c, x + col, y + row, ch, fg, SIT_BLACK, SIT_ATTR_NONE);
        }
    }
}

void sit_enter_alt_screen(void) { _sit_tty_cmd("\033[?1049h"); }
void sit_leave_alt_screen(void) { _sit_tty_cmd("\033[?1049l"); }
void sit_hide_cursor(void)       { _sit_tty_cmd("\033[?25l"); }
void sit_show_cursor(void)       { _sit_tty_cmd("\033[?25h"); }

void sit_get_term_size(sia_u32* out_w, sia_u32* out_h) {
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
        if (out_w) *out_w = size.ws_col;
        if (out_h) *out_h = size.ws_row;
    } else {
        if (out_w) *out_w = 80;
        if (out_h) *out_h = 24;
    }
}

static float _sit_spark_sample(const float* values, sia_u32 num_values, sia_u32 col, sia_u32 w){
    if (num_values == 0) return 0.0f;
    if (num_values == 1 || w <= 1) return values[0];

    float u = (float) col / (float) (w-1);
    float idx = u * (float)(num_values-1);
    sia_u32 idx_lo = (sia_u32)idx;
    sia_u32 idx_hi = idx_lo + 1;

    if (idx_hi >= num_values) idx_hi = num_values - 1;
    float blend = idx - (float)idx_lo;                       /* 0 = at lo, 1 = at hi */
    return values[idx_lo] * (1.0f - blend) + values[idx_hi] * blend;
}

void sit_sparkline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, const float* values, sia_u32 num_values){
    if (!canvas || !values || num_values == 0 || h == 0 || w == 0) return;
    float min_val = values[0];
    float max_val = values[0];

    for (sia_u32 i = 1; i < num_values; i++){
        if (values[i] < min_val) min_val = values[i];
        if (values[i] > max_val) max_val = values[i];
    }

    float range = max_val - min_val; 

    if (range <= 0.0f){
        sia_u32 base_y = y + h - 1;
        for (sia_u32 col = 0; col < w; col++){
            sit_put(canvas, x + col, base_y, 0x2588, SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
        }
        return;
    }

    if (h == 1){
        for (sia_u32 col = 0; col < w; col++){
            float v = _sit_spark_sample(values, num_values, col, w);
            float norm = (v - min_val) / range;
            sia_u32 level = (sia_u32)(norm * 7.0f + 0.5f);
            if (level > 7) level = 7;
            sit_put(canvas, x + col, y, 0x2581 + level, SIT_GREEN, SIT_BLACK, SIT_ATTR_NONE);
        }
        return;
    }

    sia_u32 prev_px = 0, prev_py = 0;
    sit_b32 have_prev = SIT_FALSE;

    for (sia_u32 col = 0; col < w; col++){
        float v = _sit_spark_sample(values, num_values, col, w);
        float norm = (v - min_val) / range;

        sia_u32 px = x + col; 
        sia_u32 py = y + (h-1) - (sia_u32)(norm * (h-1));

        if (have_prev){
            sit_line(canvas, prev_px, prev_py, px, py, 0x2588, SIT_GREEN, SIT_BLACK);
        }
        prev_px = px;
        prev_py = py;
        have_prev = SIT_TRUE;
    }
}

static void _sit_on_winch(int sig){
    (void)sig;
    _sit_term_resized = SIT_TRUE;
}

void sit_enter_raw_mode(void){
    if (_sit_raw_mode) return;

    tcgetattr(STDIN_FILENO, &_sit_orig_termios);

    struct termios raw = _sit_orig_termios;
    raw.c_iflag &= (tcflag_t) ~(ICANON| ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    struct sigaction sa;
    sa.sa_handler = _sit_on_winch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
    _sit_raw_mode = SIT_TRUE;
}

void sit_leave_raw_mode(void){
    if (!_sit_raw_mode) return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_sit_orig_termios);

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
    _sit_term_resized = SIT_FALSE;
    _sit_raw_mode = SIT_FALSE;
}

static sit_key _sit_key_from_bytes_event(sia_u8* bytes, sia_u32 num_bytes){
    sit_key key = { SIT_KEY_NONE, 0, 0 };
    if (!bytes || num_bytes == 0) return key;

    sia_u8 b = bytes[0];

    if (b == 0x1b && num_bytes > 1) {
        if (bytes[1] == '[' && num_bytes >= 3) {
            if (bytes[2] >= 'A' && bytes[2] <= 'D') {
                switch (bytes[2]) {
                case 'A': key.kind = SIT_KEY_UP; break;
                case 'B': key.kind = SIT_KEY_DOWN; break;
                case 'C': key.kind = SIT_KEY_RIGHT; break;
                case 'D': key.kind = SIT_KEY_LEFT; break;
                }
                return key;
            }
            if (num_bytes >= 4 && bytes[3] == '~') {
                switch (bytes[2]) {
                case '1': key.kind = SIT_KEY_HOME; break;
                case '3': key.kind = SIT_KEY_DELETE; break;
                case '4': key.kind = SIT_KEY_END; break;
                case '5': key.kind = SIT_KEY_PAGE_UP; break;
                case '6': key.kind = SIT_KEY_PAGE_DOWN; break;
                default: break;
                }
                return key;
            }
            if (bytes[2] == 'H') { key.kind = SIT_KEY_HOME; return key; }
            if (bytes[2] == 'F') { key.kind = SIT_KEY_END; return key; }
        }
        key.kind = SIT_KEY_ESC;
        return key;
    }

    if (b == 0x1b) { key.kind = SIT_KEY_ESC; return key; }
    if (b == '\r' || b == '\n') { key.kind = SIT_KEY_ENTER; return key; }
    if (b == 127 || b == 8) { key.kind = SIT_KEY_BACKSPACE; return key; }
    if (b == '\t') { key.kind = SIT_KEY_TAB; return key; }
    if (b == 3) {
        key.kind = SIT_KEY_CHAR;
        key.ch = 'c';
        key.mods = SIT_MOD_CTRL;
        return key;
    }
    if (b < 32) {
        key.kind = SIT_KEY_CHAR;
        key.ch = (char)(b + 96);
        key.mods = SIT_MOD_CTRL;
        return key;
    }
    if (b >= 32 && b <= 126) {
        key.kind = SIT_KEY_CHAR;
        key.ch = (char)b;
        return key;
    }
    return key;
}

static sit_key _sit_key_from_escape(void) {
    unsigned char seq[8];
    sit_key key = { SIT_KEY_ESC, 0, 0 };
    ssize_t n = read(STDIN_FILENO, seq, sizeof(seq));
    if (n <= 0) return key;  /* bare Esc */
    if (seq[0] == '[') {
        if (n >= 2 && seq[1] >= 'A' && seq[1] <= 'D') {
            switch (seq[1]) {
            case 'A': key.kind = SIT_KEY_UP; break;
            case 'B': key.kind = SIT_KEY_DOWN; break;
            case 'C': key.kind = SIT_KEY_RIGHT; break;
            case 'D': key.kind = SIT_KEY_LEFT; break;
            }
            return key;
        }
        /* ESC [ 1 ~ / 3 ~ / 4 ~ / 5 ~ / 6 ~ */
        if (n >= 3 && seq[2] == '~') {
            switch (seq[1]) {
            case '1': key.kind = SIT_KEY_HOME; break;
            case '3': key.kind = SIT_KEY_DELETE; break;
            case '4': key.kind = SIT_KEY_END; break;
            case '5': key.kind = SIT_KEY_PAGE_UP; break;
            case '6': key.kind = SIT_KEY_PAGE_DOWN; break;
            default: break;
            }
            return key;
        }
        /* Some terminals: ESC [ H home, ESC [ F end */
        if (n >= 2 && seq[1] == 'H') { key.kind = SIT_KEY_HOME; return key; }
        if (n >= 2 && seq[1] == 'F') { key.kind = SIT_KEY_END; return key; }
    }
    return key;  /* unknown → Esc */
}

sit_b32 sit_poll_event(sit_event* out){
    out->quit = SIT_FALSE;
    out->resized = SIT_FALSE;
    out->key = (sit_key){ SIT_KEY_NONE, 0, 0 };

    if (_sit_term_resized){
        _sit_term_resized = SIT_FALSE;
        out->resized = SIT_TRUE;
        return SIT_TRUE;
    }

    unsigned char b;

    ssize_t n = read(STDIN_FILENO, &b, 1);
    if (n <= 0) return SIT_FALSE;

    if (b == 0x1b){
        out->key = _sit_key_from_escape();
        return SIT_TRUE;
    } else {
        out->key = _sit_key_from_bytes_event(&b, 1);
    }
    
    
    if (out->key.kind == SIT_KEY_CHAR
        && out->key.ch == 'c'
        && (out->key.mods & SIT_MOD_CTRL))
        out->quit = SIT_TRUE;
    return SIT_TRUE;
}

sia_u32 sit_text_width(const char* str){
    sia_u32 n = 0;
    if (!str) return 0; 
    while(str[n]) n++;
    return n;
}

void sit_text_clip(sit_canvas* c,sia_u32 x, sia_u32 y, sia_u32 max_w,
    const char* str, sit_color fg, sit_color bg, sia_u8 attrs){
    
    if (!c || !str || max_w == 0) return;
    sia_u32 cx = x;
    sia_u32 drawn = 0;

    while (*str && drawn < max_w){
        if (cx >= c->width) break;
        sit_put(c, cx, y, (sia_u32)(sia_u8)*str, fg, bg, attrs);
        cx++;
        str++;
        drawn++;
    }
}

void sit_canvas_resize(si_arena* arena, sit_canvas* c, sia_u32 w, sia_u32 h) {
    if (!c || !arena || w == 0 || h == 0) return;
    if (c->width == w && c->height == h) return;
    sia_u32 total = w * h;
    c->cells = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, total);
    c->prev  = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, total);
    c->width = w;
    c->height = h;
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
