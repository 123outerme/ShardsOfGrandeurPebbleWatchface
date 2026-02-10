#include <pebble.h>

#if defined(PBL_COLOR)
#define BGColor GColorLiberty
#define TxtColor GColorWhite
#define FullColor GColorFromHEX(0x00de00)
#define LowColor GColorFromHEX(0xef0000)
#define WarnColor GColorFromHEX(0xffb700)
#define HpBorderColor GColorFromHEX(0x2b2523)
#define ChargingColor GColorFromHEX(0x005500)
#else
#define BGColor GColorWhite
#define TxtColor GColorBlack
#define FullColor GColorBlack
#define WarnColor GColorBlack
#define LowColor GColorBlack
#define HpBorderColor GColorBlack
#define ChargingColor GColorBlack
#endif

// Pebble Time Round (1 only?) graphical elements' position offsets
#define ROUND_OFFSET_TIME_Y 30
#define ROUND_OFFSET_DATE_Y 68
#define ROUND_OFFSET_SPRITE_X 44
#define ROUND_OFFSET_SPRITE_Y 4
#define ROUND_OFFSET_BAR_X -18
#define ROUND_OFFSET_BAR_Y -104
#define ROUND_OFFSET_BT_X 36
#define ROUND_OFFSET_BT_Y -14

// common rectangular watches' time/date position offsets
#define RECT_OFFSET_TIME_Y 0
#define RECT_OFFSET_DATE_Y 44

// Pebble Time 2 (200 x 228 display size) position offsets
#define EMERY_OFFSET_SPRITE_X 12
#define EMERY_OFFSET_SPRITE_Y 0
#define EMERY_OFFSET_BAR_X 28
#define EMERY_OFFSET_BAR_Y -26
#define EMERY_OFFSET_BT_X EMERY_OFFSET_BAR_X
#define EMERY_OFFSET_BT_Y EMERY_OFFSET_BAR_Y

// Non-Time 2 rectangular watches' position offsets
#define RECT_OFFSET_SPRITE_X 0
#define RECT_OFFSET_SPRITE_Y 0
#define RECT_OFFSET_BAR_X 0
#define RECT_OFFSET_BAR_Y 0
#define RECT_OFFSET_BT_X RECT_OFFSET_BAR_X
#define RECT_OFFSET_BT_Y RECT_OFFSET_BAR_Y

static Window * s_main_window;           //main window
static TextLayer * s_time_layer;         //time layer
static TextLayer * s_date_layer;         //date layer
static GFont s_time_font;                //TI-84+ font in 40pt size
static GFont s_txt_font;                 //TI-84+ font in 24pt size
static Layer * s_canvas_layer;           //bg layer
static Layer * s_battery_layer;          //drawing layer for battery indicator
static BitmapLayer * s_background_layer; //character sprite layer
static GBitmap * s_background_bitmap;    //actual sprite
static BitmapLayer * s_bt_icon_layer;    //bluetooth icon layer
static GBitmap * s_bt_icon_bitmap;       //actual bluetooth bitmap
static int s_battery_level;              //battery %
GRect bound;

static int offsetSpriteX = 0;
static int offsetSpriteY = 0;
static int offsetBarX = 0;
static int offsetBarY = 0;
static int offsetBtX = 0;
static int offsetBtY = 0;
static int offsetTimeY = 0;
static int offsetDateY = 0;

