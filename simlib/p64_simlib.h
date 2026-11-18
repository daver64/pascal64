#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p64_simlib_context p64_simlib_context;
typedef struct p64_simlib_event p64_simlib_event;
typedef struct p64_simlib_bitmap p64_simlib_bitmap;
typedef struct p64_simlib_sprite_batch p64_simlib_sprite_batch;
typedef struct p64_simlib_font p64_simlib_font;

p64_simlib_context* p64simlib_create(void);
void p64simlib_destroy(p64_simlib_context* ctx);

// Returns a short identifier for the active implementation.
// For now this is always "stub".
// NOTE: Pascal64 reserves external symbol names starting with "p64_".
// These APIs intentionally use the "p64simlib_" prefix instead.
const char* p64simlib_backend(void);
int p64simlib_hello(p64_simlib_context* ctx, const char* message);

// Display / main loop.
int p64simlib_set_gfx_mode(int driver, int width, int height);
void p64simlib_display_handle_event(p64_simlib_event* ev);
void p64simlib_set_window_title(const char* title);
void p64simlib_clear_to_colour(int red, int green, int blue, int alpha);
void p64simlib_show_video_bitmap(void);
void p64simlib_end_frame(void);
void p64simlib_display_shutdown(void);
void p64simlib_shutdown(void);
unsigned long long p64simlib_time_ms(void);
void p64simlib_rest(unsigned int milliseconds);
int p64simlib_screen_width(void);
int p64simlib_screen_height(void);

// Constants matching simlib's public API.
int p64simlib_gfx_autodetect_windowed(void);
int p64simlib_gfx_autodetect_fullscreen(void);

// Events.
p64_simlib_event* p64simlib_event_create(void);
void p64simlib_event_destroy(p64_simlib_event* ev);
int p64simlib_poll_event(p64_simlib_event* ev);
int p64simlib_event_type(p64_simlib_event* ev);
int p64simlib_event_key(p64_simlib_event* ev);
int p64simlib_event_window_width(p64_simlib_event* ev);
int p64simlib_event_window_height(p64_simlib_event* ev);
int p64simlib_event_mouse_button(p64_simlib_event* ev);
int p64simlib_event_mouse_wheel_x(p64_simlib_event* ev);
int p64simlib_event_mouse_wheel_y(p64_simlib_event* ev);
const char* p64simlib_event_text(p64_simlib_event* ev);

// Common event types (matches sl::Event::Type).
int p64simlib_event_type_quit(void);
int p64simlib_event_type_key_down(void);
int p64simlib_event_type_key_up(void);
int p64simlib_event_type_text_input(void);
int p64simlib_event_type_mouse_motion(void);
int p64simlib_event_type_mouse_button_down(void);
int p64simlib_event_type_mouse_button_up(void);
int p64simlib_event_type_mouse_wheel(void);
int p64simlib_event_type_window_resized(void);

// Input state.
int p64simlib_key_down_scancode(int scancode);
int p64simlib_mouse_x(void);
int p64simlib_mouse_y(void);
unsigned int p64simlib_mouse_buttons(void);
void p64simlib_set_mouse_mapping(int x, int y, double scale_x, double scale_y);
void p64simlib_clear_mouse_mapping(void);
int p64simlib_set_relative_mouse_mode(int enabled);
int p64simlib_relative_mouse_mode(void);
void p64simlib_mouse_delta(int* dx, int* dy);

// Simple maths helpers (kept in the simlib wrapper so Pascal programs don't
// need to link libm explicitly).
double p64simlib_sin(double x);
double p64simlib_cos(double x);

// Common SDL scancodes (stable identifiers for keyboard input).
int p64simlib_scancode_escape(void);
int p64simlib_scancode_space(void);
int p64simlib_scancode_return(void);
int p64simlib_scancode_backspace(void);
int p64simlib_scancode_tab(void);
int p64simlib_scancode_lshift(void);
int p64simlib_scancode_rshift(void);
int p64simlib_scancode_lctrl(void);
int p64simlib_scancode_lalt(void);
int p64simlib_scancode_left(void);
int p64simlib_scancode_right(void);
int p64simlib_scancode_up(void);
int p64simlib_scancode_down(void);

// 2D drawing helpers (draw to the window backbuffer).
void p64simlib_rectfill(int left, int top, int right, int bottom, int red, int green, int blue, int alpha);
void p64simlib_rect(int left, int top, int right, int bottom, int red, int green, int blue, int alpha, double thickness);
void p64simlib_line(int x1, int y1, int x2, int y2, int red, int green, int blue, int alpha);
void p64simlib_line_thick(int x1, int y1, int x2, int y2, int red, int green, int blue, int alpha, double thickness);
void p64simlib_circle(int x, int y, int radius, int red, int green, int blue, int alpha, double thickness);
void p64simlib_circlefill(int x, int y, int radius, int red, int green, int blue, int alpha);
void p64simlib_ellipse(int x, int y, int radius_x, int radius_y, int red, int green, int blue, int alpha, double thickness);
void p64simlib_ellipsefill(int x, int y, int radius_x, int radius_y, int red, int green, int blue, int alpha);
void p64simlib_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int red, int green, int blue, int alpha, double thickness);
void p64simlib_trianglefill(int x1, int y1, int x2, int y2, int x3, int y3, int red, int green, int blue, int alpha);

