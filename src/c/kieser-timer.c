#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;

static bool running = false;
static time_t startedAt = 0;

static void vibe_times(int times) {
  if (times > 4) return; // not supported

  static const uint32_t const segments[] = { 300, 100, 300, 100, 300, 100, 300 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = times * 2 - 1,
  };

  vibes_enqueue_custom_pattern(pat);
}

static void timer_ticked(struct tm* tick_time, TimeUnits units_changed) {
  static char buf[6];

  time_t secondsSinceStarted = time(NULL) - startedAt;
  snprintf(buf, sizeof(buf), "%d", (int)secondsSinceStarted);
  text_layer_set_text(s_text_layer, buf);

  switch (secondsSinceStarted) {
    case 120:
      vibe_times(4);
      break;
    case 90:
      vibe_times(2);
      break;
    case 60:
      vibe_times(1);
      break;
  }
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  /*text_layer_set_text(s_text_layer, "Select");*/

  if (running) {
    tick_timer_service_unsubscribe();
  } else {
    time(&startedAt);
    tick_timer_service_subscribe(SECOND_UNIT, timer_ticked);
  }
  running = !running;
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 62, bounds.size.w, 50));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text(s_text_layer, "Start!");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
