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

static BitmapLayer *ravineLayer = NULL;
static BitmapLayer *zeppelinLayer = NULL;

static struct PropertyAnimation *zeppelinAnimation = NULL;

// values are 'B' for boulder, 'b' for inverted boulder,
// ' ' for hole, and 'W' for wall.  Initial settings are
// just holes and walls, as holes are initially all reset
// to boulders at the start of a run.
static char ravine[10][20] = {
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

static void restart_animation(struct Animation *animation, bool finished, void *context) {
    if (finished) {
        animation_schedule(animation);
    }
}

static void handle_init(void) {
    mainWindow = window_create();
    Layer *mainLayer = window_get_root_layer(mainWindow);
    
    /* 144x93 */ ravineImage     = gbitmap_create_with_resource(RESOURCE_ID_RAVINE_IMG);
    /* 5x5 */    boulderImage    = gbitmap_create_with_resource(RESOURCE_ID_BOULDER_IMG);
    /* 5x5 */    invBoulderImage = gbitmap_create_with_resource(RESOURCE_ID_INV_BOULDER_IMG);
    /* 16x9 */   planeImage      = gbitmap_create_with_resource(RESOURCE_ID_PLANE_IMG);
    /* 28x16 */  zeppelinImage   = gbitmap_create_with_resource(RESOURCE_ID_ZEPPELIN_IMG);
    
    ravineLayer = bitmap_layer_create(GRect(0, 168 - 93, 144, 93));
    bitmap_layer_set_bitmap(ravineLayer, ravineImage);

    zeppelinLayer = bitmap_layer_create(GRect(-28, 10, 28, 16));
    bitmap_layer_set_bitmap(zeppelinLayer, zeppelinImage);
    
    layer_add_child(mainLayer, bitmap_layer_get_layer(ravineLayer));
    layer_add_child(mainLayer, bitmap_layer_get_layer(zeppelinLayer));
        
    window_stack_push(mainWindow, true);
    
    zeppelinAnimation = property_animation_create_layer_frame(
        bitmap_layer_get_layer(zeppelinLayer),
        NULL,
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

static void handle_deinit(void) {
    bitmap_layer_destroy(ravineLayer);

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
