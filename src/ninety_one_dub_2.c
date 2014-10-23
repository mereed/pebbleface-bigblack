#include "pebble.h"

GColor background_color = GColorBlack;

static Window *window;

static uint8_t batteryPercent;

static AppSync sync;
static uint8_t sync_buffer[160];

static int blink;
static int invert;
static int bluetoothvibe;
static int hourlyvibe;
static int germanlang;

static bool appStarted = false;

enum {
  BLINK_KEY = 0x0,
  INVERT_KEY = 0x1,
  BLUETOOTHVIBE_KEY = 0x2,
  HOURLYVIBE_KEY = 0x3,
  GERMAN_KEY = 0x4
};

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;

static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

static GBitmap *gday_name_image;
static BitmapLayer *gday_name_layer;

const int DAY_NAME_IMAGE_GERMAN_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SONNTAG,
  RESOURCE_ID_IMAGE_DAY_NAME_MONTAG,
  RESOURCE_ID_IMAGE_DAY_NAME_DIENSTAG,
  RESOURCE_ID_IMAGE_DAY_NAME_MITTWOCH,
  RESOURCE_ID_IMAGE_DAY_NAME_DONNERSTAG,
  RESOURCE_ID_IMAGE_DAY_NAME_FREITAG,
  RESOURCE_ID_IMAGE_DAY_NAME_SAMSTAG
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_image[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9,
  RESOURCE_ID_IMAGE_TINY_PERCENT
};

InverterLayer *inverter_layer = NULL;


void set_invert_color(bool invert) {
  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
  // No action required
}

void change_battery_icon(bool charging) {
  gbitmap_destroy(battery_image);
  if(charging) {
    battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
  }
  else {
    battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  }  
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_mark_dirty(bitmap_layer_get_layer(battery_image_layer));
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  gbitmap_destroy(old_image);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);



static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {

	switch (key) {
    case BLINK_KEY:
      blink = new_tuple->value->uint8;
	  persist_write_bool(BLINK_KEY, blink);
      tick_timer_service_unsubscribe();
      if(blink) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	  }
	  
      break;
     case INVERT_KEY:
      invert = new_tuple->value->uint8 != 0;
      persist_write_bool(INVERT_KEY, invert);
      set_invert_color(invert);
      break;
	  
	case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;	
	case GERMAN_KEY:
      germanlang = new_tuple->value->uint8 != 0;
	  persist_write_bool(GERMAN_KEY, germanlang);	  
	
		if (germanlang) {
           layer_set_hidden(bitmap_layer_get_layer(day_name_layer), true);
	  	   layer_set_hidden(bitmap_layer_get_layer(gday_name_layer), false);
        }  else {
  			layer_set_hidden(bitmap_layer_get_layer(day_name_layer), false);
	  		layer_set_hidden(bitmap_layer_get_layer(gday_name_layer), true);
        }
      break;	
  }
}

static void update_battery(BatteryChargeState charge_state) {

  batteryPercent = charge_state.charge_percent;

  if(batteryPercent==100) {
        change_battery_icon(false);
        layer_set_hidden(bitmap_layer_get_layer(battery_layer), false);
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
      layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), true);
    }  
    return;
  }

  layer_set_hidden(bitmap_layer_get_layer(battery_layer), charge_state.is_charging);
  change_battery_icon(charge_state.is_charging);

 for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), false);
  }  
  set_container_image(&battery_percent_image[0], battery_percent_layers[0], TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(22, 2));
  set_container_image(&battery_percent_image[1], battery_percent_layers[1], TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(30, 2));
  set_container_image(&battery_percent_image[2], battery_percent_layers[2], TINY_IMAGE_RESOURCE_IDS[10], GPoint(37, 2));
 
}

static void toggle_bluetooth_icon(bool connected) {
  if(appStarted && !connected && bluetoothvibe) {
    //vibe!
    vibes_long_pulse();
  }
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

void battery_layer_update_callback(Layer *me, GContext* ctx) {        
  //draw the remaining battery percentage
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(2, 2, ((batteryPercent/100.0)*11.0), 5), 0, GCornerNone);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {

  set_container_image(&gday_name_image, gday_name_layer, DAY_NAME_IMAGE_GERMAN_RESOURCE_IDS[tick_time->tm_wday], GPoint(66, 15));
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(66, 15));
		
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(66, 145));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(66, 155));
}

static void update_hours(struct tm *tick_time) {

  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
  
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(5, 14));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(33, 14));

  if (!clock_is_24h_style()) {
    if (tick_time->tm_hour >= 12) {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_PM_MODE, GPoint(60, 2));
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
    } 
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), true);
    }
    
    if (display_hour/10 == 0) {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}
static void update_minutes(struct tm *tick_time) {
  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(87, 14));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(115, 14));
}
static void update_seconds(struct tm *tick_time) {
  if(blink) {
    layer_set_hidden(bitmap_layer_get_layer(separator_layer), tick_time->tm_sec%2);
  }
  else {
    if(layer_get_hidden(bitmap_layer_get_layer(separator_layer))) {
      layer_set_hidden(bitmap_layer_get_layer(separator_layer), false);
    }
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
  }	
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
  memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
  memset(&battery_percent_image, 0, sizeof(battery_percent_image));

  const int inbound_size = 160;
  const int outbound_size = 160;
  app_message_open(inbound_size, outbound_size);  

  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
    background_color  = GColorBlack;
    window_set_background_color(window, background_color);

  window_stack_push(window, true /* Animated */);
  Layer *window_layer = window_get_root_layer(window);
		
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
  GRect frame = (GRect) {
    .origin = { .x = 66, .y = 137 },
    .size = separator_image->bounds.size
  };
  separator_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer));   

  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect frame3 = (GRect) {
    .origin = { .x = 130, .y = 2 },
    .size = bluetooth_image->bounds.size
  };
  bluetooth_layer = bitmap_layer_create(frame3);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
  
  battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect frame4 = (GRect) {
    .origin = { .x = 3, .y = 2 },
    .size = battery_image->bounds.size
  };
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_set_update_proc(bitmap_layer_get_layer(battery_layer), battery_layer_update_callback);
	
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
  
  GRect frame5 = (GRect) {
    .origin = { .x = 60, .y = 2 },
    .size = {.w = 19, .h = 8}
  };
  time_format_layer = bitmap_layer_create(frame5);
  if (clock_is_24h_style()) {
    time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_24_HOUR_MODE);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image);
    
  }
  layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));

  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
   day_name_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));	
   gday_name_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(gday_name_layer));	
	
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
  for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
  }

  toggle_bluetooth_icon(bluetooth_connection_service_peek());
  update_battery(battery_state_service_peek());

  Tuplet initial_values[] = {
    TupletInteger(BLINK_KEY, persist_read_bool(BLINK_KEY)),
    TupletInteger(INVERT_KEY, persist_read_bool(INVERT_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(GERMAN_KEY, persist_read_bool(GERMAN_KEY)),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  appStarted = true;
  
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  battery_state_service_subscribe(&update_battery);

}

static void deinit(void) {
  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
  
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
  bitmap_layer_destroy(battery_image_layer);

  layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);

  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(gday_name_layer));
  bitmap_layer_destroy(gday_name_layer);
  gbitmap_destroy(gday_name_image);
	
  for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    bitmap_layer_destroy(date_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_image[i]);
    bitmap_layer_destroy(battery_percent_layers[i]); 
  } 
	  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}