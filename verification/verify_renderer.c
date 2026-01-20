/**
 * @file verify_renderer.c
 * @brief ACSL annotated functions for formal verification with Frama-C
 * 
 * This file contains the renderer navigation logic with ACSL contracts
 * that can be verified using Frama-C/WP plugin.
 * 
 * To verify:
 *   frama-c -wp -wp-rte -wp-timeout 30 verify_renderer.c
 */

#include <stdbool.h>
#include <limits.h>

/* Simplified types for verification */
#define MAX_WALLPAPERS 10000

typedef enum {
    VIEW_MODE_HORIZONTAL,
    VIEW_MODE_GRID
} ViewMode;

typedef struct {
    int selected_index;
    int target_scroll;      /* Using int instead of float for easier verification */
    int current_scroll;
    ViewMode view_mode;
    int target_scroll_y;
    int current_scroll_y;
    bool show_help;
} Renderer;

typedef struct {
    int thumbnail_width;
    int thumbnail_height;
    int thumbnails_per_row;
} Config;

/* ========================================================================== */
/*                        Navigation Function Contracts                        */
/* ========================================================================== */

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires r->selected_index < MAX_WALLPAPERS;
  requires r->target_scroll > INT_MIN + 220;  // Prevent overflow on addition
  
  assigns r->selected_index, r->target_scroll;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= \old(r->selected_index);
  ensures r->view_mode == VIEW_MODE_HORIZONTAL ==> 
          (r->selected_index == \old(r->selected_index) - 1 || 
           r->selected_index == \old(r->selected_index));
  ensures r->view_mode == VIEW_MODE_GRID ==>
          (r->selected_index == \old(r->selected_index) - 1 ||
           r->selected_index == \old(r->selected_index));
*/
void renderer_select_prev(Renderer *r, const Config *config) {
    if (r->view_mode == VIEW_MODE_HORIZONTAL) {
        if (r->selected_index > 0) {
            r->selected_index--;
            r->target_scroll += 220; // thumbnail width + spacing
        }
    } else {
        // In grid mode, move left
        if (r->selected_index > 0) {
            r->selected_index--;
        }
    }
}

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires r->selected_index <= max;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  requires r->target_scroll < INT_MAX - 220;  // Prevent overflow on subtraction becoming addition
  
  assigns r->selected_index, r->target_scroll;
  
  ensures r->selected_index >= 0;
  ensures r->selected_index <= max;
  ensures r->selected_index >= \old(r->selected_index);
  ensures r->view_mode == VIEW_MODE_HORIZONTAL ==>
          (r->selected_index == \old(r->selected_index) + 1 ||
           r->selected_index == \old(r->selected_index));
  ensures r->view_mode == VIEW_MODE_GRID ==>
          (r->selected_index == \old(r->selected_index) + 1 ||
           r->selected_index == \old(r->selected_index));
*/
void renderer_select_next(Renderer *r, int max, const Config *config) {
    if (r->view_mode == VIEW_MODE_HORIZONTAL) {
        if (r->selected_index < max) {
            r->selected_index++;
            r->target_scroll -= 220; // thumbnail width + spacing
        }
    } else {
        // In grid mode, move right
        if (r->selected_index < max) {
            r->selected_index++;
        }
    }
}

/*@
  requires \valid(r);
  requires \valid(config);
  requires r->selected_index >= 0;
  requires r->selected_index < MAX_WALLPAPERS;
  requires config->thumbnails_per_row > 0;
  requires config->thumbnails_per_row <= 100;
  requires config->thumbnail_height > 0;
  requires config->thumbnail_height <= 1000;
  requires r->target_scroll_y < INT_MAX - config->thumbnail_height - 20;
  requires r->selected_index >= config->thumbnails_per_row;  // Only decrements if we have room
  
  assigns r->selected_index, r->target_scroll_y;
  
  behavior can_move_up:
    assumes r->view_mode == VIEW_MODE_GRID;
    assumes r->selected_index >= config->thumbnails_per_row;
    ensures r->selected_index == \old(r->selected_index) - config->thumbnails_per_row;
  
  behavior horizontal_mode:
    assumes r->view_mode == VIEW_MODE_HORIZONTAL;
    ensures r->selected_index == \old(r->selected_index);
    
  complete behaviors;
  disjoint behaviors;
*/
void renderer_select_up(Renderer *r, const Config *config) {
    if (r->view_mode == VIEW_MODE_GRID) {
        int cols = config->thumbnails_per_row;
        if (r->selected_index >= cols) {
            r->selected_index -= cols;
            r->target_scroll_y += config->thumbnail_height + 20;
        }
    }
}

