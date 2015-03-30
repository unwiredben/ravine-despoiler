/* Copyright (C) 2015 Ben Combee
 *
 * Released under the MIT Licence, see LICENSE for details
 */

#include <pebble.h>

static Window *mainWindow = NULL;

static GBitmap *ravineImage = NULL;
static GBitmap *boulderImage = NULL;
static GBitmap *invBoulderImage = NULL;
static GBitmap *planeImage = NULL;
static GBitmap *zeppelinImage = NULL;

static Layer *ravineLayer = NULL;
static BitmapLayer *zeppelinLayer = NULL;
static BitmapLayer *planeLayer = NULL;

static struct PropertyAnimation *zeppelinAnimation = NULL;
static struct PropertyAnimation *planeAnimation = NULL;

// simple 3x5 font for numbers and colon to use
// to form current time in the boulders
//
// ***  **   ***  ***  * *  ***  ***  ***  ***  ***
// * *   *     *    *  * *  *    *      *  * *  * * 
// * *   *   ***  ***  ***  ***  ***    *  ***  ***
// * *   *   *      *    *    *  * *    *  * *    *
// ***  ***  ***  ***    *  ***  ***    *  ***  ***

static const char boulder_font[11][5] = {
    /* 0 */ { 0b111, 0b101, 0b101, 0b101, 0b111 },
    /* 1 */ { 0b110, 0b010, 0b010, 0b010, 0b111 },
    /* 2 */ { 0b111, 0b001, 0b111, 0b100, 0b111 },
    /* 3 */ { 0b111, 0b001, 0b111, 0b001, 0b111 },
    /* 4 */ { 0b101, 0b101, 0b111, 0b001, 0b001 },
    /* 5 */ { 0b111, 0b100, 0b111, 0b001, 0b111 },
    /* 6 */ { 0b111, 0b101, 0b111, 0b101, 0b111 },
    /* 7 */ { 0b111, 0b001, 0b001, 0b001, 0b001 },
    /* 8 */ { 0b111, 0b101, 0b111, 0b101, 0b111 },
    /* 9 */ { 0b111, 0b101, 0b111, 0b001, 0b111 }        
};  

#define RAVINE_WIDTH (20)
#define RAVINE_HEIGHT (10)

