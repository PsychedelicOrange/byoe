#include "add_primitive.h"
#include "icon.h"

add_primitive_state add_primitive_default(int window[2])
{
    add_primitive_state p = {0};
    p.show                = 0;
    p.action              = add_primitive_no_action;
    p.x                   = window[0] / 2.0;
    p.y                   = window[1] / 2.0;
    p.window[0]           = window[0];
    p.window[1]           = window[1];
    p.sphere              = icon_load("./resources/prims/sphere.png");
    p.box                 = icon_load("./resources/prims/cube.png");
    p.cone                = icon_load("./resources/prims/cone.png");
    p.torus               = icon_load("./resources/prims/torus.png");
    return p;
}
void draw_add_primitive(struct nk_context* ctx, add_primitive_state* state)
{
    if (!state->show) return;
    if (nk_begin(ctx, "Add primitive", nk_rect(state->x, state->y, state->window[0] / 6.0, state->window[1] / 3.0), NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        nk_layout_row_dynamic(ctx, state->window[0] / 6.0, 1);
        if (nk_group_begin(ctx, "Primitives", NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 0, 1);
            if (nk_button_image_label(ctx, state->sphere, "sphere", NK_TEXT_CENTERED)) {
                //state->selected_primitive = sphere;
                state->action = add_primitive_add;
            }
            if (nk_button_image_label(ctx, state->box, "box", NK_TEXT_CENTERED)) {
                state->action = add_primitive_add;
            }
            if (nk_button_image_label(ctx, state->cone, "cone", NK_TEXT_CENTERED)) {
                state->action = add_primitive_add;
            }
            if (nk_button_image_label(ctx, state->torus, "torus", NK_TEXT_CENTERED)) {
                state->action = add_primitive_add;
            }
            nk_group_end(ctx);
        }
    };
    nk_end(ctx);
}
