#ifndef PRIMITIVE_UI_H
#define PRIMITIVE_UI_H
#include "nuklear_include.h"

typedef enum add_primitive_action
{
    add_primitive_no_action,
    add_primitive_add
} add_primitive_action;

typedef struct add_primitive_state
{
    int show;

    float           x, y;
    int             window[2];
    struct nk_image sphere;
    struct nk_image box;
    struct nk_image cone;
    struct nk_image torus;

    add_primitive_action action;
    // use primitive_type enum here
    int selected_primitive;

} add_primitive_state;

add_primitive_state add_primitive_default(int window[2]);

void draw_add_primitive(struct nk_context* ctx, add_primitive_state* state);

#endif
