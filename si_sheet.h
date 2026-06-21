#ifndef SI_SHEET_H
#define SI_SHEET_H

#include <inttypes.h>
#ifndef SIS_FUNC_DEF
#   if defined(SIS_STATIC)
#       define SIS_FUNC_DEF static
#   else
#       define SIS_FUNC_DEF extern
#   endif
#endif

#ifndef SI_TUI_H
#   error "si_tui.h must be included before si_sheet.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SIS_ROW_HDR_W 5
#define SIS_COL_HDR_H 1
#define SIS_STATUS_H  1
#define SIS_EDIT_CAP  256
#define SIS_CMD_CAP   256


#define SIS_TAB_BAR_H  1
#define SIS_MSG_CAP    128
#define SIS_PATH_CAP   512   /* max path length for cmd args / display */
#define SIS_TAB_MAX    32    /* or grow via arena — fixed is simpler v1 */

typedef sia_i32 sis_b32;

typedef struct {
    si_arena* arena;
    sia_u32   rows;
    sia_u32   cols;
    sia_u32   col_width;
    char**    cells;
} sis_sheet;

typedef struct {
    sia_u32 scroll_row;
    sia_u32 scroll_col;
    sia_u32 cursor_row;
    sia_u32 cursor_col;
} sis_view;

typedef enum {
    SIS_MODE_NORMAL,
    SIS_MODE_INSERT,
    SIS_MODE_COMMAND,
    SIS_MODE_VISUAL,
} sis_mode;

typedef struct {
    sis_mode  mode;
    sis_view  view;
    sis_sheet sheet;
    sia_u32   vis_rows;
    sia_u32   vis_cols;
    char      edit_buf[SIS_EDIT_CAP];
    sia_u32   edit_len;
    char      cmd_buf[SIS_CMD_CAP];
    sia_u32   cmd_len;
    sit_b32   quit;
} sis_editor;

typedef struct{
    sis_sheet sheet;
    sis_view view;
    char* path;
    char* title;

    sia_u32 sel_row;
    sia_u32 sel_col;
    sit_b32 selecting;
    sit_b32 dirty;
} sis_tab;

typedef struct {
    const char* data;
    sia_u32 len;
    sia_u32 pos;
}   sis_csv_reader;

typedef struct {
    sia_u32 rows;
    sia_u32 cols;
    char** cells;
} sis_yank_buffer;

typedef struct{
    si_arena* arena;

    sis_tab* tabs;
    sia_u32 tab_count;
    sia_u32 active;
    sia_u32 next_tab_id;

     /* viewport (derived each frame from terminal size) */
     sia_u32   vis_rows;       /* grid body rows (excludes hdr/tab/status) */
     sia_u32   vis_cols;       /* visible columns */

    sis_mode  mode;           /* NORMAL | INSERT | COMMAND */
    char      edit_buf[SIS_EDIT_CAP];
    sia_u32   edit_len;
    char      cmd_buf[SIS_CMD_CAP];
    sia_u32   cmd_len;

    /* clipboard */
    sis_yank_buffer yank;

     /* feedback */
     char      msg[SIS_MSG_CAP];   /* last error/info: "Wrote foo.csv" */
     sia_u32   msg_ttl;            /* frames to show msg; 0 = until next key */
     sit_b32   quit;           /* exit entire app */
} sis_app;

SIS_FUNC_DEF void        sis_sheet_init(sis_sheet* sheet, si_arena* arena,
                            sia_u32 rows, sia_u32 cols, sia_u32 col_width);
SIS_FUNC_DEF const char* sis_cell_get(const sis_sheet* sheet, sia_u32 row, sia_u32 col);
SIS_FUNC_DEF void        sis_cell_set(sis_sheet* sheet, sia_u32 row, sia_u32 col, const char* value);
SIS_FUNC_DEF void        sis_cell_label(sia_u32 row, sia_u32 col, char* buf, sia_u32 buf_cap);

SIS_FUNC_DEF void sis_editor_init(sis_editor* editor, si_arena* arena,
                            sia_u32 rows, sia_u32 cols);
SIS_FUNC_DEF void sis_editor_set_viewport(sis_editor* editor, sia_u32 vis_rows, sia_u32 vis_cols);
SIS_FUNC_DEF void sis_editor_on_key(sis_editor* editor, sit_key key);
SIS_FUNC_DEF void sis_editor_on_resize(sis_editor* editor, sit_canvas* canvas,
                            si_arena* arena, sia_u32 w, sia_u32 h);
SIS_FUNC_DEF void sis_render(const sis_editor* editor, sit_canvas* canvas);

SIS_FUNC_DEF void    sis_app_init(sis_app* app, si_arena* arena);
SIS_FUNC_DEF void    sis_app_set_viewport(sis_app* app, sia_u32 vis_rows, sia_u32 vis_cols);
SIS_FUNC_DEF void    sis_app_on_key(sis_app* app, sit_key key);
SIS_FUNC_DEF void    sis_app_on_resize(sis_app* app, sit_canvas* canvas, sia_u32 w, sia_u32 h);
SIS_FUNC_DEF void    sis_app_render(const sis_app* app, sit_canvas* canvas);
SIS_FUNC_DEF sis_b32 sis_app_tab_new(sis_app* app);
SIS_FUNC_DEF sis_b32 sis_app_tab_open(sis_app* app, const char* path);
SIS_FUNC_DEF void    sis_app_tab_close(sis_app* app);
SIS_FUNC_DEF void    sis_app_tab_next(sis_app* app);
SIS_FUNC_DEF void    sis_app_tab_prev(sis_app* app);

SIS_FUNC_DEF sis_b32 sis_sheet_save_csv(const sis_sheet* sheet, const char* path, char* err, sia_u32 errlen);
SIS_FUNC_DEF sis_b32 sis_sheet_load_csv(sis_sheet* sheet,
    const char* path, char* err, sia_u32 errlen);

#ifdef __cplusplus
}
#endif

#endif /* SI_SHEET_H */

#ifdef SI_SHEET_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SIS_USE_TINYEXPR
#include "tinyexpr.h"
#endif

void sis_sheet_init(sis_sheet* sheet, si_arena* arena, sia_u32 rows, sia_u32 cols, sia_u32 col_width) {
    sheet->arena = arena;
    sheet->rows = rows;
    sheet->cols = cols;
    sheet->col_width = col_width;
    sheet->cells = SIA_PUSH_ZERO_ARRAY(arena, char*, rows * cols);
}

const char* sis_cell_get(const sis_sheet* sheet, sia_u32 row, sia_u32 col) {
    if (!sheet || row >= sheet->rows || col >= sheet->cols) return "";
    char* value = sheet->cells[row * sheet->cols + col];
    return value ? value : "";
}

void sis_cell_set(sis_sheet* sheet, sia_u32 row, sia_u32 col, const char* value) {
    if (!sheet || row >= sheet->rows || col >= sheet->cols) return;

    sia_u32 idx = row * sheet->cols + col;
    if (!value || !value[0]) {
        sheet->cells[idx] = NULL;
        return;
    }

    sia_u32 len = 0;
    while (value[len]) len++;

    char* copy = (char*)sia_push(sheet->arena, len + 1);
    memcpy(copy, value, len + 1);
    sheet->cells[idx] = copy;
}

static sia_u32 sis_text_len(const char* text) {
    sia_u32 len = 0;
    if (!text) return 0;
    while (text[len]) len++;
    return len;
}

