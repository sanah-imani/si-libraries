#define SI_ARENA_IMPL
#include "../si_arena.h"
#define SI_TUI_IMPL
#include "../si_tui.h"
#define SI_SHEET_IMPL
#include "../si_sheet.h"
int main(int argc, char** argv) {
    si_arena* arena = sia_create(&(sia_desc){ .desired_max_size = SIA_MiB(4) });
    sit_enter_alt_screen();
    sit_hide_cursor();
    sit_enter_raw_mode();
    sia_u32 w = 80, h = 24;
    sit_get_term_size(&w, &h);
    sit_canvas* canvas = sit_canvas_create(arena, w, h);
    sis_app app;
    sis_app_init(&app, arena);

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
            sis_app_tab_open(&app, argv[i]);
    } else {
        sis_tab* tab = &app.tabs[app.active];
        sis_cell_set(&tab->sheet, 0, 0, "10");
        sis_cell_set(&tab->sheet, 0, 1, "5");
        sis_cell_set(&tab->sheet, 0, 2, "=A1+B1");
        sis_cell_set(&tab->sheet, 1, 0, "=(A1+B1)*2");
        sis_cell_set(&tab->sheet, 1, 1, "=SUM(A1:B1)");
        sis_cell_set(&tab->sheet, 2, 0, "Name");
        sis_cell_set(&tab->sheet, 2, 1, "Qty");
        sis_cell_set(&tab->sheet, 3, 0, "Apple");
        sis_cell_set(&tab->sheet, 3, 1, "3");
    }
    for (;;) {
        sis_tab* active = &app.tabs[app.active];
        sia_u32 data_h = h - SIS_TAB_BAR_H - SIS_COL_HDR_H - SIS_STATUS_H;
        sia_u32 vis_cols = (w > SIS_ROW_HDR_W) ? (w - SIS_ROW_HDR_W) / (active->sheet.col_width + 1) : 1;
        sis_app_set_viewport(&app, data_h, vis_cols);
        sis_app_render(&app, canvas);
        sit_flush(canvas);
        sit_event ev;
        while (sit_poll_event(&ev)) {
            if (ev.quit || app.quit) goto done;
            if (ev.resized) {
                sit_get_term_size(&w, &h);
                sis_app_on_resize(&app, canvas, w, h);
                break;
            }
            sis_app_on_key(&app, ev.key);
        }
    }
done:
    sit_leave_raw_mode();
    sit_show_cursor();
    sit_leave_alt_screen();
    return 0;
}