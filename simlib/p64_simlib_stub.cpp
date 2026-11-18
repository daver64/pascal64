#include "p64_simlib.h"

#include <cmath>
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

struct p64_simlib_sprite_batch {
  int reserved;
};

struct p64_simlib_font {
  int point_size;
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

int p64simlib_event_mouse_button(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_mouse_wheel_x(p64_simlib_event*) {
  return 0;
}

int p64simlib_event_mouse_wheel_y(p64_simlib_event*) {
  return 0;
}

const char* p64simlib_event_text(p64_simlib_event*) {
  return "";
}

int p64simlib_event_type_quit(void) { return 2; }
int p64simlib_event_type_key_down(void) { return 13; }
int p64simlib_event_type_key_up(void) { return 14; }
int p64simlib_event_type_text_input(void) { return 16; }
int p64simlib_event_type_mouse_motion(void) { return 19; }
int p64simlib_event_type_mouse_button_down(void) { return 20; }
int p64simlib_event_type_mouse_button_up(void) { return 21; }
int p64simlib_event_type_mouse_wheel(void) { return 22; }
int p64simlib_event_type_window_resized(void) { return 35; }

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

void p64simlib_set_mouse_mapping(int, int, double, double) {
}

void p64simlib_clear_mouse_mapping(void) {
}

int p64simlib_set_relative_mouse_mode(int) {
  return 0;
}

int p64simlib_relative_mouse_mode(void) {
  return 0;
}

void p64simlib_mouse_delta(int* dx, int* dy) {
  if (dx) *dx = 0;
  if (dy) *dy = 0;
}

void p64simlib_start_text_input(void) {
}

void p64simlib_stop_text_input(void) {
}

unsigned int p64simlib_mouse_button_left_mask(void) {
  return 1u << 0;
}

unsigned int p64simlib_mouse_button_middle_mask(void) {
  return 1u << 1;
}

unsigned int p64simlib_mouse_button_right_mask(void) {
  return 1u << 2;
}

double p64simlib_sin(double x) {
  return std::sin(x);
}

double p64simlib_cos(double x) {
  return std::cos(x);
}

int p64simlib_scancode_escape(void) {
  return 41;
}

int p64simlib_scancode_space(void) {
  return 44;
}

int p64simlib_scancode_return(void) {
  return 40;
}

int p64simlib_scancode_backspace(void) {
  return 42;
}

int p64simlib_scancode_tab(void) {
  return 43;
}

int p64simlib_scancode_lshift(void) {
  return 225;
}

int p64simlib_scancode_rshift(void) {
  return 229;
}

int p64simlib_scancode_lctrl(void) {
  return 224;
}

int p64simlib_scancode_lalt(void) {
  return 226;
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

void p64simlib_rect(int, int, int, int, int, int, int, int, double) {
}

void p64simlib_line(int, int, int, int, int, int, int, int) {
}

void p64simlib_line_thick(int, int, int, int, int, int, int, int, double) {
}

void p64simlib_circlefill(int, int, int, int, int, int, int) {
}

void p64simlib_circle(int, int, int, int, int, int, int, double) {
}

void p64simlib_ellipse(int, int, int, int, int, int, int, int, double) {
}

void p64simlib_ellipsefill(int, int, int, int, int, int, int, int) {
}

void p64simlib_triangle(int, int, int, int, int, int, int, int, int, int, double) {
}

void p64simlib_trianglefill(int, int, int, int, int, int, int, int, int, int) {
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

int p64simlib_bitmap_width(p64_simlib_bitmap*) {
  return 0;
}

int p64simlib_bitmap_height(p64_simlib_bitmap*) {
  return 0;
}

void p64simlib_bitmap_set_smooth(p64_simlib_bitmap*, int) {
}

void p64simlib_bitmap_set_mipmaps(p64_simlib_bitmap*, int) {
}

void p64simlib_bitmap_set_repeat(p64_simlib_bitmap*, int) {
}

void p64simlib_bitmap_set_anisotropy(p64_simlib_bitmap*, double) {
}

int p64simlib_acquire_bitmap(p64_simlib_bitmap*) {
  return 0;
}

int p64simlib_release_bitmap(p64_simlib_bitmap*) {
  return 0;
}

void p64simlib_clear_bitmap(p64_simlib_bitmap*, int, int, int, int) {
}

void p64simlib_putpixel(p64_simlib_bitmap*, int, int, int, int, int, int) {
}

void p64simlib_getpixel(p64_simlib_bitmap*, int, int, int* red, int* green, int* blue, int* alpha) {
  if (red) *red = 0;
  if (green) *green = 0;
  if (blue) *blue = 0;
  if (alpha) *alpha = 0;
}

int p64simlib_upload_bitmap(p64_simlib_bitmap*) {
  return 0;
}

int p64simlib_download_bitmap(p64_simlib_bitmap*) {
  return 0;
}

p64_simlib_bitmap* p64simlib_load_bitmap(const char*) {
  return nullptr;
}

p64_simlib_bitmap* p64simlib_load_bitmap_from_memory(const unsigned char*, unsigned long long) {
  return nullptr;
}

int p64simlib_save_bitmap(p64_simlib_bitmap*, const char*) {
  return 0;
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

p64_simlib_sprite_batch* p64simlib_sprite_batch_create(void) {
  return new p64_simlib_sprite_batch{0};
}

void p64simlib_sprite_batch_destroy(p64_simlib_sprite_batch* batch) {
  delete batch;
}

void p64simlib_sprite_batch_clear(p64_simlib_sprite_batch*) {
}

void p64simlib_sprite_batch_add(
  p64_simlib_sprite_batch*,
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
  int) {
}

void p64simlib_sprite_batch_draw(p64_simlib_bitmap*, p64_simlib_sprite_batch*) {
}

int p64simlib_begin_render_target(p64_simlib_bitmap*) {
  return 0;
}

void p64simlib_end_render_target(void) {
}

void p64simlib_clear_render_target(int, int, int, int) {
}

p64_simlib_font* p64simlib_open_font(const char*, int point_size) {
  return new p64_simlib_font{point_size};
}

p64_simlib_font* p64simlib_open_monospace_font(int point_size) {
  return new p64_simlib_font{point_size};
}

void p64simlib_close_font(p64_simlib_font* font) {
  delete font;
}

int p64simlib_text_length(p64_simlib_font*, const char* text) {
  if (!text) return 0;
  int len = 0;
  while (text[len] != '\0') len++;
  return len * 8;
}

int p64simlib_text_height(p64_simlib_font*) {
  return 16;
}

void p64simlib_textout(p64_simlib_font*, int, int, int, int, int, int, const char*) {
}
}

