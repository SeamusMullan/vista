/**
 * @file verify_core.c
 * @brief Core ACSL annotated functions for formal verification with Frama-C
 * 
 * This file contains simplified renderer navigation logic with ACSL contracts
 * that can be fully verified using Frama-C/WP plugin.
 * 
 * To verify:
 *   eval $(opam env) && frama-c -wp -wp-rte verify_core.c
 * 
 * Verified properties:
 * - Index bounds are maintained (never negative, never exceeds max)
 * - View mode toggle is correct
 * - Help toggle is correct
 */

#include <stdbool.h>

/* ========================================================================== */
/*                              Type Definitions                               */
/* ========================================================================== */

#define MAX_WALLPAPERS 10000

typedef enum {
    VIEW_MODE_HORIZONTAL = 0,
    VIEW_MODE_GRID = 1
} ViewMode;

typedef struct {
    int selected_index;
    ViewMode view_mode;
    bool show_help;
} RendererState;

/* ========================================================================== */
/*                        Index Bounds Verification                           */
/* ========================================================================== */

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  
  assigns r->selected_index;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= \old(r->selected_index);
  ensures \old(r->selected_index) > 0 ==> r->selected_index == \old(r->selected_index) - 1;
  ensures \old(r->selected_index) == 0 ==> r->selected_index == 0;
*/
void select_prev(RendererState *r) {
    if (r->selected_index > 0) {
        r->selected_index--;
    }
}

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires r->selected_index <= max;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns r->selected_index;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= max;
  ensures r->selected_index >= \old(r->selected_index);
  ensures \old(r->selected_index) < max ==> r->selected_index == \old(r->selected_index) + 1;
  ensures \old(r->selected_index) == max ==> r->selected_index == max;
*/
void select_next(RendererState *r, int max) {
    if (r->selected_index < max) {
        r->selected_index++;
    }
}

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires cols > 0;
  requires cols <= 100;
  
  assigns r->selected_index;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= \old(r->selected_index);
  ensures \old(r->selected_index) >= cols ==> 
          r->selected_index == \old(r->selected_index) - cols;
  ensures \old(r->selected_index) < cols ==> 
          r->selected_index == \old(r->selected_index);
*/
void select_up_grid(RendererState *r, int cols) {
    if (r->selected_index >= cols) {
        r->selected_index -= cols;
    }
}

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires r->selected_index <= max;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  requires cols > 0;
  requires cols <= 100;
  
  assigns r->selected_index;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= max;
  ensures r->selected_index >= \old(r->selected_index);
  ensures \old(r->selected_index) + cols <= max ==> 
          r->selected_index == \old(r->selected_index) + cols;
  ensures \old(r->selected_index) + cols > max ==> 
          r->selected_index == \old(r->selected_index);
*/
void select_down_grid(RendererState *r, int max, int cols) {
    if (r->selected_index + cols <= max) {
        r->selected_index += cols;
    }
}

/* ========================================================================== */
/*                         View Mode Toggle Verification                       */
/* ========================================================================== */

/*@
  requires \valid(r);
  requires r->view_mode == VIEW_MODE_HORIZONTAL || r->view_mode == VIEW_MODE_GRID;
  
  assigns r->view_mode;
  
  ensures r->view_mode == VIEW_MODE_HORIZONTAL || r->view_mode == VIEW_MODE_GRID;
  ensures r->view_mode != \old(r->view_mode);
  ensures \old(r->view_mode) == VIEW_MODE_HORIZONTAL ==> r->view_mode == VIEW_MODE_GRID;
  ensures \old(r->view_mode) == VIEW_MODE_GRID ==> r->view_mode == VIEW_MODE_HORIZONTAL;
*/
void toggle_view_mode(RendererState *r) {
    r->view_mode = (r->view_mode == VIEW_MODE_HORIZONTAL) ? VIEW_MODE_GRID : VIEW_MODE_HORIZONTAL;
}

/* ========================================================================== */
/*                          Help Toggle Verification                           */
/* ========================================================================== */

/*@
  requires \valid(r);
  
  assigns r->show_help;
  
  ensures r->show_help != \old(r->show_help);
  ensures \old(r->show_help) == true ==> r->show_help == false;
  ensures \old(r->show_help) == false ==> r->show_help == true;
*/
void toggle_help(RendererState *r) {
    r->show_help = !r->show_help;
}

/* ========================================================================== */
/*                         Index Safety Lemmas                                 */
/* ========================================================================== */

/*@
  requires \valid_read(r);
  requires r->selected_index >= 0;
  requires r->selected_index <= max;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns \nothing;
  
  ensures \result == true;
*/
bool index_in_bounds(const RendererState *r, int max) {
    return r->selected_index >= 0 && r->selected_index <= max;
}

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns r->selected_index;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= max;
*/
void clamp_index(RendererState *r, int max) {
    if (r->selected_index < 0) {
        r->selected_index = 0;
    }
    if (r->selected_index > max) {
        r->selected_index = max;
    }
}

/* ========================================================================== */
/*                      Sequence of Operations (Invariant)                     */
/* ========================================================================== */

/*@ 
  predicate valid_state(RendererState *r, integer max) = 
    \valid(r) && 
    r->selected_index >= 0 && 
    r->selected_index <= max &&
    (r->view_mode == VIEW_MODE_HORIZONTAL || r->view_mode == VIEW_MODE_GRID);
*/

/*@
  requires valid_state(r, max);
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns r->selected_index;
  
  ensures valid_state(r, max);
*/
void safe_navigate_left(RendererState *r, int max) {
    select_prev(r);
}

/*@
  requires valid_state(r, max);
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns r->selected_index;
  
  ensures valid_state(r, max);
*/
void safe_navigate_right(RendererState *r, int max) {
    select_next(r, max);
}
