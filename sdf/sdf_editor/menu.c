#include "menu.h"
#include "math.h"

menu_state menu_default(int window[2])
{
    menu_state m;
    m.show           = 0;
    m.item_spacing   = window[0] * 0.03;
    m.window[0]      = window[0];
    m.window[1]      = window[1];
    m.current_action = 0;
    m.h              = fmax(window[1] * 0.037, 37);
    return m;
}

void menu_draw(struct nk_context* ctx, menu_state* state)
{
    if (!state->show) return;
    // reset_pressed buttons
    state->current_action = 0;
    // make height responsive
    state->h = fmax(state->window[1] * 0.037, 37);

    if (nk_begin(ctx, "File Menu", nk_rect(0, 0, state->window[0], state->h), NK_WINDOW_BORDER)) {
        nk_menubar_begin(ctx);
        nk_layout_row_static(ctx, state->window[1] * 0.03, state->item_spacing, 2);
        if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(420, 200))) {
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_menu_item_label(ctx, "New", NK_TEXT_LEFT)) {
                state->current_action = menu_new;
            }
            if (nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT)) {
                state->current_action = menu_open;
            }
            if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT)) {
                state->current_action = menu_save;
            }
            if (nk_menu_item_label(ctx, "Save As", NK_TEXT_LEFT)) {
                state->current_action = menu_save_as;
            }
            if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
                state->current_action = menu_exit;
            }
            nk_menu_end(ctx);
        }
        if (nk_menu_begin_label(ctx, "View", NK_TEXT_LEFT, nk_vec2(120, 200))) {
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_menu_item_label(ctx, "Sidebar", NK_TEXT_LEFT)) {
                state->current_action = menu_view_sidebar;
            }
            nk_menu_end(ctx);
        }
        nk_menubar_end(ctx);
    }
    nk_end(ctx);
}
