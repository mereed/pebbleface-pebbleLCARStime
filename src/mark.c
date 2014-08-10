#include <pebble.h>

Window* window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static TextLayer *date2_layer;
static TextLayer *day_layer;
TextLayer *layer_ampm_text;

static BitmapLayer *background_layer;
static BitmapLayer *mouth_layer;
static TextLayer *battery_layer;

static GBitmap *face;
static GBitmap *mouth;

static PropertyAnimation *mouth_animation_beg = NULL;
static PropertyAnimation *mouth_animation_end = NULL;

static GRect mouth_from_rect;
static GRect mouth_to_rect;

static bool status_showing = false;
static bool battery_hide = false;

BitmapLayer *layer_conn_img;

GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;

static char date_buffer[] = "0000.000";
static char date2_buffer[] = "xxxxxxxxxx  00";
static char time_buffer[] = "00:00";
static char day_buffer[] = "xxxxxxxxx";
char *time_format;
int cur_day = -1;


static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "xxxxxxx xxxxx 100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "Battery Level %d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
	if (charge_state.is_plugged) battery_hide = false;
	else if (charge_state.charge_percent <= 30) battery_hide = false;
	else if (status_showing) battery_hide = false;
	else battery_hide = true;
	layer_set_hidden(text_layer_get_layer(battery_layer), battery_hide);

}

//UPDATE TIME
void time_handler(struct tm *tick_time, TimeUnits units_changed) {

	static char ampm_text[] = "A";
    char *time_format;

    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;
		
      strftime(date_buffer, sizeof(date_buffer), "%Y.%j", tick_time);
	  text_layer_set_text(date_layer, date_buffer);
		
	  strftime(day_buffer, sizeof(day_buffer), "%A", tick_time);
      text_layer_set_text(day_layer, day_buffer);
		
	  strftime(date2_buffer, sizeof(date2_buffer), "%B  %d", tick_time);
      text_layer_set_text(date2_layer, date2_buffer);	
		
	}
	
    if (clock_is_24h_style()) {
        time_format = "%R";
		
    } else {
        time_format = "%I:%M";
		
		strftime(ampm_text, sizeof(ampm_text), "%p", tick_time);
        text_layer_set_text(layer_ampm_text, ampm_text);
		
    }

    strftime(time_buffer, sizeof(time_buffer), time_format, tick_time);
	
// removes leading 0
//    if (!clock_is_24h_style() && (time_buffer[0] == '0')) {
//        memmove(time_buffer, &time_buffer[1], sizeof(time_buffer) - 1);
//    }

    text_layer_set_text(time_layer, time_buffer);
}

void destroy_property_animation(PropertyAnimation **prop_animation) {
    if (*prop_animation == NULL) {
        return;
    }

    if (animation_is_scheduled((Animation*) *prop_animation)) {
        animation_unschedule((Animation*) *prop_animation);
    }

    property_animation_destroy(*prop_animation);
    *prop_animation = NULL;
}

void animation_started(Animation *animation, void *data) {
    (void)animation;
    (void)data;
}

void animation_stopped(Animation *animation, void *data) {
    (void)animation;
    (void)data;

    destroy_property_animation(&mouth_animation_end);
    mouth_animation_end = property_animation_create_layer_frame(bitmap_layer_get_layer(mouth_layer), &mouth_to_rect, &mouth_from_rect);
	animation_set_duration ((Animation*) mouth_animation_end, 2000 );
	animation_set_delay	(( Animation *) mouth_animation_end, 1500 );	
    animation_schedule((Animation*) mouth_animation_end);
	
}

void animatenow() {
    destroy_property_animation(&mouth_animation_beg);
	mouth_animation_beg = property_animation_create_layer_frame(bitmap_layer_get_layer(mouth_layer), &mouth_from_rect, &mouth_to_rect);
	animation_set_duration ((Animation*) mouth_animation_beg, 1500 );
    animation_set_handlers ((Animation*) mouth_animation_beg, (AnimationHandlers) {
        .started = (AnimationStartedHandler) animation_started,
        .stopped = (AnimationStoppedHandler) animation_stopped
    }, 0);

    // section ends
	
    animation_schedule((Animation*) mouth_animation_beg);
}

void handle_bluetooth(bool connected) {

	if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
        vibes_long_pulse();
    }
}

void force_update(void) {
    handle_bluetooth(bluetooth_connection_service_peek());
}


// Shows animation
void show_status() {
	status_showing = true;
    handle_battery(battery_state_service_peek());
    animatenow();
}

// Shake/Tap Handler. On shake/tap... call "show_status"
void tap_handler(AccelAxisType axis, int32_t direction) {
	show_status();	
}

static void window_load(Window *window) {
Layer *root_layer = window_get_root_layer(window);

img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
	
face = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FACE);
mouth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOUTH);

background_layer = bitmap_layer_create(layer_get_frame(root_layer));
bitmap_layer_set_bitmap(background_layer, face);
layer_add_child(root_layer, bitmap_layer_get_layer(background_layer));