static sia_u32 sis_wrap_lines(const char* text, sia_u32 col_w){
    sia_u32 len = sis_text_len(text);
    if (col_w == 0 || len <= col_w) return 1;
    return (len + col_w - 1) / col_w;
}

static void sis_text_slice(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 max_w,
    const char* text, sia_u32 offset,
    sit_color fg, sit_color bg, sia_u8 attrs) {
    if (!canvas || !text || max_w == 0) return;

    sia_u32 len = sis_text_len(text);
    if (offset >= len) return;

    char buf[256];

    sia_u32 n = 0;
    while (n + 1 < sizeof(buf) && n < max_w && offset + n < len){
        buf[n] = text[offset + n];
        n++;
    }
    buf[n] = '\0';
    sit_text_clip(canvas, x, y, max_w, buf, fg, bg, attrs);
}

static void sis_cell_fill(sit_canvas* canvas, sia_u32 x, sia_u32 y, sia_u32 w,
    sit_color fg, sit_color bg, sia_u8 attrs) {
    for (sia_u32 i = 0; i < w; i++)
        sit_put(canvas, x + i, y, ' ', fg, bg, attrs);
}

static const char* sis_app_cell_display(const sis_app* app, const sis_tab* tab,
    sia_u32 row, sia_u32 col);
static double sis_eval_formula(const sis_sheet* sheet, const char* expr, sia_u32 depth);

static sia_u32 sis_app_row_height(const sis_app* app, const sis_tab* tab, sia_u32 row) {

    sia_u32 row_h = 1;
    sia_u32 col_w = tab->sheet.col_width;
    sia_u32 vis_cols = app->vis_cols;

    for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
        sia_u32 col = tab->view.scroll_col + col_idx;
        if (col >= tab->sheet.cols) break;
        const char* text = sis_app_cell_display(app, tab, row, col);
        sia_u32 lines = sis_wrap_lines(text, col_w);
        if (lines > row_h) row_h = lines;
    }
    return row_h;

}

static void sis_app_ensure_col_visible(sis_app* app, sis_tab* tab) {
    sis_view* view = &tab->view;
    if (view->cursor_col < view->scroll_col)
        view->scroll_col = view->cursor_col;
    if (app->vis_cols > 0 && view->cursor_col >= view->scroll_col + app->vis_cols)
        view->scroll_col = view->cursor_col - app->vis_cols + 1;
}
static sia_u32 sis_app_cursor_visual_y(const sis_app* app, const sis_tab* tab) {
    sia_u32 y = 0;
    for (sia_u32 row = tab->view.scroll_row; row < tab->view.cursor_row; row++) {
        y += sis_app_row_height(app, tab, row);
        if (y >= app->vis_rows) break;
    }
    return y;
}

static void sis_app_ensure_visible_wrapped(sis_app* app, sis_tab* tab) {
    if (!app || !tab) return;
    sis_view* view = &tab->view;
    if (view->cursor_row < view->scroll_row)
        view->scroll_row = view->cursor_row;
    while (view->scroll_row < view->cursor_row) {
        sia_u32 cursor_y = sis_app_cursor_visual_y(app, tab);
        if (cursor_y < app->vis_rows) break;
        view->scroll_row++;
    }
    sis_app_ensure_col_visible(app, tab);
}

static void sis_col_label(sia_u32 col, char* buf, sia_u32 buf_cap) {
    if (!buf || buf_cap == 0) return;

    char reversed[8];
    sia_u32 len = 0;

    do {
        reversed[len++] = (char)('A' + (col % 26));
        col = (col / 26) - 1;
    } while ((sia_i32)col >= 0 && len < sizeof(reversed));

    sia_u32 i = 0;
    while (len > 0 && i + 1 < buf_cap)
        buf[i++] = reversed[--len];
    buf[i] = '\0';
}

void sis_cell_label(sia_u32 row, sia_u32 col, char* buf, sia_u32 buf_cap) {
    if (!buf || buf_cap == 0) return;

    char col_name[8];
    sis_col_label(col, col_name, sizeof(col_name));
    snprintf(buf, buf_cap, "%s%u", col_name, row + 1);
}

static void sis_view_ensure_visible(sis_view* view, sia_u32 vis_rows, sia_u32 vis_cols){
    if (view->cursor_row < view->scroll_row)
        view->scroll_row = view->cursor_row;
    if (vis_rows > 0 && view->cursor_row >= view->scroll_row + vis_rows)
        view->scroll_row = view->cursor_row - vis_rows + 1;
    if (view->cursor_col < view->scroll_col)
        view->scroll_col = view->cursor_col;
    if (vis_cols > 0 && view->cursor_col >= view->scroll_col + vis_cols)
        view->scroll_col = view->cursor_col - vis_cols + 1;
}

static void sis_move_cursor(sis_editor* ed, sia_i32 drow, sia_i32 dcol) {
    sis_view* view = &ed->view;
    sia_i32 row = (sia_i32)view->cursor_row + drow;
    sia_i32 col = (sia_i32)view->cursor_col + dcol;
    if (row < 0 || col < 0) return;
    if ((sia_u32)row >= ed->sheet.rows || (sia_u32)col >= ed->sheet.cols) return;
    view->cursor_row = (sia_u32)row;
    view->cursor_col = (sia_u32)col;
    sis_view_ensure_visible(view, ed->vis_rows, ed->vis_cols);
}

static const char* sis_cell_display(const sis_editor* ed, sia_u32 row, sia_u32 col) {
    if (ed->mode == SIS_MODE_INSERT
        && row == ed->view.cursor_row
        && col == ed->view.cursor_col)
        return ed->edit_buf;
    return sis_cell_get(&ed->sheet, row, col);
}

static void sis_enter_insert(sis_editor* ed, sit_b32 clear) {
    if (clear) {
        ed->edit_len = 0;
        ed->edit_buf[0] = '\0';
    } else {
        const char* existing = sis_cell_get(&ed->sheet, ed->view.cursor_row, ed->view.cursor_col);
        ed->edit_len = (sia_u32)strlen(existing);
        if (ed->edit_len >= SIS_EDIT_CAP) ed->edit_len = SIS_EDIT_CAP - 1;
        memcpy(ed->edit_buf, existing, ed->edit_len);
        ed->edit_buf[ed->edit_len] = '\0';
    }
    ed->mode = SIS_MODE_INSERT;
}


static void sis_commit_cell(sis_editor* ed) {
    sis_cell_set(&ed->sheet, ed->view.cursor_row, ed->view.cursor_col, ed->edit_buf);
    ed->edit_len = 0;
    ed->edit_buf[0] = '\0';
    ed->mode = SIS_MODE_NORMAL;
}

static void sis_run_command(sis_editor* ed) {
    const char* cmd = ed->cmd_buf;
    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "q!") == 0
        || strcmp(cmd, "wq") == 0 || strcmp(cmd, "wq!") == 0)
        ed->quit = SIT_TRUE;
    ed->cmd_len = 0;
    ed->cmd_buf[0] = '\0';
    ed->mode = SIS_MODE_NORMAL;
}

static void sis_sheet_clear_cells(sis_sheet* sheet){
    if (!sheet || !sheet->cells) return;
    for (sia_u32 i = 0; i < sheet->rows * sheet->cols; i++)
        sheet->cells[i] = NULL;
}


