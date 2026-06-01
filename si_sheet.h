#ifndef SI_SHEET_H
#define SI_SHEET_H

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
#endif /* SI_SHEET_IMPL */