// Blend modes.
void p64simlib_set_blend_mode(int mode);
int p64simlib_blend_mode_normal(void);
int p64simlib_blend_mode_additive(void);
int p64simlib_blend_mode_multiply(void);
int p64simlib_blend_mode_screen(void);
int p64simlib_blend_mode_premultiplied(void);

// Clipping.
void p64simlib_set_clip_rect(int x, int y, int width, int height);
void p64simlib_clear_clip_rect(void);

// Bitmaps / textures.
p64_simlib_bitmap* p64simlib_create_bitmap(int width, int height);
p64_simlib_bitmap* p64simlib_create_render_target(int width, int height);
void p64simlib_destroy_bitmap(p64_simlib_bitmap* bitmap);
int p64simlib_bitmap_width(p64_simlib_bitmap* bitmap);
int p64simlib_bitmap_height(p64_simlib_bitmap* bitmap);
void p64simlib_bitmap_set_smooth(p64_simlib_bitmap* bitmap, int enabled);
void p64simlib_bitmap_set_mipmaps(p64_simlib_bitmap* bitmap, int enabled);
void p64simlib_bitmap_set_repeat(p64_simlib_bitmap* bitmap, int enabled);
void p64simlib_bitmap_set_anisotropy(p64_simlib_bitmap* bitmap, double anisotropy);
int p64simlib_acquire_bitmap(p64_simlib_bitmap* bitmap);
int p64simlib_release_bitmap(p64_simlib_bitmap* bitmap);
void p64simlib_clear_bitmap(p64_simlib_bitmap* bitmap, int red, int green, int blue, int alpha);
void p64simlib_putpixel(p64_simlib_bitmap* bitmap, int x, int y, int red, int green, int blue, int alpha);
void p64simlib_getpixel(p64_simlib_bitmap* bitmap, int x, int y, int* red, int* green, int* blue, int* alpha);
int p64simlib_upload_bitmap(p64_simlib_bitmap* bitmap);
int p64simlib_download_bitmap(p64_simlib_bitmap* bitmap);
p64_simlib_bitmap* p64simlib_load_bitmap(const char* path);
p64_simlib_bitmap* p64simlib_load_bitmap_from_memory(const unsigned char* data, unsigned long long size);
int p64simlib_save_bitmap(p64_simlib_bitmap* bitmap, const char* path);
void p64simlib_draw_sprite(p64_simlib_bitmap* bitmap, int x, int y);
void p64simlib_draw_sprite_stretched(p64_simlib_bitmap* bitmap, int x, int y, int width, int height);
void p64simlib_draw_sprite_rotated(p64_simlib_bitmap* bitmap, int center_x, int center_y, double angle_degrees);
void p64simlib_draw_sprite_rotated_stretched(p64_simlib_bitmap* bitmap, int center_x, int center_y, double angle_degrees, int width, int height);
void p64simlib_draw_sprite_region_flipped(
  p64_simlib_bitmap* bitmap,
  int source_x,
  int source_y,
  int source_width,
  int source_height,
  int x,
  int y,
  int destination_width,
  int destination_height,
  int flip_horizontal,
  int flip_vertical,
  int flip_diagonal,
  int tint_red,
  int tint_green,
  int tint_blue,
  int tint_alpha);

void p64simlib_draw_sprite_ex(
  p64_simlib_bitmap* bitmap,
  double x,
  double y,
  double origin_x,
  double origin_y,
  double scale_x,
  double scale_y,
  double rotation_degrees,
  int flip_x,
  int flip_y,
  int source_x,
  int source_y,
  int source_width,
  int source_height,
  int tint_red,
  int tint_green,
  int tint_blue,
  int tint_alpha);

// Sprite batches.
p64_simlib_sprite_batch* p64simlib_sprite_batch_create(void);
void p64simlib_sprite_batch_destroy(p64_simlib_sprite_batch* batch);
void p64simlib_sprite_batch_clear(p64_simlib_sprite_batch* batch);
void p64simlib_sprite_batch_add(
  p64_simlib_sprite_batch* batch,
  double x,
  double y,
  double scale_x,
  double scale_y,
  double rotation_degrees,
  int source_x,
  int source_y,
  int source_width,
  int source_height,
  int tint_red,
  int tint_green,
  int tint_blue,
  int tint_alpha);
void p64simlib_sprite_batch_draw(p64_simlib_bitmap* bitmap, p64_simlib_sprite_batch* batch);

// Render targets.
int p64simlib_begin_render_target(p64_simlib_bitmap* target);
void p64simlib_end_render_target(void);
void p64simlib_clear_render_target(int red, int green, int blue, int alpha);

// Text input.
void p64simlib_start_text_input(void);
void p64simlib_stop_text_input(void);

// Mouse button mask constants.
unsigned int p64simlib_mouse_button_left_mask(void);
unsigned int p64simlib_mouse_button_middle_mask(void);
unsigned int p64simlib_mouse_button_right_mask(void);

// Fonts / text.
p64_simlib_font* p64simlib_open_font(const char* path, int point_size);
p64_simlib_font* p64simlib_open_monospace_font(int point_size);
void p64simlib_close_font(p64_simlib_font* font);
int p64simlib_text_length(p64_simlib_font* font, const char* text);
int p64simlib_text_height(p64_simlib_font* font);
void p64simlib_textout(p64_simlib_font* font, int x, int y, int red, int green, int blue, int alpha, const char* text);

#ifdef __cplusplus
}
#endif