static void sis_sheet_used_bounds(const sis_sheet* sheet,
    sia_u32* out_rows, sia_u32* out_cols) {
    sia_u32 max_row = 0;
    sia_u32 max_col = 0;
    for (sia_u32 r = 0; r < sheet->rows; r++) {
        for (sia_u32 c = 0; c < sheet->cols; c++) {
            const char* value = sis_cell_get(sheet, r, c);
            if (value && value[0]) {
                if (r + 1 > max_row) max_row = r + 1;
                if (c + 1 > max_col) max_col = c + 1;
            }
        }
    }
    *out_rows = max_row;
    *out_cols = max_col;
}

static sis_b32 sis_csv_needs_quote(const char* text) {
    if (!text) return SIT_FALSE;
    for (const char* p = text; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r')
            return SIT_TRUE;
    }
    return SIT_FALSE;
}

static void sis_csv_write_field(FILE* fp, const char* text){
    if (!text) text = "";

    if (!sis_csv_needs_quote(text)){
        fputs(text, fp);
        return;
    }

    fputc('"', fp);
    for (const char* p = text; *p; p++) {
        if (*p == '"') fputc('"', fp);
        fputc(*p, fp);
    }
    fputc('"', fp);
}


static void sis_set_error(char* err, sia_u32 err_cap, const char* msg) {
    if (!err || err_cap == 0) return;
    snprintf(err, err_cap, "%s", msg ? msg : "error");
}

static int sis_csv_peek(const sis_csv_reader* r){
    return (r->pos < r->len) ? (unsigned char)r->data[r->pos] : -1;
}

static int sis_csv_get(sis_csv_reader* r){
    if (r->pos >= r->len) return -1;
    return (unsigned char)r->data[r->pos++];
}

static sis_b32 sis_csv_read_field(sis_csv_reader* r, char* buf, sia_u32 buf_cap, int* out_eol){
    sia_u32 len = 0;
    int c = sis_csv_peek(r);

    *out_eol = 0;

    if (c == '"') {
        sis_csv_get(r); /* consume opening quote */
        while ((c = sis_csv_get(r)) >= 0) {
            if (c == '"') {
                if (sis_csv_peek(r) == '"') {
                    sis_csv_get(r);
                    if (len + 1 >= buf_cap) return SIT_FALSE;
                    buf[len++] = '"';
                } else {
                    break;
                }
            } else {
                if (len + 1 >= buf_cap) return SIT_FALSE;
                buf[len++] = (char)c;
            }
        }
    } else {
        while ((c = sis_csv_peek(r)) >= 0 && c != ',' && c != '\n' && c != '\r') {
            sis_csv_get(r);
            if (len + 1 >= buf_cap) return SIT_FALSE;
            buf[len++] = (char)c;
        }
    }
    buf[len] = '\0';
    c = sis_csv_peek(r);
    if (c == ',') {
        sis_csv_get(r);
    } else if (c == '\r') {
        sis_csv_get(r);
        if (sis_csv_peek(r) == '\n') sis_csv_get(r);
        *out_eol = 1;
    } else if (c == '\n') {
        sis_csv_get(r);
        *out_eol = 1;
    } else if (c < 0) {
        *out_eol = 1;
    }
    return SIT_TRUE;

}
sis_b32 sis_sheet_save_csv(const sis_sheet* sheet, const char* path, char* err, sia_u32 errlen){
    if (!sheet || !path || !path[0]){
        sis_set_error(err, errlen, "No file name");
        return SIT_FALSE;
    }

    FILE* fp = fopen(path, "wb");

    if (!fp){
        sis_set_error(err, errlen, "Could not open file");
        return SIT_FALSE;
    }

    sia_u32 used_rows = 0;
    sia_u32 used_cols = 0;

    sis_sheet_used_bounds(sheet, &used_rows, &used_cols);

    for (sia_u32 r = 0; r < used_rows; r++){
        for (sia_u32 c = 0; c < used_cols; c++){
            if (c > 0) fputc(',', fp);
            sis_csv_write_field(fp, sis_cell_get(sheet, r, c));
        }
        fputc('\n', fp);
    }

    if (ferror(fp)) {
        fclose(fp);
        sis_set_error(err, errlen, "Write failed");
        return SIT_FALSE;
    }
    fclose(fp);
    return SIT_TRUE;
}

sis_b32 sis_sheet_load_csv(sis_sheet* sheet,
    const char* path, char* err, sia_u32 err_cap) {
    if (!sheet || !path || !path[0]) {
        sis_set_error(err, err_cap, "No file name");
        return SIT_FALSE;
    }
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        sis_set_error(err, err_cap, "Cannot open file");
        return SIT_FALSE;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        sis_set_error(err, err_cap, "Cannot read file");
        return SIT_FALSE;
    }
    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        sis_set_error(err, err_cap, "Cannot read file");
        return SIT_FALSE;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        sis_set_error(err, err_cap, "Cannot read file");
        return SIT_FALSE;
    }
    char* file_data = (char*)sia_push(sheet->arena, (sia_u64)file_size + 1);
    size_t read_bytes = fread(file_data, 1, (size_t)file_size, fp);
    fclose(fp);
    if (read_bytes != (size_t)file_size) {
        sis_set_error(err, err_cap, "Read failed");
        return SIT_FALSE;
    }
    file_data[file_size] = '\0';
    sis_sheet_clear_cells(sheet);
    sis_csv_reader reader = {
        .data = file_data,
        .len = (sia_u32)file_size,
        .pos = 0,
    };
    char field[SIS_EDIT_CAP];
    sia_u32 row = 0;
    sia_u32 col = 0;
    int eol = 0;
    while (reader.pos < reader.len || (row == 0 && col == 0)) {
        if (!sis_csv_read_field(&reader, field, sizeof(field), &eol)) {
            sis_set_error(err, err_cap, "CSV field too large");
            return SIT_FALSE;
        }
        if (field[0]) {
            if (row >= sheet->rows || col >= sheet->cols) {
                sis_set_error(err, err_cap, "CSV exceeds sheet bounds");
                return SIT_FALSE;
            }
            sis_cell_set(sheet, row, col, field);
        }
        col++;
        if (eol) {
            row++;
            col = 0;
        }
        if (reader.pos >= reader.len)
            break;
    }
    return SIT_TRUE;
}

/* Editor API*/
void sis_editor_init(sis_editor* editor, si_arena* arena, sia_u32 rows, sia_u32 cols) {
    *editor = (sis_editor){0};
    editor->mode = SIS_MODE_NORMAL;
    editor->quit = SIT_FALSE;
    sis_sheet_init(&editor->sheet, arena, rows, cols, 10);
}


void sis_editor_set_viewport(sis_editor* editor, sia_u32 vis_rows, sia_u32 vis_cols) {
    editor->vis_rows = vis_rows;
    editor->vis_cols = vis_cols;
    sis_view_ensure_visible(&editor->view, vis_rows, vis_cols);
}

void sis_editor_on_resize(sis_editor* editor, sit_canvas* canvas, si_arena* arena, sia_u32 w, sia_u32 h) {
    (void)editor;
    sit_canvas_resize(arena, canvas, w, h);
}