// values are 'B' for boulder, 'b' for inverted boulder,
// ' ' for hole, and 'W' for wall.  Initial settings are
// just holes and walls, as holes are initially all reset
// to boulders at the start of a run.
static char ravine[RAVINE_HEIGHT][RAVINE_WIDTH] = {
    { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' },
    { 'W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' },
    { 'W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' },
    { 'W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' },
    { 'W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','W' },
    { 'W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','W' },
    { 'W','W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','W','W' },
    { 'W','W',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','W','W' },
    { 'W','W',' ',' ',' ',' ',' ',' ','W','W','W',' ',' ',' ',' ',' ',' ',' ','W','W' },
    { 'W','W',' ',' ',' ',' ',' ',' ','W','W','W',' ',' ',' ',' ',' ',' ',' ','W','W' }
};

static void draw_boulder_char(int pos, int ch) {
    int rightX = pos * 4 + 4;
    if (pos > 1) { ++rightX; }
    
    for (int i = 0; i < 5; ++i) {
        char boulderRow = boulder_font[ch][i];
        int x = rightX;
        while (boulderRow != 0) {
            if (boulderRow & 1) {
                ravine[i + 1][x] = 'b';
            }
            boulderRow = boulderRow >> 1;
            --x;
        }
    }
}

static void reset_ravine(struct tm *tick_time) {
    for (int i = 0; i < RAVINE_HEIGHT; ++i) {
        for (int j = 0; j < RAVINE_WIDTH; ++j) {
            if (ravine[i][j] != 'W') {
                ravine[i][j] = 'B';
            }
        }
    }
    /* now get current time and use that to paste in inverted boulders */
    int hour = tick_time->tm_hour;
    if (!clock_is_24h_style() && hour > 12) {
        hour -= 12;
    }
    int first = hour / 10;
    if (first > 0)
        draw_boulder_char(0, first);
    draw_boulder_char(1, hour % 10);
    draw_boulder_char(2, tick_time->tm_min / 10);
    draw_boulder_char(3, tick_time->tm_min % 10);
}

/* returns true if any state changed */
static bool step_ravine(void) {
    bool dirty = false;
    for (int i = RAVINE_HEIGHT - 1; i > 0; --i) {
        for (int j = 0; j < RAVINE_WIDTH; ++j) {
            if (ravine[i][j] == ' ' && ravine[i - 1][j] == 'B') {
                ravine[i][j] = 'B';
                ravine[i - 1][j] = ' ';
                dirty = true;
            }
        }
    }
    return dirty;
}

// animations:
// zeppelin goes from left to right (repeating property animation)
// airplane goes from right to left (repeating property animation)
// once a second, one of the two aircraft drops a bomb which is animated until it hits the ravine
// ten times a second, ravine is checked for boulders falling


static void restart_animation(struct Animation *animation, bool finished, void *context) {
    if (finished) {
        animation_schedule(animation);
    }
}

static void start_zeppelin(void) {
    zeppelinAnimation = property_animation_create_layer_frame(
        bitmap_layer_get_layer(zeppelinLayer),
        &GRect(-28, 10, 28, 16),
        &GRect(144 + 28, 10, 28, 16)
    );
    animation_set_duration(property_animation_get_animation(zeppelinAnimation), 10000);
    animation_set_curve(property_animation_get_animation(zeppelinAnimation), AnimationCurveLinear);
    animation_set_handlers(
        property_animation_get_animation(zeppelinAnimation),
        (AnimationHandlers){ NULL, restart_animation },
        NULL);
    animation_schedule(property_animation_get_animation(zeppelinAnimation));
}

static void start_plane(void) {
    planeAnimation = property_animation_create_layer_frame(
        bitmap_layer_get_layer(planeLayer),
        &GRect(144 + 16, 36, 16, 9),
        &GRect(-16, 36, 16, 9)
    );
    animation_set_duration(property_animation_get_animation(planeAnimation), 6000);
    animation_set_delay(property_animation_get_animation(planeAnimation), 2500);
    animation_set_curve(property_animation_get_animation(planeAnimation), AnimationCurveLinear);
    animation_set_handlers(
        property_animation_get_animation(planeAnimation),
        (AnimationHandlers){ NULL, restart_animation },
        NULL);
    animation_schedule(property_animation_get_animation(planeAnimation));
}

static void draw_ravine(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_frame(layer);
    bounds.origin.x = bounds.origin.y = 0;
    
    // draw the ravine bitmap
    graphics_draw_bitmap_in_rect(ctx, ravineImage, bounds);

    // draw the boulders
    bounds = GRect(8, 8, 5, 5);
    for (int i = 0; i < RAVINE_HEIGHT; ++i) {
        bounds.origin.x = 8;
        for (int j = 0; j < RAVINE_WIDTH; ++j) {
            if (ravine[i][j] == 'B') {
                graphics_draw_bitmap_in_rect(ctx, boulderImage, bounds);
            }
            else if (ravine[i][j] == 'b') {
                graphics_draw_bitmap_in_rect(ctx, invBoulderImage, bounds);
            }
            bounds.origin.x += 6;
        }
        bounds.origin.y += 6;
    }
}

// every minute, reset the ravine with the current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    reset_ravine(tick_time);
}

static void handle_init(void) {
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    reset_ravine(timeinfo);
    
    mainWindow = window_create();
    Layer *mainLayer = window_get_root_layer(mainWindow);
    
    /* 144x93 */ ravineImage     = gbitmap_create_with_resource(RESOURCE_ID_RAVINE_IMG);
    /* 5x5 */    boulderImage    = gbitmap_create_with_resource(RESOURCE_ID_BOULDER_IMG);
    /* 5x5 */    invBoulderImage = gbitmap_create_with_resource(RESOURCE_ID_INV_BOULDER_IMG);
    /* 16x9 */   planeImage      = gbitmap_create_with_resource(RESOURCE_ID_PLANE_IMG);
    /* 28x16 */  zeppelinImage   = gbitmap_create_with_resource(RESOURCE_ID_ZEPPELIN_IMG);
    
    ravineLayer = layer_create(GRect(0, 168 - 93, 144, 93));
    layer_set_update_proc(ravineLayer, draw_ravine);

    zeppelinLayer = bitmap_layer_create(GRect(-28, 10, 28, 16));
    bitmap_layer_set_bitmap(zeppelinLayer, zeppelinImage);
    
    planeLayer = bitmap_layer_create(GRect(144 + 16, 32, 16, 9));
    bitmap_layer_set_bitmap(planeLayer, planeImage);

    layer_add_child(mainLayer, ravineLayer);
    layer_add_child(mainLayer, bitmap_layer_get_layer(zeppelinLayer));
    layer_add_child(mainLayer, bitmap_layer_get_layer(planeLayer));
        
    window_stack_push(mainWindow, true);
    
    start_zeppelin();
    start_plane();
    
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void handle_deinit(void) {
    tick_timer_service_unsubscribe();
    animation_unschedule_all();
                         
    property_animation_destroy(planeAnimation);
    property_animation_destroy(zeppelinAnimation);
    
    bitmap_layer_destroy(planeLayer);
    bitmap_layer_destroy(zeppelinLayer);
    layer_destroy(ravineLayer);

    gbitmap_destroy(ravineImage);
    gbitmap_destroy(boulderImage);
    gbitmap_destroy(invBoulderImage);
    gbitmap_destroy(planeImage);
    gbitmap_destroy(zeppelinImage);

    window_destroy(mainWindow);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
