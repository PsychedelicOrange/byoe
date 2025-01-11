#include "nuklear_include.h"

typedef struct sidebar_state{
    int show;

    float y ;
    float width ;
    float height ;
}sidebar_state;

sidebar_state sidebar_default(int window[2]);

void sidebar_draw(struct nk_context* ctx, sidebar_state *state);

