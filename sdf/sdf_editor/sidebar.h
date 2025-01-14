#ifndef SIDEBAR_UI_H
#define SIDEBAR_UI_H
#include "nuklear_include.h"

typedef struct sidebar_state{
    int show;

    float y ;
    float width ;
    float height ;

    // current_file_name
    char* filename;
    // saved =  ,  unsaved = *
    char file_state;

    // primitives names
    char** primitives[100];
    int primitives_count;
    int selected_primitive;

}sidebar_state;

sidebar_state sidebar_default(int window[2]);

void sidebar_draw(struct nk_context* ctx, sidebar_state *state);

#endif