mouth_from_rect = GRect(0, 0, 144, 168);
mouth_to_rect = GRect(144, 0, 144, 168);
    
mouth_layer = bitmap_layer_create(mouth_from_rect);
bitmap_layer_set_bitmap(mouth_layer, mouth);
layer_add_child(root_layer, bitmap_layer_get_layer(mouth_layer));

layer_conn_img  = bitmap_layer_create(GRect(0, 0, 144, 168));
layer_add_child(bitmap_layer_get_layer(mouth_layer), (Layer*) layer_conn_img); 

//Load fonts
ResHandle date_font_handle = resource_get_handle(RESOURCE_ID_FONT_LCARS_20);
ResHandle time_font_handle = resource_get_handle(RESOURCE_ID_FONT_LCARS_60);
ResHandle day_font_handle = resource_get_handle(RESOURCE_ID_FONT_LCARS_30);
ResHandle bat_font_handle = resource_get_handle(RESOURCE_ID_FONT_LCARS_20);

//Day/-Date layer
date_layer = text_layer_create(GRect(0, 123, 144, 168));
text_layer_set_background_color(date_layer, GColorClear);
text_layer_set_text_color(date_layer, GColorWhite);
text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
text_layer_set_font(date_layer, fonts_load_custom_font(date_font_handle));
layer_add_child(bitmap_layer_get_layer(mouth_layer), (Layer*) date_layer);	

//Month-Date layer
date2_layer = text_layer_create(GRect(0, 92, 144, 168));
text_layer_set_background_color(date2_layer, GColorClear);
text_layer_set_text_color(date2_layer, GColorWhite);
text_layer_set_text_alignment(date2_layer, GTextAlignmentCenter);
text_layer_set_font(date2_layer, fonts_load_custom_font(bat_font_handle));
layer_add_child(bitmap_layer_get_layer(background_layer), (Layer*) date2_layer);	
	
//Time layer
time_layer = text_layer_create(GRect(0, 30, 144, 168));
text_layer_set_background_color(time_layer, GColorClear);
text_layer_set_text_color(time_layer, GColorWhite);
text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
text_layer_set_font(time_layer, fonts_load_custom_font(time_font_handle));
layer_add_child(bitmap_layer_get_layer(mouth_layer), (Layer*) time_layer);

//day layer
day_layer = text_layer_create(GRect(0, 90, 144, 168));
text_layer_set_background_color(day_layer, GColorClear);
text_layer_set_text_color(day_layer, GColorWhite);
text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);
text_layer_set_font(day_layer, fonts_load_custom_font(day_font_handle));
layer_add_child(bitmap_layer_get_layer(mouth_layer), (Layer*) day_layer);

//ampm layer
layer_ampm_text = text_layer_create(GRect(120, 70, 15, 168));
text_layer_set_background_color(layer_ampm_text, GColorClear);
text_layer_set_text_color(layer_ampm_text, GColorWhite);
text_layer_set_text_alignment(layer_ampm_text, GTextAlignmentLeft);
text_layer_set_font(layer_ampm_text, fonts_load_custom_font(date_font_handle));
layer_add_child(bitmap_layer_get_layer(mouth_layer), (Layer*) layer_ampm_text);	
	
//Battery layer
battery_layer = text_layer_create(GRect(0, 115, 144, 168));
text_layer_set_background_color(battery_layer, GColorClear);
text_layer_set_text_color(battery_layer, GColorWhite);
text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
text_layer_set_font(battery_layer, fonts_load_custom_font(bat_font_handle));
layer_add_child(bitmap_layer_get_layer(background_layer), (Layer*) battery_layer);
		
//Get a time structure so that the face doesn't start blank
struct tm *t;
time_t temp;	
temp = time(NULL);	
t = localtime(&temp);	

//Manually call the tick handler when the window is loading
time_handler(t, MINUTE_UNIT);

}

static void window_unload(Window *window) {
    destroy_property_animation(&mouth_animation_beg);
    destroy_property_animation(&mouth_animation_end);

    bitmap_layer_destroy(mouth_layer);
    bitmap_layer_destroy(background_layer);

    gbitmap_destroy(mouth);
    gbitmap_destroy(face);
	
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    
	tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) time_handler);
    battery_state_service_subscribe(&handle_battery);
	bluetooth_connection_service_subscribe(&handle_bluetooth);
	accel_tap_service_subscribe(tap_handler);

    window_stack_push(window, animated);
	
	show_status();

}
	
static void deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    accel_tap_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();

    layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
    bitmap_layer_destroy(layer_conn_img);
    gbitmap_destroy(img_bt_connect);
    gbitmap_destroy(img_bt_disconnect);
	
	text_layer_destroy( time_layer);
	text_layer_destroy( date_layer);
	text_layer_destroy( date2_layer);
	text_layer_destroy( day_layer);
	text_layer_destroy( layer_ampm_text);
	text_layer_destroy( battery_layer);
	
	window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}