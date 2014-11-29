#include <pebble.h>
	
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_schedule_layer;
static GFont s_time_font;
static GFont s_schedule_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

static void main_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FHJOANNEUM_LOGO);
	s_background_layer = bitmap_layer_create(GRect(5, 5, 134, 47));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
	
	s_time_layer = text_layer_create(GRect(0, 52, window_bounds.size.w, 50));
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorClear);
	
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TREBUCHET_42));
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

	s_schedule_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TREBUCHET_18));
	s_schedule_layer = text_layer_create(GRect(5, 107, window_bounds.size.w-10, window_bounds.size.h - 102));
	text_layer_set_background_color(s_schedule_layer, GColorClear);
	text_layer_set_text_color(s_schedule_layer, GColorBlack);
	text_layer_set_font(s_schedule_layer, s_schedule_font);
	text_layer_set_text_alignment(s_schedule_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_schedule_layer));
	text_layer_set_text(s_schedule_layer, "ENG02\n8:30 - 10:00\nSR66");
																		

}

static void main_window_unload(Window *window) {
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_schedule_layer);
	fonts_unload_custom_font(s_schedule_font);
	fonts_unload_custom_font(s_time_font);
	// Destroy GBitmap
	gbitmap_destroy(s_background_bitmap);

	// Destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
}

static void init(){
	s_main_window = window_create();
	
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	window_stack_push(s_main_window, true);
	update_time();
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
}

static void deinit(){
	window_destroy(s_main_window);
}

int main(void){
	init();
	app_event_loop();
	deinit();
}