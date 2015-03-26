#include <pebble.h>

static Window *mainWindow = NULL;

static GBitmap *ravineImage = NULL;
static GBitmap *boulderImage = NULL;
static GBitmap *invBoulderImage = NULL;
static GBitmap *planeImage = NULL;
static GBitmap *zeppelinImage = NULL;

static BitmapLayer *ravineLayer = NULL;
static BitmapLayer *zeppelinLayer = NULL;

static PropertyAnimation *zeppelinAnimation = NULL;

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

static void update_zeppelin(struct Animation *animation, const uint32_t distance_normalized) {
    // start zep at x = -30, continue to x = 154
    uint32_t x = (distance_normalized * (154 - -30)
    layer_set_frame(bitmap_layer_get_layer(zeppelinLayer), GRect(distance_normalized, 10, 28, 16));
    layer_set_hidden(bitmap_layer_get_layer(zeppelinLayer), false);
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

    zeppelinLayer = bitmap_layer_create(GRect(10, 10, 28, 16));
    bitmap_layer_set_bitmap(zeppelinLayer, zeppelinImage);
    layer_set_hidden(bitmap_layer_get_layer(zeppelinLayer), true);
    
    layer_add_child(mainLayer, bitmap_layer_get_layer(ravineLayer));
    layer_add_child(mainLayer, bitmap_layer_get_layer(zeppelinLayer));
        
    window_stack_push(mainWindow, true);
    
    zeppelinAnimation = animation_create();
    animation_set_duration(zeppelinAnimation, 10000);
    animation_set_curve(zeppelinAnimation, AnimationCurveLinear);
    animation_set_handlers(zeppelinAnimation, NULL);
    
}

static void handle_deinit(void) {

    app_timer_cancel(zeppelinTimer);
    
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