void sis_editor_on_key(sis_editor* editor, sit_key key) {
    switch (editor->mode) {
        case SIS_MODE_NORMAL:
        if (key.kind == SIT_KEY_CHAR && key.ch == 'h') sis_move_cursor(editor, 0, -1);
        else if (key.kind == SIT_KEY_CHAR && key.ch == 'j') sis_move_cursor(editor, 1, 0);
        else if (key.kind == SIT_KEY_CHAR && key.ch == 'k') sis_move_cursor(editor, -1, 0);
        else if (key.kind == SIT_KEY_CHAR && key.ch == 'l') sis_move_cursor(editor, 0, 1);
        else if (key.kind == SIT_KEY_LEFT)  sis_move_cursor(editor, 0, -1);
        else if (key.kind == SIT_KEY_RIGHT) sis_move_cursor(editor, 0,  1);
        else if (key.kind == SIT_KEY_UP)    sis_move_cursor(editor, -1, 0);
        else if (key.kind == SIT_KEY_DOWN)  sis_move_cursor(editor,  1, 0);
        else if (key.kind == SIT_KEY_CHAR && key.ch == 'i') sis_enter_insert(editor, SIT_FALSE);
        else if (key.kind == SIT_KEY_CHAR && key.ch == 'c') sis_enter_insert(editor, SIT_TRUE);
        else if (key.kind == SIT_KEY_ENTER) sis_enter_insert(editor, SIT_FALSE);
        else if (key.kind == SIT_KEY_CHAR && key.ch == ':') {
            editor->mode = SIS_MODE_COMMAND;
            editor->cmd_len = 0;
            editor->cmd_buf[0] = '\0';
        }
        break;
    case SIS_MODE_INSERT:
        if (key.kind == SIT_KEY_ESC || key.kind == SIT_KEY_ENTER) sis_commit_cell(editor);
        else if (key.kind == SIT_KEY_BACKSPACE && editor->edit_len > 0)
            editor->edit_buf[--editor->edit_len] = '\0';
        else if (key.kind == SIT_KEY_CHAR && editor->edit_len + 1 < SIS_EDIT_CAP) {
            editor->edit_buf[editor->edit_len++] = key.ch;
            editor->edit_buf[editor->edit_len] = '\0';
        }
        break;
    case SIS_MODE_COMMAND:
        if (key.kind == SIT_KEY_ESC) {
            editor->cmd_len = 0;
            editor->cmd_buf[0] = '\0';
            editor->mode = SIS_MODE_NORMAL;
        } else if (key.kind == SIT_KEY_ENTER) {
            sis_run_command(editor);
        } else if (key.kind == SIT_KEY_BACKSPACE && editor->cmd_len > 0) {
            editor->cmd_buf[--editor->cmd_len] = '\0';
        } else if (key.kind == SIT_KEY_CHAR && editor->cmd_len + 1 < SIS_CMD_CAP) {
            editor->cmd_buf[editor->cmd_len++] = key.ch;
            editor->cmd_buf[editor->cmd_len] = '\0';
        }
        break;
    case SIS_MODE_VISUAL:
        editor->mode = SIS_MODE_NORMAL;
        break;
    }
}

void sis_render(const sis_editor* editor, sit_canvas* canvas) {
    sit_clear(canvas);

    sia_u32 h = canvas->height;

    sia_u32 col_w = editor->sheet.col_width;
    sia_u32 cell_stride = col_w + 1;
    sia_u32 vis_cols = editor->vis_cols;
    sia_u32 vis_rows = editor->vis_rows;
    if (vis_cols == 0 || h <= SIS_STATUS_H) return;


    for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
        sia_u32 col = editor->view.scroll_col + col_idx;
        if (col >= editor->sheet.cols) break;
        char label[8];
        sis_col_label(col, label, sizeof(label));
        sia_u32 x = SIS_ROW_HDR_W + col_idx * cell_stride + 1;
        sit_text_clip(canvas, x, 0, col_w, label, SIT_CYAN, SIT_BLACK, SIT_ATTR_BOLD);
    }

    for (sia_u32 row_idx = 0; row_idx < vis_rows; row_idx++) {
        sia_u32 row = editor->view.scroll_row + row_idx;
        if (row >= editor->sheet.rows) break;
        sia_u32 y = SIS_COL_HDR_H + row_idx;

        char row_label[8];
        snprintf(row_label, sizeof(row_label), "%4u ", row + 1);
        sit_text(canvas, 0, y, row_label, SIT_CYAN, SIT_BLACK, SIT_ATTR_NONE);

        for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
            sia_u32 x = SIS_ROW_HDR_W + col_idx * cell_stride;
            sia_u32 col = editor->view.scroll_col + col_idx;
            if (col >= editor->sheet.cols) break;
            sit_b32 active = (row == editor->view.cursor_row && col == editor->view.cursor_col);
            sia_u8 attrs = active ? SIT_ATTR_REVERSE : SIT_ATTR_NONE;
            sit_put(canvas, x, y, '|', SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
            sit_text_clip(canvas, x + 1, y, col_w, sis_cell_display(editor, row, col), SIT_WHITE, SIT_BLACK, attrs);
        }
    }

    /* status / command line */
    char addr[16];
    sis_cell_label(editor->view.cursor_row, editor->view.cursor_col, addr, sizeof(addr));
    if (editor->mode == SIS_MODE_COMMAND) {
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE, ":%s", editor->cmd_buf);
    } else {
        const char* mode =
            editor->mode == SIS_MODE_INSERT ? "INSERT" : "NORMAL";
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE,
            " -- %s --  %s  %s", mode, addr,
            sis_cell_display(editor, editor->view.cursor_row, editor->view.cursor_col));
    }
}

static void sis_range_bounds(sia_u32 a_row, sia_u32 a_col, sia_u32 b_row, sia_u32 b_col,
    sia_u32* min_row, sia_u32* min_col, sia_u32* max_row, sia_u32* max_col) {
    *min_row = SIA_MIN(a_row, b_row);
    *min_col = SIA_MIN(a_col, b_col);
    *max_row = SIA_MAX(a_row, b_row);
    *max_col = SIA_MAX(a_col, b_col);
}

static sis_tab* sis_app_active_tab(sis_app* app);
static sis_b32 sis_app_tab_save(sis_app* app, sis_tab* tab, const char* path);
static sis_b32 sis_app_tab_load(sis_app* app, sis_tab* tab, const char* path);

static sit_b32 sis_app_selection_bounds(const sis_app* app, sia_u32* min_row, sia_u32* min_col, sia_u32* max_row, sia_u32* max_col) {
    sis_tab* tab = sis_app_active_tab((sis_app*)app);
    if (!tab || !tab->selecting) return SIT_FALSE;
    sis_range_bounds(tab->sel_row, tab->sel_col,
        tab->view.cursor_row, tab->view.cursor_col,
        min_row, min_col, max_row, max_col);
    return SIT_TRUE;
}

