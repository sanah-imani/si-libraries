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
    sit_b32 dirty;
} sis_tab;

typedef struct{
    si_arena* arena;

    sis_tab* tabs;
    sia_u32 tab_count;
    sia_u32 active;

     /* viewport (derived each frame from terminal size) */
     sia_u32   vis_rows;       /* grid body rows (excludes hdr/tab/status) */
     sia_u32   vis_cols;       /* visible columns */

    sis_mode  mode;           /* NORMAL | INSERT | COMMAND */
    char      edit_buf[SIS_EDIT_CAP];
    sia_u32   edit_len;
    char      cmd_buf[SIS_CMD_CAP];
    sia_u32   cmd_len;

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

#ifdef __cplusplus
}
#endif

#endif /* SI_SHEET_H */

#ifdef SI_SHEET_IMPL

#include <stdio.h>
#include <string.h>

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

static sis_tab* sis_app_active_tab(sis_app* app) {
    if (!app || app->tab_count == 0) return NULL;
    return &app->tabs[app->active];
}


static const sis_tab* sis_app_active_tab_const(const sis_app* app) {
    if (!app || app->tab_count == 0) return NULL;
    return &app->tabs[app->active];
}

static char* sis_app_copy_string(sis_app* app, const char* str) {
    if (!app || !str) return NULL;
    sia_u32 len = 0;
    while (str[len]) len++;
    char* copy = (char*)sia_push(app->arena, len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

static const char* sis_app_cell_display(const sis_app* app, const sis_tab* tab,
                                        sia_u32 row, sia_u32 col) {
    if (app->mode == SIS_MODE_INSERT
        && row == tab->view.cursor_row
        && col == tab->view.cursor_col)
        return app->edit_buf;
    return sis_cell_get(&tab->sheet, row, col);
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
    sis_view_ensure_visible(view, app->vis_rows, app->vis_cols);
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
    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "q!") == 0) {
        sis_app_tab_close(app);
    } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "wq!") == 0) {
        /* Save later; for now this behaves like close. */
        sis_app_tab_close(app);
    } else if (strcmp(cmd, "enew") == 0 || strcmp(cmd, "tabnew") == 0) {
        sis_app_tab_new(app);
    } else if (strcmp(cmd, "bn") == 0 || strcmp(cmd, "tabnext") == 0) {
        sis_app_tab_next(app);
    } else if (strcmp(cmd, "bp") == 0 || strcmp(cmd, "tabprev") == 0) {
        sis_app_tab_prev(app);
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
    tab->dirty = SIT_FALSE;
    app->active = idx;
    return SIT_TRUE;
}

sis_b32 sis_app_tab_open(sis_app* app, const char* path) {
    if (!sis_app_tab_new(app)) return SIT_FALSE;
    sis_tab* tab = sis_app_active_tab(app);
    if (tab && path && path[0])
        tab->path = sis_app_copy_string(app, path);
    return SIT_TRUE;
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
    sis_view_ensure_visible(&tab->view, vis_rows, vis_cols);
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
    sia_u32 vis_rows = app->vis_rows;
    if (vis_cols == 0 || h <= SIS_TAB_BAR_H + SIS_STATUS_H) return;

    sia_u32 tab_x = 0;
    for (sia_u32 i = 0; i < app->tab_count; i++) {
        const sis_tab* t = &app->tabs[i];
        const char* title = t->path ? t->path : "[No Name]";
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

    for (sia_u32 row_idx = 0; row_idx < vis_rows; row_idx++) {
        sia_u32 row = tab->view.scroll_row + row_idx;
        if (row >= tab->sheet.rows) break;
        sia_u32 y = SIS_TAB_BAR_H + SIS_COL_HDR_H + row_idx;
        if (y >= h - SIS_STATUS_H) break;

        char row_label[8];
        snprintf(row_label, sizeof(row_label), "%4u ", row + 1);
        sit_text(canvas, 0, y, row_label, SIT_CYAN, SIT_BLACK, SIT_ATTR_NONE);

        for (sia_u32 col_idx = 0; col_idx < vis_cols; col_idx++) {
            sia_u32 x = SIS_ROW_HDR_W + col_idx * cell_stride;
            sia_u32 col = tab->view.scroll_col + col_idx;
            if (col >= tab->sheet.cols) break;
            sit_b32 active = (row == tab->view.cursor_row && col == tab->view.cursor_col);
            sia_u8 attrs = active ? SIT_ATTR_REVERSE : SIT_ATTR_NONE;
            sit_put(canvas, x, y, '|', SIT_GRAY, SIT_BLACK, SIT_ATTR_NONE);
            sit_text_clip(canvas, x + 1, y, col_w,
                          sis_app_cell_display(app, tab, row, col),
                          SIT_WHITE, SIT_BLACK, attrs);
        }
    }

    char addr[16];
    sis_cell_label(tab->view.cursor_row, tab->view.cursor_col, addr, sizeof(addr));
    if (app->mode == SIS_MODE_COMMAND) {
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE, ":%s", app->cmd_buf);
    } else if (app->msg[0]) {
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE, "%s", app->msg);
    } else {
        const char* mode = app->mode == SIS_MODE_INSERT ? "INSERT" : "NORMAL";
        sit_textf(canvas, 0, h - 1, SIT_WHITE, SIT_BLACK, SIT_ATTR_NONE,
                  " -- %s --  %s  %s", mode, addr,
                  sis_app_cell_display(app, tab, tab->view.cursor_row, tab->view.cursor_col));
    }
}
#endif /* SI_SHEET_IMPL */
