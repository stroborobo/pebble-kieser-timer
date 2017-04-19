#include <pebble.h>

static Window *window;
static GRect window_bounds;

static StatusBarLayer *status_bar_layer;
static TextLayer *background_color_layer;
static TextLayer *animated_layer;
static TextLayer *counter_layer;

static bool running = false;
static time_t startedAt = 0;

static void kieser_vibe_times(int times) {
  if (times > 4) return; // not supported

  static const uint32_t const segments[] = { 300, 100, 300, 100, 300, 100, 300 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = times * 2 - 1,
  };

  vibes_enqueue_custom_pattern(pat);
}

static void kieser_set_background_colors(GColor animated, GColor background) {
  text_layer_set_background_color(animated_layer, animated);
  text_layer_set_background_color(background_color_layer, background);
}

static void kieser_set_default_colors() {
  kieser_set_background_colors(GColorPictonBlue, GColorGreen);
}

static void kieser_animate_frame(int seconds) {
  Layer *underlying_layer = text_layer_get_layer(animated_layer);
  layer_set_frame(underlying_layer, window_bounds);

  /*GRect end = GRect(window_bounds.size.w/2, window_bounds.size.h/2, 0, 0);*/
  GRect end = GRect(window_bounds.size.w, 0, 0, window_bounds.size.h);

  Animation *animation = property_animation_get_animation(
      property_animation_create_layer_frame(
        underlying_layer, NULL, &end));
  animation_set_duration(animation, seconds * 1000);
  animation_set_curve(animation, AnimationCurveLinear);
  animation_schedule(animation);
}

static void kieser_play_start() {
  kieser_set_default_colors();
  kieser_animate_frame(60);
}

static void kieser_play_60() {
  kieser_set_background_colors(GColorGreen, GColorYellow);
  kieser_vibe_times(1);
  kieser_animate_frame(30);
}

static void kieser_play_90() {
  kieser_set_background_colors(GColorYellow, GColorRed);
  kieser_vibe_times(2);
  kieser_animate_frame(30);
}

static void kieser_play_120() {
  kieser_set_background_colors(GColorRed, GColorPictonBlue);
  kieser_vibe_times(4);
  kieser_animate_frame(30);
}

static void kieser_timer_ticked(struct tm* tick_time, TimeUnits units_changed) {
  static char buf[4];

  time_t secondsSinceStarted = time(NULL) - startedAt;
  if (secondsSinceStarted > 300) {
    exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
    window_stack_pop_all(true);
    return;
  }

  snprintf(buf, sizeof(buf), "%d", (int)secondsSinceStarted);
  /*text_layer_set_font(counter_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));*/
  text_layer_set_text(counter_layer, buf);

  switch (secondsSinceStarted) {
    case 120:
      kieser_play_120();
      break;
    case 90:
      kieser_play_90();
      break;
    case 60:
      kieser_play_60();
      break;
  }
}

static void kieser_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (running) {
    animation_unschedule_all();
    tick_timer_service_unsubscribe();
  } else {
    kieser_play_start();
    time(&startedAt);
    tick_timer_service_subscribe(SECOND_UNIT, kieser_timer_ticked);
  }
  running = !running;
}

static void kieser_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, kieser_select_click_handler);
}

static void kieser_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  window_bounds = layer_get_bounds(window_layer);

  // Static Background
  background_color_layer = text_layer_create(window_bounds);
  layer_add_child(window_layer, text_layer_get_layer(background_color_layer));

  // Animated Background
  animated_layer = text_layer_create(window_bounds);
  layer_add_child(window_layer, text_layer_get_layer(animated_layer));

  // Text
  counter_layer = text_layer_create(
      GRect(0, (window_bounds.size.h+42)/2, window_bounds.size.w, 50));
  text_layer_set_font(counter_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_background_color(counter_layer, GColorClear);
  text_layer_set_text(counter_layer, "GO!");
  text_layer_set_text_alignment(counter_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(counter_layer));

  // StatusBar
  status_bar_layer = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(status_bar_layer));

  kieser_set_default_colors();
}

static void kieser_window_unload(Window *window) {
  text_layer_destroy(background_color_layer);
  text_layer_destroy(counter_layer);
  text_layer_destroy(animated_layer);
  status_bar_layer_destroy(status_bar_layer);
}

static void kieser_init(void) {
  window = window_create();
  window_set_click_config_provider(window, kieser_click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = kieser_window_load,
    .unload = kieser_window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void kieser_deinit(void) {
  window_destroy(window);
}

int main(void) {
  kieser_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  kieser_deinit();
}