static char* sis_app_copy_string(sis_app* app, const char* str) {
    if (!app || !str) return NULL;
    sia_u32 len = 0;
    while (str[len]) len++;
    char* copy = (char*)sia_push(app->arena, len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}
static void sis_app_yank_selection(sis_app* app) {
    sia_u32 min_row, min_col, max_row, max_col;
    if (!sis_app_selection_bounds(app, &min_row, &min_col, &max_row, &max_col)) return;
    sis_yank_buffer* yank = &app->yank;
    
    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;

    sia_u32 rows = max_row - min_row + 1;
    sia_u32 cols = max_col - min_col + 1;
    yank->rows = rows;
    yank->cols = cols;
    yank->cells = SIA_PUSH_ZERO_ARRAY(app->arena, char*, rows * cols);
    for (sia_u32 row = 0; row < rows; row++) {
        for (sia_u32 col = 0; col < cols; col++) {
            const char* value = sis_cell_get(&tab->sheet, min_row + row, min_col + col);

            if (value && value[0]){
                yank->cells[row * cols + col] = sis_app_copy_string(app, value);
            }
        }
    }

    tab->selecting = SIT_FALSE;
    app->mode = SIS_MODE_NORMAL;
    snprintf(app->msg, sizeof(app->msg), "Yanked %ux%u cells", rows, cols);
}   


static void sis_app_paste_yank(sis_app* app) {
    sis_yank_buffer* yank = &app->yank;
    if (!app || !yank->cells || yank->rows == 0 || yank->cols == 0) {
        snprintf(app->msg, sizeof(app->msg), "Nothing yanked");
        return;
    }

    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;

    sia_u32 start_row = tab->view.cursor_row;
    sia_u32 start_col = tab->view.cursor_col;

    for (sia_u32 row = 0; row < yank->rows; row++) {
        for (sia_u32 col = 0; col < yank->cols; col++) {
            sia_u32 dst_row = start_row + row;
            sia_u32 dst_col = start_col + col;
            if (dst_row >= tab->sheet.rows || dst_col >= tab->sheet.cols) continue;

            const char* value = yank->cells[row * yank->cols + col];
            sis_cell_set(&tab->sheet, dst_row, dst_col, value ? value : "");
        }
    }

    tab->dirty = SIT_TRUE;
    tab->selecting = SIT_FALSE;
    app->mode = SIS_MODE_NORMAL;
    snprintf(app->msg, sizeof(app->msg), "Pasted %ux%u cells", yank->rows, yank->cols);
}

static sit_b32 sis_tab_cell_selected(const sis_tab* tab, sia_u32 row, sia_u32 col) {
    if (!tab->selecting) return SIT_FALSE;
    sia_u32 min_row, min_col, max_row, max_col;
    sis_range_bounds(tab->sel_row, tab->sel_col,
        tab->view.cursor_row, tab->view.cursor_col,
        &min_row, &min_col, &max_row, &max_col);
    return row >= min_row && col >= min_col && row <= max_row && col <= max_col;
}

static sis_tab* sis_app_active_tab(sis_app* app) {
    if (!app || app->tab_count == 0) return NULL;
    return &app->tabs[app->active];
}


static const sis_tab* sis_app_active_tab_const(const sis_app* app) {
    if (!app || app->tab_count == 0) return NULL;
    return &app->tabs[app->active];
}

static const char* sis_app_skip_spaces(const char* str) {
    while (str && (*str == ' ' || *str == '\t')) str++;
    return str;
}

static sit_b32 sis_app_set_tab_title(sis_app* app, const char* title) {
    sis_tab* tab = sis_app_active_tab(app);
    title = sis_app_skip_spaces(title);
    if (!tab || !title || !title[0]) return SIT_FALSE;
    tab->title = sis_app_copy_string(app, title);
    snprintf(app->msg, sizeof(app->msg), "Tab renamed to %s", tab->title);
    return SIT_TRUE;
}

static const char* sis_app_cell_display(const sis_app* app, const sis_tab* tab,
                                        sia_u32 row, sia_u32 col) {
    if (app->mode == SIS_MODE_INSERT
        && row == tab->view.cursor_row
        && col == tab->view.cursor_col)
        return app->edit_buf;

    const char* raw = sis_cell_get(&tab->sheet, row, col);
    if (!raw || raw[0] != '=') return raw;

    static char formula_buf[64];
    double value = sis_eval_formula(&tab->sheet, raw + 1, 0);
    snprintf(formula_buf, sizeof(formula_buf), "%.10g", value);
    return formula_buf;
}

static void sis_app_move_cursor(sis_app* app, sia_i32 drow, sia_i32 dcol) {
    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;
    
    sis_view* view = &tab->view;
    sia_i32 row = (sia_i32)view->cursor_row + drow;
    sia_i32 col = (sia_i32)view->cursor_col + dcol;
    if (row < 0 || col < 0) return;
    if ((sia_u32)row >= tab->sheet.rows || (sia_u32)col >= tab->sheet.cols) return;
    view->cursor_row = (sia_u32)row;
    view->cursor_col = (sia_u32)col;
    sis_app_ensure_visible_wrapped(app, tab);
}

static void sis_app_enter_insert(sis_app* app, sit_b32 clear) {
    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;
    if (clear) {
        app->edit_len = 0;
        app->edit_buf[0] = '\0';
    } else {
        const char* existing = sis_cell_get(&tab->sheet, tab->view.cursor_row, tab->view.cursor_col);
        app->edit_len = (sia_u32)strlen(existing);
        if (app->edit_len >= SIS_EDIT_CAP) app->edit_len = SIS_EDIT_CAP - 1;
        memcpy(app->edit_buf, existing, app->edit_len);
        app->edit_buf[app->edit_len] = '\0';
    }
    app->mode = SIS_MODE_INSERT;
}

static void sis_app_commit_cell(sis_app* app) {
    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;
    sis_cell_set(&tab->sheet, tab->view.cursor_row, tab->view.cursor_col, app->edit_buf);
    tab->dirty = SIT_TRUE;
    app->edit_len = 0;
    app->edit_buf[0] = '\0';
    app->mode = SIS_MODE_NORMAL;
}

static void sis_app_run_command(sis_app* app) {
    const char* cmd = app->cmd_buf;
    sis_tab* tab = sis_app_active_tab(app);

    if (strcmp(cmd, "q") == 0) {
        if (tab && tab->dirty) {
            snprintf(app->msg, sizeof(app->msg), "Unsaved changes; use :q!");
        } else {
            sis_app_tab_close(app);
        }
    } else if (strcmp(cmd, "q!") == 0) {
        sis_app_tab_close(app);
    } else if (strcmp(cmd, "wq") == 0) {
        if (tab && tab->path && tab->path[0]) {
            if (sis_app_tab_save(app, tab, tab->path))
                sis_app_tab_close(app);
        } else {
            snprintf(app->msg, sizeof(app->msg), "No file name");
        }
    } else if (strcmp(cmd, "wq!") == 0) {
        if (tab && tab->path && tab->path[0]) {
            if (sis_app_tab_save(app, tab, tab->path))
                sis_app_tab_close(app);
        } else {
            sis_app_tab_close(app);
        }
    } else if (strcmp(cmd, "w") == 0) {
        if (tab && tab->path && tab->path[0]) {
            sis_app_tab_save(app, tab, tab->path);
        } else {
            snprintf(app->msg, sizeof(app->msg), "No file name");
        }
    } else if (cmd[0] == 'w' && (cmd[1] == ' ' || cmd[1] == '\t')) {
        const char* path = sis_app_skip_spaces(cmd + 1);
        if (tab && path && path[0]) {
            sis_app_tab_save(app, tab, path);
        } else {
            snprintf(app->msg, sizeof(app->msg), "No file name");
        }
    } else if (cmd[0] == 'e' && (cmd[1] == ' ' || cmd[1] == '\t')) {
        const char* path = sis_app_skip_spaces(cmd + 1);
        if (path && path[0]) {
            sis_app_tab_open(app, path);
        } else {
            snprintf(app->msg, sizeof(app->msg), "No file name");
        }
    } else if (strcmp(cmd, "enew") == 0 || strcmp(cmd, "tabnew") == 0) {
        sis_app_tab_new(app);
    } else if (strcmp(cmd, "bn") == 0 || strcmp(cmd, "tabnext") == 0) {
        sis_app_tab_next(app);
    } else if (strcmp(cmd, "bp") == 0 || strcmp(cmd, "tabprev") == 0) {
        sis_app_tab_prev(app);
    } else if (strncmp(cmd, "name", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ' || cmd[4] == '\t')) {
        sis_app_set_tab_title(app, cmd + 4);
    } else if (strncmp(cmd, "tabname", 7) == 0 && (cmd[7] == '\0' || cmd[7] == ' ' || cmd[7] == '\t')) {
        sis_app_set_tab_title(app, cmd + 7);
    }

    app->cmd_len = 0;
    app->cmd_buf[0] = '\0';
    app->mode = SIS_MODE_NORMAL;
}

void sis_app_init(sis_app* app, si_arena* arena) {
    *app = (sis_app){0};
    app->arena = arena;
    app->tabs = SIA_PUSH_ZERO_ARRAY(arena, sis_tab, SIS_TAB_MAX);
    app->mode = SIS_MODE_NORMAL;
    app->quit = SIT_FALSE;
    sis_app_tab_new(app);
}

sis_b32 sis_app_tab_new(sis_app* app) {
    if (!app || app->tab_count >= SIS_TAB_MAX) return SIT_FALSE;
    sia_u32 idx = app->tab_count++;
    sis_tab* tab = &app->tabs[idx];
    *tab = (sis_tab){0};
    sis_sheet_init(&tab->sheet, app->arena, 1000, 26, 10);
    tab->path = NULL;
    char title[32];
    snprintf(title, sizeof(title), "Sheet %u", ++app->next_tab_id);
    tab->title = sis_app_copy_string(app, title);
    tab->dirty = SIT_FALSE;
    app->active = idx;
    return SIT_TRUE;
}

static sis_b32 sis_app_tab_save(sis_app* app, sis_tab* tab, const char* path) {
    if (!app || !tab) return SIT_FALSE;
    char err[SIS_MSG_CAP];
    if (!sis_sheet_save_csv(&tab->sheet, path, err, sizeof(err))) {
        snprintf(app->msg, sizeof(app->msg), "%s", err);
        return SIT_FALSE;
    }
    tab->path = sis_app_copy_string(app, path);
    tab->title = sis_app_copy_string(app, path);
    tab->dirty = SIT_FALSE;
    snprintf(app->msg, sizeof(app->msg), "Wrote %s", path);
    return SIT_TRUE;
}

static sis_b32 sis_app_tab_load(sis_app* app, sis_tab* tab, const char* path){
    if (!app || !tab) return SIT_FALSE;

    char err[SIS_MSG_CAP];
    if (!sis_sheet_load_csv(&tab->sheet,  path, err, sizeof(err))){
        snprintf(app->msg, sizeof(app->msg), "%s", err);
        return SIT_FALSE;
    }

    tab->path = sis_app_copy_string(app, path);
    tab->title = sis_app_copy_string(app, path);
    tab->dirty = SIT_FALSE;
    tab->view.cursor_row = 0;
    tab->view.cursor_col = 0;
    tab->view.scroll_row = 0;
    tab->view.scroll_col = 0;
    snprintf(app->msg, sizeof(app->msg), "Opened %s", path);
    return SIT_TRUE;
}

sis_b32 sis_app_tab_open(sis_app* app, const char* path) {
    if (!sis_app_tab_new(app)) return SIT_FALSE;
    sis_tab* tab = sis_app_active_tab(app);
    if (!tab || !path || !path[0]) return SIT_FALSE;
    return sis_app_tab_load(app, tab, path);
}


void sis_app_tab_next(sis_app* app) {
    if (!app || app->tab_count == 0) return;
    app->active = (app->active + 1) % app->tab_count;
}


void sis_app_tab_prev(sis_app* app) {
    if (!app || app->tab_count == 0) return;
    app->active = app->active == 0 ? app->tab_count - 1 : app->active - 1;
}


void sis_app_tab_close(sis_app* app) {
    if (!app || app->tab_count == 0) return;
    if (app->tab_count == 1) {
        app->quit = SIT_TRUE;
        return;
    }
    for (sia_u32 i = app->active; i + 1 < app->tab_count; i++)
        app->tabs[i] = app->tabs[i + 1];
    app->tab_count--;
    if (app->active >= app->tab_count)
        app->active = app->tab_count - 1;
}

void sis_app_set_viewport(sis_app* app, sia_u32 vis_rows, sia_u32 vis_cols) {
    if (!app) return;

    app->vis_rows = vis_rows;
    app->vis_cols = vis_cols;

    sis_tab* tab = sis_app_active_tab(app);
    if (!tab) return;
    sis_app_ensure_visible_wrapped(app, tab);
}

void sis_app_on_resize(sis_app* app, sit_canvas* canvas, sia_u32 w, sia_u32 h) {
    if (!app) return;
    sit_canvas_resize(app->arena, canvas, w, h);
}

void sis_app_on_key(sis_app* app, sit_key key) {
    if (!app || app->tab_count == 0) return;

    switch (app->mode){
        case SIS_MODE_NORMAL:
            if (key.kind == SIT_KEY_CHAR && key.ch == 'h') sis_app_move_cursor(app, 0, -1);
            else if (key.kind == SIT_KEY_CHAR && key.ch == 'j') sis_app_move_cursor(app, 1, 0);
            else if (key.kind == SIT_KEY_CHAR && key.ch == 'k') sis_app_move_cursor(app, -1, 0);
            else if (key.kind == SIT_KEY_CHAR && key.ch == 'l') sis_app_move_cursor(app, 0, 1);
            else if (key.kind == SIT_KEY_LEFT)  sis_app_move_cursor(app, 0, -1);
            else if (key.kind == SIT_KEY_RIGHT) sis_app_move_cursor(app, 0,  1);
            else if (key.kind == SIT_KEY_UP)    sis_app_move_cursor(app, -1, 0);
            else if (key.kind == SIT_KEY_DOWN)  sis_app_move_cursor(app,  1, 0);
            else if (key.kind == SIT_KEY_CHAR && key.ch == 'i') sis_app_enter_insert(app, SIT_FALSE);
            else if (key.kind == SIT_KEY_CHAR && key.ch == 'c') sis_app_enter_insert(app, SIT_TRUE);
            else if (key.kind == SIT_KEY_ENTER) sis_app_enter_insert(app, SIT_FALSE);
            else if (key.kind == SIT_KEY_CHAR && key.ch == ':') {
                app->mode = SIS_MODE_COMMAND;
                app->cmd_len = 0;
                app->cmd_buf[0] = '\0';
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'v') {
                sis_tab* tab = sis_app_active_tab(app);
                if (tab) {
                    tab->selecting = SIT_TRUE;
                    tab->sel_row = tab->view.cursor_row;
                    tab->sel_col = tab->view.cursor_col;
                    app->mode = SIS_MODE_VISUAL;
                }
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'p') {
                sis_app_paste_yank(app);
            }
            break;

        case SIS_MODE_INSERT:
            if (key.kind == SIT_KEY_ESC || key.kind == SIT_KEY_ENTER) {
                sis_app_commit_cell(app);
            } else if (key.kind == SIT_KEY_BACKSPACE && app->edit_len > 0) {
                app->edit_buf[--app->edit_len] = '\0';
            } else if (key.kind == SIT_KEY_CHAR && app->edit_len + 1 < SIS_EDIT_CAP) {
                app->edit_buf[app->edit_len++] = key.ch;
                app->edit_buf[app->edit_len] = '\0';
            }
            break;
        case SIS_MODE_COMMAND:
            if (key.kind == SIT_KEY_ESC) {
                app->cmd_len = 0;
                app->cmd_buf[0] = '\0';
                app->mode = SIS_MODE_NORMAL;
            } else if (key.kind == SIT_KEY_ENTER) {
                sis_app_run_command(app);
            } else if (key.kind == SIT_KEY_BACKSPACE && app->cmd_len > 0) {
                app->cmd_buf[--app->cmd_len] = '\0';
            } else if (key.kind == SIT_KEY_CHAR && app->cmd_len + 1 < SIS_CMD_CAP) {
                app->cmd_buf[app->cmd_len++] = key.ch;
                app->cmd_buf[app->cmd_len] = '\0';
            }
            break;
        case SIS_MODE_VISUAL: {
            sis_tab* tab = sis_app_active_tab(app);
            if (key.kind == SIT_KEY_ESC) {
                if (tab) tab->selecting = SIT_FALSE;
                app->mode = SIS_MODE_NORMAL;
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'h') {
                sis_app_move_cursor(app, 0, -1);
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'j') {
                sis_app_move_cursor(app, 1, 0);
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'k') {
                sis_app_move_cursor(app, -1, 0);
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'l') {
                sis_app_move_cursor(app, 0, 1);
            } else if (key.kind == SIT_KEY_LEFT) {
                sis_app_move_cursor(app, 0, -1);
            } else if (key.kind == SIT_KEY_RIGHT) {
                sis_app_move_cursor(app, 0, 1);
            } else if (key.kind == SIT_KEY_UP) {
                sis_app_move_cursor(app, -1, 0);
            } else if (key.kind == SIT_KEY_DOWN) {
                sis_app_move_cursor(app, 1, 0);
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'y') {
                sis_app_yank_selection(app);
            } else if (key.kind == SIT_KEY_CHAR && key.ch == 'p') {
                sis_app_paste_yank(app);
            }
        } break;
        }

    }

void sis_app_render(const sis_app* app, sit_canvas* canvas) {
    sit_clear(canvas);
    if (!app || app->tab_count == 0) return;

    const sis_tab* tab = sis_app_active_tab_const(app);
    if (!tab) return;

    sia_u32 h = canvas->height;
    sia_u32 col_w = tab->sheet.col_width;
    sia_u32 cell_stride = col_w + 1;
    sia_u32 vis_cols = app->vis_cols;
    if (vis_cols == 0 || h <= SIS_TAB_BAR_H + SIS_STATUS_H) return;

    sia_u32 tab_x = 0;
    for (sia_u32 i = 0; i < app->tab_count; i++) {
        const sis_tab* t = &app->tabs[i];
        const char* title = t->title ? t->title : (t->path ? t->path : "[No Name]");
        sia_u8 attrs = (i == app->active) ? SIT_ATTR_REVERSE : SIT_ATTR_NONE;
        sit_textf(canvas, tab_x, 0, SIT_WHITE, SIT_BLACK, attrs,
                  " %u:%s%s ", i + 1, title, t->dirty ? "*" : "");
        tab_x += 12;
    }

    sia_u32 header_y = SIS_TAB_BAR_H;
    for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
        sia_u32 col = tab->view.scroll_col + col_idx;
        if (col >= tab->sheet.cols) break;
        char label[8];
        sis_col_label(col, label, sizeof(label));
        sia_u32 x = SIS_ROW_HDR_W + col_idx * cell_stride + 1;
        sit_text_clip(canvas, x, header_y, col_w, label, SIT_CYAN, SIT_BLACK, SIT_ATTR_BOLD);
    }

    sia_u32 y = SIS_TAB_BAR_H + SIS_COL_HDR_H;
    sia_u32 status_y = h - SIS_STATUS_H;

    for (sia_u32 row = tab->view.scroll_row; row < tab->sheet.rows && y < status_y; row++) {
        sia_u32 row_h = sis_app_row_height(app, tab, row);
        if (row_h == 0) row_h = 1;

        char row_label[8];
        snprintf(row_label, sizeof(row_label), "%4u ", row + 1);

        for (sia_u32 line = 0; line < row_h && y + line < status_y; line++) {
            if (line == 0)
                sit_text(canvas, 0, y + line, row_label, SIT_CYAN, SIT_BLACK, SIT_ATTR_NONE);

            for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
                sia_u32 x = SIS_ROW_HDR_W + col_idx * cell_stride;
                sia_u32 col = tab->view.scroll_col + col_idx;
                if (col >= tab->sheet.cols) break;

                sit_b32 active = (row == tab->view.cursor_row && col == tab->view.cursor_col);
                sit_b32 selected = sis_tab_cell_selected(tab, row, col);
                sia_u8 attrs = (active || selected) ? SIT_ATTR_REVERSE : SIT_ATTR_NONE;
                const char* text = sis_app_cell_display(app, tab, row, col);

                sit_put(canvas, x, y + line, '|', SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
                sis_cell_fill(canvas, x + 1, y + line, col_w, SIT_WHITE, SIT_BLACK, attrs);
                sis_text_slice(canvas, x + 1, y + line, col_w, text, line * col_w,
                    SIT_WHITE, SIT_BLACK, attrs);
            }
        }
        y += row_h;
    }

    char addr[16];
    sis_cell_label(tab->view.cursor_row, tab->view.cursor_col, addr, sizeof(addr));
    if (app->mode == SIS_MODE_COMMAND) {
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE, ":%s", app->cmd_buf);
    } else if (app->msg[0] && app->mode == SIS_MODE_NORMAL) {
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE, "%s", app->msg);
    } else {
        const char* mode =
            app->mode == SIS_MODE_INSERT ? "INSERT" :
            app->mode == SIS_MODE_VISUAL ? "VISUAL" :
            "NORMAL";
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE,
                  " -- %s --  %s  %s", mode, addr,
                  sis_app_cell_display(app, tab, tab->view.cursor_row, tab->view.cursor_col));
    }
}

