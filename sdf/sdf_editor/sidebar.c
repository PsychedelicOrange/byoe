#include "nuklear_include.h"
#include "sidebar.h"
#include "assert.h"
#include "stdlib.h"
#include "string.h"

sidebar_state sidebar_default(int window[2]){
    sidebar_state s;
    s.show = 0;

    s.width = window[0] * .3;
    s.height = window[1];
    s.y = 0;

    s.filename = malloc(256);
    strcpy(s.filename,"Untitled.sdf");
    s.file_state = '*';

    s.primitives_count = 0;
    s.selected_primitive = 0;

    return s;
};

void label_select(int i, nk_bool selected[100]){
    assert(i < 100);
    for(int k = 0; k < 100; k++){
	selected[k] = nk_false;
    }
    selected[i] = nk_true;
}

void sidebar_draw(struct nk_context* ctx, sidebar_state *state){
    if (!state->show) return;
    // primitives state
    static nk_bool selected[100];
    
    if (nk_begin(ctx, "Sidebar", nk_rect(0, state->y, state->width, state->height), NK_WINDOW_NO_SCROLLBAR))
    {
	{ // label
	    nk_layout_row_dynamic(ctx,20,1);
	    static char title[40];
	    sprintf(title,"sdf-editor - %s%c",state->filename, state->file_state);
	    nk_label(ctx,title,NK_TEXT_ALIGN_LEFT);
	}
	{ // Primitives
	    if(nk_tree_push(ctx, NK_TREE_TAB, "Primitives", NK_MAXIMIZED)) {
		nk_layout_row_dynamic(ctx, 500, 1);
		if(nk_group_begin(ctx,"Primitives", NK_WINDOW_BORDER))
		{
		    nk_layout_row_dynamic(ctx,30,1);
		    for(int i = 0; i < 50; i++){
			if(nk_selectable_label(ctx, "Sphere", NK_TEXT_LEFT, &selected[i])){
			    state->selected_primitive = i;
			    label_select(i,selected);
			}
		    }
		    nk_group_end(ctx);
		}
		nk_tree_pop(ctx);
	    }
	}
	{ // Properties
	    if(nk_tree_push(ctx, NK_TREE_TAB, "Properties", NK_MAXIMIZED)) {
		nk_layout_row_dynamic(ctx, 30, 2);
		static float radius = 4.0;
		nk_property_float(ctx,"radius",0,&radius,10,0.1,0.1);
		nk_tree_pop(ctx);
	    }
	}
    }
    nk_end(ctx);
}