static void canvas_update_proc();
static void battery_update_proc();
static void battery_callback();
static void bluetooth_callback(bool connected);
static void update_date();

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
	
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
	
  // Display the time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  update_date();
}
static void update_date() {
  //get a tm struct
  time_t temp = time(NULL);
  struct tm *tick_date = localtime(&temp);
  // Write the current date into a string
  static char date[] = "XXX YYY 88";
  strftime(date, sizeof(date), "%a %b %d", tick_date);
  text_layer_set_text(s_date_layer, date);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  
  GRect bounds = layer_get_bounds(window_get_root_layer(window));

  // Create canvas layer 
  s_canvas_layer = layer_create(bounds);
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  // Redraw this ASAP
  layer_mark_dirty(s_canvas_layer);
  
  bound = layer_get_unobstructed_bounds(window_layer);
  
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(2, offsetTimeY, bounds.size.w, 48));
  s_date_layer = text_layer_create(GRect(3, offsetDateY, bounds.size.w, 28));

  // Improve the layout to be more like a watchface
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RETRO_FONT_40));
  s_txt_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RETRO_FONT_18));
	
  text_layer_set_background_color(s_time_layer, BGColor);
  text_layer_set_text_color(s_time_layer, TxtColor);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  text_layer_set_background_color(s_date_layer, BGColor);
  text_layer_set_text_color(s_date_layer, TxtColor);
  text_layer_set_font(s_date_layer, s_txt_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_IMAGE);
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_DC_IMAGE);

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(GRect(offsetSpriteX, 104 + offsetSpriteY, bounds.size.w / 2, 64));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(bound.origin.x, bound.origin.y, bound.size.w, bound.size.h));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
	
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(90 + offsetBtX, bound.size.h * .725 + offsetBtY, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);

  // Add to Window
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  layer_add_child(window_layer, s_battery_layer);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
	
  // Show the correct state of the BT connection from the start
  //bluetooth_callback(false); // FOR TESTING BT CONNECTION LOST ICON ONLY
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
	
  // Destroy Battery
  layer_destroy(s_battery_layer);
  
  // Destroy Window
  window_destroy(s_main_window);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Custom drawing happens here!
  graphics_context_set_stroke_color(ctx, BGColor);
  graphics_context_set_fill_color(ctx, BGColor);
  graphics_fill_rect(ctx, GRect(bound.origin.x, bound.origin.y, bound.size.w, bound.size.h), 0, GCornersAll);
  // Update meter
  battery_callback(battery_state_service_peek());
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  int width = (s_battery_level * 64) / 100;  // Find the width of the bar (total width = 114px)
  if (s_battery_level > 65) {
      graphics_context_set_stroke_color(ctx, FullColor);
      graphics_context_set_fill_color(ctx, FullColor);
  } else if (s_battery_level > 35) {
      graphics_context_set_stroke_color(ctx, WarnColor);
      graphics_context_set_fill_color(ctx, WarnColor);
  } else {
      graphics_context_set_stroke_color(ctx, LowColor);
      graphics_context_set_fill_color(ctx, LowColor);
    }
	int magicNum = bound.size.h * .675 + offsetBarY;
	int startX = 75 + offsetBarX;
  if (s_battery_level != 0) 
    graphics_fill_rect(ctx, GRect(startX, magicNum, width, 6), 0, GCornersAll);
  else
    {
      graphics_draw_line(ctx, GPoint(startX, magicNum), GPoint(startX + 63, magicNum + 5));
      graphics_draw_line(ctx, GPoint(startX, magicNum + 5), GPoint(startX + 63, magicNum));
    }
  graphics_context_set_stroke_color(ctx, HpBorderColor);
  graphics_draw_rect(ctx, GRect(startX - 1, bound.size.h * .675 - 1 + offsetBarY, 66, 8));
  if (battery_state_service_peek().is_charging == true)
    {
      graphics_context_set_stroke_color(ctx, ChargingColor);
	  
      int xCoord[6] = {startX + 3, startX + 14, startX + 23, startX + 35, startX + 46, startX + 58};
      int yCoord[12] = {magicNum - 4, magicNum - 5, magicNum - 4, magicNum - 4, magicNum - 5, magicNum - 4, magicNum - 7, magicNum - 8, magicNum - 6, magicNum - 7, magicNum - 8, magicNum - 7};
      for(int i = 0; i < 6; i++)
        {
          for (int j = -2; j < 2; j++) {
        		graphics_draw_line(ctx, GPoint(xCoord[i] + j, yCoord[i]), GPoint(xCoord[i] + j, yCoord[i + 6]));
          }
	  	  }
    }
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected)
    if (!quiet_time_is_active()) {
      // Issue a vibrating alert
	    vibes_double_pulse();
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Make sure the time is displayed every minute
  update_time();
}

/*
static void date_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Get a tm structure
  update_date();
}
//*/

static void init() {
  PBL_IF_RECT_ELSE(offsetTimeY = RECT_OFFSET_TIME_Y, offsetTimeY = ROUND_OFFSET_TIME_Y);
  PBL_IF_RECT_ELSE(offsetDateY = RECT_OFFSET_DATE_Y, offsetDateY = ROUND_OFFSET_DATE_Y);
  
  if (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery) {
    offsetSpriteX = EMERY_OFFSET_SPRITE_X;
    offsetSpriteY = EMERY_OFFSET_SPRITE_Y;
    offsetBarX = EMERY_OFFSET_BAR_X;
    offsetBarY = EMERY_OFFSET_BAR_Y;
    offsetBtX = EMERY_OFFSET_BT_X;
    offsetBtY = EMERY_OFFSET_BT_Y;
  } else {
    PBL_IF_RECT_ELSE(offsetSpriteX = RECT_OFFSET_SPRITE_X, offsetSpriteX = ROUND_OFFSET_SPRITE_X);
    PBL_IF_RECT_ELSE(offsetSpriteY = RECT_OFFSET_SPRITE_Y, offsetSpriteY = ROUND_OFFSET_SPRITE_Y);
    PBL_IF_RECT_ELSE(offsetBarX = RECT_OFFSET_BAR_X, offsetBarX = ROUND_OFFSET_BAR_X);
    PBL_IF_RECT_ELSE(offsetBarY = RECT_OFFSET_BAR_Y, offsetBarY = ROUND_OFFSET_BAR_Y);
    PBL_IF_RECT_ELSE(offsetBtX = RECT_OFFSET_BT_X, offsetBtX = ROUND_OFFSET_BT_X);
    PBL_IF_RECT_ELSE(offsetBtY = RECT_OFFSET_BT_Y, offsetBtY = ROUND_OFFSET_BT_Y);
  }


  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
	
	// Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
  	.pebble_app_connection_handler = bluetooth_callback
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  // Make sure date is also displayed immediately
  update_date();
}

static void deinit() {

}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
