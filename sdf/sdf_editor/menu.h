/* MENU */
#include "nuklear_include.h"

typedef enum menu_action{
    menu_no_action,
    menu_open,
    menu_new,
    menu_save,
    menu_save_as,
    menu_exit
}menu_action;

typedef struct menu_state{
    int show;
    float item_spacing;
    menu_action current_action;
    int window[2];
}menu_state;

menu_state menu_default(int window[2]);

void menu_draw(struct nk_context* ctx, menu_state* state);
