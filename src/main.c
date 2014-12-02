#include <pebble.h>
	
#define KEY_TITLE 0
#define KEY_LOCATION 1
#define KEY_TIME 2
#define KEY_DISPLAY_STRING 3

// UI Elements
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_schedule_layer;

// Fonts
static GFont s_time_font;
static GFont s_schedule_font;

// Graphics
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// Current Lecture Contents
static char current_lecture_layer_buffer[50];

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

static void update_schedule_from_storage(){
	if(persist_exists(KEY_DISPLAY_STRING)){
		persist_read_string(KEY_DISPLAY_STRING, current_lecture_layer_buffer, sizeof(current_lecture_layer_buffer));
		text_layer_set_text(s_schedule_layer, current_lecture_layer_buffer);
	}
}

static void main_window_load(Window *window) {
	// Prepare window properties
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
	// First create the FH logo
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FHJOANNEUM_LOGO);
	s_background_layer = bitmap_layer_create(GRect(5, 5, 134, 47));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
	
	// Create a text layer for time including font
	s_time_layer = text_layer_create(GRect(0, 52, window_bounds.size.w, 50));
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorClear);
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_42));
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

	// Create a text layer for the lecture including font
	s_schedule_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_18));
	s_schedule_layer = text_layer_create(GRect(5, 107, window_bounds.size.w-10, window_bounds.size.h - 102));
	text_layer_set_background_color(s_schedule_layer, GColorClear);
	text_layer_set_text_color(s_schedule_layer, GColorBlack);
	text_layer_set_font(s_schedule_layer, s_schedule_font);
	text_layer_set_text_alignment(s_schedule_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_schedule_layer));
	text_layer_set_text(s_schedule_layer, "Loading...");
	
	// Begin by updating the time.
	update_time();
	
	// Load data from storage
	update_schedule_from_storage();
}

static void main_window_unload(Window *window) {
	// Unload fonts
	fonts_unload_custom_font(s_schedule_font);
	fonts_unload_custom_font(s_time_font);
	
	// Unload text layers
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_schedule_layer);
	
	// Destroy GBitmap
	gbitmap_destroy(s_background_bitmap);

	// Destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	// Update the time every minute.
	update_time();
	
	// Update the lectures every 30 minutes
	if(tick_time->tm_min % 30 == 0) {
		// Begin dictionary
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);

		// Add a key-value pair
		dict_write_uint8(iter, 0, 0);

		// Send the message!
		app_message_outbox_send();
	}
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// Store incoming lecture information
	static char lecture_title_buffer[15];
	static char lecture_location_buffer[15];
	static char lecture_time_buffer[15];
	static char lecture_layer_buffer[50];

	// Read the first item
	Tuple *t = dict_read_first(iterator);
	
	// For each item
	while(t != NULL){
		
		switch(t->key) {
			case KEY_TITLE:
				snprintf(lecture_title_buffer, sizeof(lecture_title_buffer), "%s", t->value->cstring);
				break;
			case KEY_TIME:
				snprintf(lecture_time_buffer, sizeof(lecture_time_buffer), "%s", t->value->cstring);
				break;
			case KEY_LOCATION:
				snprintf(lecture_location_buffer, sizeof(lecture_location_buffer), "%s", t->value->cstring);
				break;
			default:
				APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int) t->key);
				break;
		}
		
		// Get next item
		t = dict_read_next(iterator);
	}
	snprintf(lecture_layer_buffer, sizeof(lecture_layer_buffer), "%s\n%s\n%s", lecture_title_buffer, lecture_time_buffer, lecture_location_buffer);
	text_layer_set_text(s_schedule_layer, lecture_layer_buffer);
	
	// Compare the buffers and decide whether to write to storage
	if(strcmp(lecture_layer_buffer, current_lecture_layer_buffer) != 0){
		persist_write_string(KEY_DISPLAY_STRING, lecture_layer_buffer);
		strcpy(lecture_layer_buffer, current_lecture_layer_buffer);
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init(){
	// Create main window
	s_main_window = window_create();
	
	// Handlers for elements inside the window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	window_stack_push(s_main_window, true);
	
	// Get a time update every minute
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
}

static void deinit(){
	window_destroy(s_main_window);
}

int main(void){
	init();
	app_event_loop();
	deinit();
}