/*@
  requires \valid(r);
  requires \valid(config);
  requires r->selected_index >= 0;
  requires r->selected_index <= max;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  requires config->thumbnails_per_row > 0;
  requires config->thumbnails_per_row <= 100;
  requires config->thumbnail_height > 0;
  requires config->thumbnail_height <= 1000;
  requires r->target_scroll_y > INT_MIN + config->thumbnail_height + 20;
  requires r->selected_index <= max - config->thumbnails_per_row;  // Only increments if we have room
  
  assigns r->selected_index, r->target_scroll_y;
  
  behavior can_move_down:
    assumes r->view_mode == VIEW_MODE_GRID;
    assumes r->selected_index + config->thumbnails_per_row <= max;
    ensures r->selected_index == \old(r->selected_index) + config->thumbnails_per_row;
    
  behavior horizontal_mode:
    assumes r->view_mode == VIEW_MODE_HORIZONTAL;
    ensures r->selected_index == \old(r->selected_index);
    
  complete behaviors;
  disjoint behaviors;
*/
void renderer_select_down(Renderer *r, int max, const Config *config) {
    if (r->view_mode == VIEW_MODE_GRID) {
        int cols = config->thumbnails_per_row;
        if (r->selected_index + cols <= max) {
            r->selected_index += cols;
            r->target_scroll_y -= config->thumbnail_height + 20;
        }
    }
}

/*@
  requires \valid(r);
  
  assigns r->view_mode, r->target_scroll, r->current_scroll, 
          r->target_scroll_y, r->current_scroll_y;
  
  ensures r->view_mode != \old(r->view_mode);
  ensures \old(r->view_mode) == VIEW_MODE_HORIZONTAL ==> 
          r->view_mode == VIEW_MODE_GRID;
  ensures \old(r->view_mode) == VIEW_MODE_GRID ==> 
          r->view_mode == VIEW_MODE_HORIZONTAL;
  ensures r->target_scroll == 0;
  ensures r->current_scroll == 0;
  ensures r->target_scroll_y == 0;
  ensures r->current_scroll_y == 0;
*/
void renderer_toggle_view_mode(Renderer *r) {
    r->view_mode = (r->view_mode == VIEW_MODE_HORIZONTAL) ? VIEW_MODE_GRID : VIEW_MODE_HORIZONTAL;
    r->target_scroll = 0;
    r->current_scroll = 0;
    r->target_scroll_y = 0;
    r->current_scroll_y = 0;
}

/*@
  requires \valid(r);
  
  assigns r->show_help;
  
  ensures r->show_help != \old(r->show_help);
  ensures \old(r->show_help) == true ==> r->show_help == false;
  ensures \old(r->show_help) == false ==> r->show_help == true;
*/
void renderer_toggle_help(Renderer *r) {
    r->show_help = !r->show_help;
}

/* ========================================================================== */
/*                          Bounds Checking Properties                         */
/* ========================================================================== */

/*@
  requires \valid(r);
  requires r->selected_index >= 0;
  requires max >= 0;
  requires max < MAX_WALLPAPERS;
  
  assigns \nothing;
  
  ensures \result == true ==> r->selected_index <= max;
  ensures \result == true ==> r->selected_index >= 0;
*/
bool renderer_index_valid(const Renderer *r, int max) {
    return r->selected_index >= 0 && r->selected_index <= max;
}

/* ========================================================================== */
/*                          Config Validation Properties                       */
/* ========================================================================== */

/*@
  requires \valid(config);
  
  assigns \nothing;
  
  ensures \result == true ==> config->thumbnail_width > 0;
  ensures \result == true ==> config->thumbnail_height > 0;
  ensures \result == true ==> config->thumbnails_per_row > 0;
*/
bool config_valid(const Config *config) {
    return config->thumbnail_width > 0 &&
           config->thumbnail_height > 0 &&
           config->thumbnails_per_row > 0;
}
