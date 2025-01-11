#include "nuklear_include.h"

typedef struct sidebar_state{
    int show;

    float y ;
    float width ;
    float height ;
}sidebar_state;

sidebar_state sidebar_default(int window[2]){
    sidebar_state s;
    s.show = 0;

    s.y = 0;
    s.width = window[0] * .3;
    s.height = window[1];
    return s;
};

void sidebar_draw(struct nk_context* ctx, sidebar_state *state){
    if (!state->show) return;
    if (nk_begin(ctx, "Sidebar", nk_rect(0, state->y, state->width, state->height), 0))
    {
	
    }
    nk_end(ctx);
}

