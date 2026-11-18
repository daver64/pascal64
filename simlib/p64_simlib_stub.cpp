#include "p64_simlib.h"

#include <cstdio>

struct p64_simlib_context {
  int reserved;
};

struct p64_simlib_event {
  int reserved;
};

struct p64_simlib_bitmap {
  int reserved;
};

extern "C" {
p64_simlib_context* p64simlib_create(void) {
  return new p64_simlib_context{0};
}

void p64simlib_destroy(p64_simlib_context* ctx) {
  delete ctx;
}

const char* p64simlib_backend(void) {
  return "stub";
}

int p64simlib_hello(p64_simlib_context*, const char* message) {
  std::fprintf(stdout, "p64simlib: hello %s\n", message ? message : "");
  return 0;
}

int p64simlib_set_gfx_mode(int, int, int) {
  return 0;
}

void p64simlib_display_handle_event(p64_simlib_event*) {
}

void p64simlib_set_window_title(const char*) {
}

void p64simlib_clear_to_colour(int, int, int, int) {
}

void p64simlib_show_video_bitmap(void) {
}

void p64simlib_end_frame(void) {
}

void p64simlib_display_shutdown(void) {
}

void p64simlib_shutdown(void) {
}

unsigned long long p64simlib_time_ms(void) {
  return 0ull;
}

void p64simlib_rest(unsigned int) {
}

int p64simlib_screen_width(void) {
  return 0;
}

int p64simlib_screen_height(void) {
  return 0;
}

int p64simlib_gfx_autodetect_windowed(void) {
  return 1;
}

int p64simlib_gfx_autodetect_fullscreen(void) {
  return 2;
}

p64_simlib_event* p64simlib_event_create(void) {
  return new p64_simlib_event{0};
}

void p64simlib_event_destroy(p64_simlib_event* ev) {
  delete ev;
}

int p64simlib_poll_event(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_type(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_key(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_window_width(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_window_height(p64_simlib_event*) {
  return 0;
}

int p64simlib_key_down_scancode(int) {
  return 0;
}

int p64simlib_mouse_x(void) {
  return 0;
}

int p64simlib_mouse_y(void) {
  return 0;
}

unsigned int p64simlib_mouse_buttons(void) {
  return 0u;
}

int p64simlib_scancode_escape(void) {
  return 41;
}

int p64simlib_scancode_left(void) {
  return 80;
}

int p64simlib_scancode_right(void) {
  return 79;
}

int p64simlib_scancode_up(void) {
  return 82;
}

int p64simlib_scancode_down(void) {
  return 81;
}

void p64simlib_rectfill(int, int, int, int, int, int, int, int) {
}

void p64simlib_line(int, int, int, int, int, int, int, int) {
}

void p64simlib_circlefill(int, int, int, int, int, int, int) {
}

void p64simlib_set_blend_mode(int) {
}

int p64simlib_blend_mode_normal(void) { return 0; }
int p64simlib_blend_mode_additive(void) { return 1; }
int p64simlib_blend_mode_multiply(void) { return 2; }
int p64simlib_blend_mode_screen(void) { return 3; }
int p64simlib_blend_mode_premultiplied(void) { return 4; }

void p64simlib_set_clip_rect(int, int, int, int) {
}

void p64simlib_clear_clip_rect(void) {
}

p64_simlib_bitmap* p64simlib_create_bitmap(int, int) {
  return nullptr;
}

p64_simlib_bitmap* p64simlib_create_render_target(int, int) {
  return nullptr;
}

void p64simlib_destroy_bitmap(p64_simlib_bitmap*) {
}

void p64simlib_putpixel(p64_simlib_bitmap*, int, int, int, int, int, int) {
}

int p64simlib_upload_bitmap(p64_simlib_bitmap*) {
  return 0;
}

p64_simlib_bitmap* p64simlib_load_bitmap(const char*) {
  return nullptr;
}

void p64simlib_draw_sprite(p64_simlib_bitmap*, int, int) {
}

void p64simlib_draw_sprite_stretched(p64_simlib_bitmap*, int, int, int, int) {
}

void p64simlib_draw_sprite_rotated(p64_simlib_bitmap*, int, int, double) {
}

void p64simlib_draw_sprite_rotated_stretched(p64_simlib_bitmap*, int, int, double, int, int) {
}

void p64simlib_draw_sprite_region_flipped(
  p64_simlib_bitmap*,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int) {
}

void p64simlib_draw_sprite_ex(
  p64_simlib_bitmap*,
  double,
  double,
  double,
  double,
  double,
  double,
  double,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int,
  int) {
}

int p64simlib_begin_render_target(p64_simlib_bitmap*) {
  return 0;
}

void p64simlib_end_render_target(void) {
}

void p64simlib_clear_render_target(int, int, int, int) {
}
}

