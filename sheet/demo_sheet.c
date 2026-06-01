#define SI_ARENA_IMPL
#include "../si_arena.h"
#define SI_TUI_IMPL
#include "../si_tui.h"
#define SI_SHEET_IMPL
#include "../si_sheet.h"
int main(void) {
    si_arena* arena = sia_create(&(sia_desc){ .desired_max_size = SIA_MiB(4) });
    sit_enter_alt_screen();
    sit_hide_cursor();
    sit_enter_raw_mode();
    sia_u32 w = 80, h = 24;
    sit_get_term_size(&w, &h);
    sit_canvas* canvas = sit_canvas_create(arena, w, h);
    sis_editor ed;
    sis_editor_init(&ed, arena, 1000, 26);
    sis_cell_set(&ed.sheet, 0, 0, "Name");
    sis_cell_set(&ed.sheet, 0, 1, "Qty");
    sis_cell_set(&ed.sheet, 1, 0, "Apple");
    sis_cell_set(&ed.sheet, 1, 1, "3");
    for (;;) {
        sia_u32 data_h = h - SIS_COL_HDR_H - SIS_STATUS_H;
        sia_u32 vis_cols = (w > SIS_ROW_HDR_W) ? (w - SIS_ROW_HDR_W) / (ed.sheet.col_width + 1) : 1;
        sis_editor_set_viewport(&ed, data_h, vis_cols);
        sis_render(&ed, canvas);
        sit_flush(canvas);
        sit_event ev;
        while (sit_poll_event(&ev)) {
            if (ev.quit || ed.quit) goto done;
            if (ev.resized) {
                sit_get_term_size(&w, &h);
                sis_editor_on_resize(&ed, canvas, arena, w, h);
                break;
            }
            sis_editor_on_key(&ed, ev.key);
        }
    }
done:
    sit_leave_raw_mode();
    sit_show_cursor();
    sit_leave_alt_screen();
    return 0;
}