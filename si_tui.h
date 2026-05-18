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
SIT_FUN_DEF void sit_sparkline(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w, sia_u32 h, const float* values, sia)
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

sit_canvas* sit_canvas_create(si_arena* arena, sia_u32 width, sia_u32 height) {
    sit_canvas* canvas = SIT_PUSH_ZERO_STRUCT(arena, sit_canvas);
    c->arena = arena;
    c->width = width;
    c->height = height;
    c->cells = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, width * height);
    c->prev = SIA_PUSH_ZERO_ARRAY(arena, sit_cell, width * height);
    c->clear_fg = SIT_WHITE;
    c->clear_bg = SIT_BLACK;
    return c;
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