static const char* sis_formula_skip_ws(const char* text) {
    while (*text == ' ' || *text == '\t') text++;
    return text;
}

static sis_b32 sis_is_alpha(char c){
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static sis_b32 sis_is_digit(char c){
    return c >= '0' && c <= '9';
}

static char sis_upper(char c){
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; 
}

static sis_b32 sis_parse_cell_ref(const char* s, sia_u32* out_row, sia_u32* out_col,
    const char** out_end) {
    s = sis_formula_skip_ws(s);
    if (!sis_is_alpha(*s)) return SIT_FALSE;

    sia_u32 col = 0;
    while (sis_is_alpha(*s)) {
        col = col * 26 + (sia_u32)(sis_upper(*s) - 'A' + 1);
        s++;
    }
    if (!sis_is_digit(*s)) return SIT_FALSE;

    sia_u32 row = 0;
    while (sis_is_digit(*s)) {
        row = row * 10 + (sia_u32)(*s - '0');
        s++;
    }

    if (row == 0 || col == 0) return SIT_FALSE;
    *out_row = row - 1;
    *out_col = col - 1;
    if (out_end) *out_end = s;
    return SIT_TRUE;
}

static double sis_cell_number(const sis_sheet* sheet, sia_u32 row, sia_u32 col, sia_u32 depth){
    if (!sheet || row >= sheet->rows || col >= sheet->cols) return 0.0;
    if (depth > 32) return 0.0; /* crude cycle guard */

    const char* raw = sis_cell_get(sheet, row, col);
    if (!raw || !raw[0]) return 0.0;

    if (raw[0] == '=') {
        return sis_eval_formula(sheet, raw + 1, depth + 1);
    }
    char* end = NULL;
    double value = strtod(raw, &end);
    return end != raw ? value : 0.0;
}

static double sis_eval_atom(const sis_sheet* sheet, const char* text, const char** out_end, sia_u32 depth) {
    text = sis_formula_skip_ws(text);

    sia_u32 row, col;

    const char* end = NULL;

    if (sis_parse_cell_ref(text, &row, &col, &end)){
        if (out_end) *out_end = end;
        return sis_cell_number(sheet, row, col, depth + 1);
    }

    char* num_end = NULL;
    double value = strtod(text, &num_end);
    if (num_end != text) {
        if (out_end) *out_end = num_end;
        return value;
    }

    if (out_end) *out_end = text;
    return 0.0;
}


static sis_b32 sis_parse_range(const char* text, sia_u32* row0, sia_u32* col0, sia_u32* row1, sia_u32* col1, const char** out_end){
    const char* end = NULL;
    if (!sis_parse_cell_ref(text, row0, col0, &end)){
        return SIT_FALSE;
    }

    end = sis_formula_skip_ws(end);
    if (*end != ':') return SIT_FALSE;
    end++;

    if (!sis_parse_cell_ref(end, row1, col1, &end)) return SIT_FALSE;
    if (*row1 < *row0) { sia_u32 t = *row0; *row0 = *row1; *row1 = t; }
    if (*col1 < *col0) { sia_u32 t = *col0; *col0 = *col1; *col1 = t; }
    if (out_end) *out_end = end;
    return SIT_TRUE;
}

static sis_b32 sis_match_func(const char* s, const char* name, const char** after_open) {
    while (*name) {
        if (sis_upper(*s++) != *name++) return SIT_FALSE;
    }
    s = sis_formula_skip_ws(s);
    if (*s != '(') return SIT_FALSE;
    *after_open = s + 1;
    return SIT_TRUE;
}

static sis_b32 sis_eval_range_func(const sis_sheet* sheet, const char* expr,
    double* out_value, sia_u32 depth) {
    const char* args = NULL;
    int mode = 0; /* 1 sum, 2 avg, 3 min, 4 max */

    if (sis_match_func(expr, "SUM", &args)) mode = 1;
    else if (sis_match_func(expr, "AVG", &args)) mode = 2;
    else if (sis_match_func(expr, "MIN", &args)) mode = 3;
    else if (sis_match_func(expr, "MAX", &args)) mode = 4;
    else return SIT_FALSE;

    sia_u32 r0, c0, r1, c1;
    const char* end = NULL;
    if (!sis_parse_range(args, &r0, &c0, &r1, &c1, &end)) return SIT_FALSE;

    end = sis_formula_skip_ws(end);
    if (*end != ')') return SIT_FALSE;

    double acc = 0.0;
    sia_u32 count = 0;
    for (sia_u32 r = r0; r <= r1 && r < sheet->rows; r++) {
        for (sia_u32 c = c0; c <= c1 && c < sheet->cols; c++) {
            double v = sis_cell_number(sheet, r, c, depth + 1);
            if (count == 0) acc = v;
            else if (mode == 1 || mode == 2) acc += v;
            else if (mode == 3 && v < acc) acc = v;
            else if (mode == 4 && v > acc) acc = v;
            count++;
        }
    }

    if (mode == 2 && count > 0) acc /= (double)count;
    *out_value = acc;
    return SIT_TRUE;
}

#ifdef SIS_USE_TINYEXPR
#ifndef SIS_FORMULA_MAX_VARS
#define SIS_FORMULA_MAX_VARS 64
#endif

#ifndef SIS_FORMULA_VAR_NAME_CAP
#define SIS_FORMULA_VAR_NAME_CAP 16
#endif

typedef struct {
    char names[SIS_FORMULA_MAX_VARS][SIS_FORMULA_VAR_NAME_CAP];
    double values[SIS_FORMULA_MAX_VARS];
    te_variable vars[SIS_FORMULA_MAX_VARS];
    int count;
} sis_formula_vars;

static sis_b32 sis_formula_add_ref(const sis_sheet* sheet, sis_formula_vars* vars,
                                   const char* begin, const char* end, sia_u32 depth) {
    sia_u32 name_len = (sia_u32)(end - begin);
    if (name_len == 0 || name_len >= SIS_FORMULA_VAR_NAME_CAP) return SIT_FALSE;

    for (int i = 0; i < vars->count; i++) {
        if (strlen(vars->names[i]) == name_len &&
            strncmp(vars->names[i], begin, name_len) == 0) {
            return SIT_TRUE;
        }
    }

    if (vars->count >= SIS_FORMULA_MAX_VARS) return SIT_FALSE;

    sia_u32 row = 0;
    sia_u32 col = 0;
    const char* parsed_end = NULL;
    if (!sis_parse_cell_ref(begin, &row, &col, &parsed_end) || parsed_end != end) {
        return SIT_FALSE;
    }

    int idx = vars->count++;
    memcpy(vars->names[idx], begin, name_len);
    vars->names[idx][name_len] = '\0';
    vars->values[idx] = sis_cell_number(sheet, row, col, depth + 1);
    vars->vars[idx].name = vars->names[idx];
    vars->vars[idx].address = &vars->values[idx];
    vars->vars[idx].type = TE_VARIABLE;
    vars->vars[idx].context = NULL;
    return SIT_TRUE;
}

static sis_b32 sis_formula_collect_refs(const sis_sheet* sheet, const char* expr,
                                        sis_formula_vars* vars, sia_u32 depth) {
    const char* cursor = expr;
    while (*cursor) {
        sia_u32 row = 0;
        sia_u32 col = 0;
        const char* end = NULL;

        if (sis_parse_cell_ref(cursor, &row, &col, &end) && end > cursor) {
            (void)row;
            (void)col;
            if (!sis_formula_add_ref(sheet, vars, cursor, end, depth)) {
                return SIT_FALSE;
            }
            cursor = end;
        } else {
            cursor++;
        }
    }
    return SIT_TRUE;
}

static sis_b32 sis_eval_formula_tinyexpr(const sis_sheet* sheet, const char* expr,
                                         double* out_value, sia_u32 depth) {
    sis_formula_vars vars = {0};
    if (!sis_formula_collect_refs(sheet, expr, &vars, depth)) {
        return SIT_FALSE;
    }

    int err = 0;
    te_expr* compiled = te_compile(expr, vars.vars, vars.count, &err);
    if (!compiled) return SIT_FALSE;

    *out_value = te_eval(compiled);
    te_free(compiled);
    return SIT_TRUE;
}
#endif

static double sis_eval_formula(const sis_sheet* sheet, const char* expr, sia_u32 depth) {
    if (depth > 32) return 0.0;
    double func_value = 0.0;
    if (sis_eval_range_func(sheet, expr, &func_value, depth)) {
        return func_value;
    }
#ifdef SIS_USE_TINYEXPR
    double tinyexpr_value = 0.0;
    if (sis_eval_formula_tinyexpr(sheet, expr, &tinyexpr_value, depth)) {
        return tinyexpr_value;
    }
#endif
    const char* end = NULL;
    double lhs = sis_eval_atom(sheet, expr, &end, depth);
    end = sis_formula_skip_ws(end);
    if (*end == '\0') return lhs;
    char op = *end++;
    double rhs = sis_eval_atom(sheet, end, &end, depth);
    switch (op) {
        case '+': return lhs + rhs;
        case '-': return lhs - rhs;
        case '*': return lhs * rhs;
        case '/': return rhs != 0.0 ? lhs / rhs : 0.0;
        default:  return lhs;
    }
}

#endif /* SI_SHEET_IMPL */
