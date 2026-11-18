#include "p64_simlib.h"

#include <cstdio>

// rsimlib public API
#include <sl.h>

#include <SDL2/SDL_scancode.h>

struct p64_simlib_context {
  int reserved;
};

struct p64_simlib_event {
  sl::Event event;
};

struct p64_simlib_bitmap {
  sl::Bitmap* bitmap;
};

extern "C" {
p64_simlib_context* p64simlib_create(void) {
  return new p64_simlib_context{0};
}

void p64simlib_destroy(p64_simlib_context* ctx) {
  delete ctx;
}

const char* p64simlib_backend(void) {
  return "rsimlib";
}

int p64simlib_hello(p64_simlib_context*, const char* message) {
  std::fprintf(stdout, "p64simlib(rs): hello %s\n", message ? message : "");
  return 0;
}

int p64simlib_set_gfx_mode(int driver, int width, int height) {
  return sl::set_gfx_mode(driver, width, height) ? 1 : 0;
}

void p64simlib_display_handle_event(p64_simlib_event* ev) {
  if (!ev) return;
  sl::display_handle_event(ev->event);
}

void p64simlib_set_window_title(const char* title) {
  sl::set_window_title(title ? title : "");
}

void p64simlib_clear_to_colour(int red, int green, int blue, int alpha) {
  sl::clear_to_colour(static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha));
}

void p64simlib_show_video_bitmap(void) {
  sl::show_video_bitmap();
}

void p64simlib_end_frame(void) {
  sl::end_frame();
}

void p64simlib_display_shutdown(void) {
  sl::display_shutdown();
}

void p64simlib_shutdown(void) {
  sl::shutdown();
}

unsigned long long p64simlib_time_ms(void) {
  return static_cast<unsigned long long>(sl::time_ms());
}

void p64simlib_rest(unsigned int milliseconds) {
  sl::rest(milliseconds);
}

int p64simlib_screen_width(void) {
  return sl::screen_width();
}

int p64simlib_screen_height(void) {
  return sl::screen_height();
}

int p64simlib_gfx_autodetect_windowed(void) {
  return sl::GFX_AUTODETECT_WINDOWED;
}

int p64simlib_gfx_autodetect_fullscreen(void) {
  return sl::GFX_AUTODETECT_FULLSCREEN;
}

p64_simlib_event* p64simlib_event_create(void) {
  return new p64_simlib_event{};
}

void p64simlib_event_destroy(p64_simlib_event* ev) {
  delete ev;
}

int p64simlib_poll_event(p64_simlib_event* ev) {
  if (!ev) return 0;
  return sl::poll_event(&ev->event) ? 1 : 0;
}

int p64simlib_event_type(p64_simlib_event* ev) {
  if (!ev) return 0;
  return static_cast<int>(ev->event.type());
}

int p64simlib_event_key(p64_simlib_event* ev) {
  if (!ev) return 0;
  return static_cast<int>(ev->event.key());
}

int p64simlib_event_window_width(p64_simlib_event* ev) {
  if (!ev) return 0;
  return ev->event.window_width();
}

int p64simlib_event_window_height(p64_simlib_event* ev) {
  if (!ev) return 0;
  return ev->event.window_height();
}

int p64simlib_key_down_scancode(int scancode) {
  return sl::key_down(static_cast<SDL_Scancode>(scancode)) ? 1 : 0;
}

int p64simlib_mouse_x(void) {
  return sl::mouse_x();
}

int p64simlib_mouse_y(void) {
  return sl::mouse_y();
}

unsigned int p64simlib_mouse_buttons(void) {
  return static_cast<unsigned int>(sl::mouse_buttons());
}

int p64simlib_scancode_escape(void) {
  return SDL_SCANCODE_ESCAPE;
}

int p64simlib_scancode_left(void) {
  return SDL_SCANCODE_LEFT;
}

int p64simlib_scancode_right(void) {
  return SDL_SCANCODE_RIGHT;
}

int p64simlib_scancode_up(void) {
  return SDL_SCANCODE_UP;
}

int p64simlib_scancode_down(void) {
  return SDL_SCANCODE_DOWN;
}

void p64simlib_rectfill(int left, int top, int right, int bottom, int red, int green, int blue, int alpha) {
  sl::rectfill(sl::screen,
    static_cast<float>(left),
    static_cast<float>(top),
    static_cast<float>(right),
    static_cast<float>(bottom),
    sl::Colour{static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha)});
}

void p64simlib_line(int x1, int y1, int x2, int y2, int red, int green, int blue, int alpha) {
  sl::line(sl::screen,
    static_cast<float>(x1),
    static_cast<float>(y1),
    static_cast<float>(x2),
    static_cast<float>(y2),
    sl::Colour{static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha)});
}

void p64simlib_circlefill(int x, int y, int radius, int red, int green, int blue, int alpha) {
  sl::circlefill(sl::screen,
    static_cast<float>(x),
    static_cast<float>(y),
    static_cast<float>(radius),
    sl::Colour{static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha)});
}

void p64simlib_set_blend_mode(int mode) {
  sl::BlendMode blend = sl::BlendMode::normal;
  switch (mode) {
    case 0: blend = sl::BlendMode::normal; break;
    case 1: blend = sl::BlendMode::additive; break;
    case 2: blend = sl::BlendMode::multiply; break;
    case 3: blend = sl::BlendMode::screen; break;
    case 4: blend = sl::BlendMode::premultiplied; break;
    default: blend = sl::BlendMode::normal; break;
  }
  sl::set_blend_mode(blend);
}

int p64simlib_blend_mode_normal(void) { return 0; }
int p64simlib_blend_mode_additive(void) { return 1; }
int p64simlib_blend_mode_multiply(void) { return 2; }
int p64simlib_blend_mode_screen(void) { return 3; }
int p64simlib_blend_mode_premultiplied(void) { return 4; }

void p64simlib_set_clip_rect(int x, int y, int width, int height) {
  sl::set_clip_rect(x, y, width, height);
}

void p64simlib_clear_clip_rect(void) {
  sl::clear_clip_rect();
}

p64_simlib_bitmap* p64simlib_create_bitmap(int width, int height) {
  sl::Bitmap* bitmap = sl::create_bitmap(width, height);
  if (!bitmap) return nullptr;
  return new p64_simlib_bitmap{bitmap};
}

p64_simlib_bitmap* p64simlib_create_render_target(int width, int height) {
  sl::Bitmap* bitmap = sl::create_render_target(width, height);
  if (!bitmap) return nullptr;
  return new p64_simlib_bitmap{bitmap};
}

void p64simlib_destroy_bitmap(p64_simlib_bitmap* bitmap) {
  if (!bitmap) return;
  if (bitmap->bitmap) sl::destroy_bitmap(bitmap->bitmap);
  delete bitmap;
}

void p64simlib_putpixel(p64_simlib_bitmap* bitmap, int x, int y, int red, int green, int blue, int alpha) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::putpixel(bitmap->bitmap,
    x,
    y,
    sl::Colour{static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha)});
}

int p64simlib_upload_bitmap(p64_simlib_bitmap* bitmap) {
  if (!bitmap || !bitmap->bitmap) return 0;
  return sl::upload_bitmap(bitmap->bitmap) ? 1 : 0;
}

p64_simlib_bitmap* p64simlib_load_bitmap(const char* path) {
  if (!path) return nullptr;
  sl::Bitmap* bitmap = sl::load_bitmap(std::string(path));
  if (!bitmap) return nullptr;
  return new p64_simlib_bitmap{bitmap};
}

void p64simlib_draw_sprite(p64_simlib_bitmap* bitmap, int x, int y) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::draw_sprite(bitmap->bitmap, static_cast<float>(x), static_cast<float>(y));
}

void p64simlib_draw_sprite_stretched(p64_simlib_bitmap* bitmap, int x, int y, int width, int height) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::draw_sprite_stretched(bitmap->bitmap, static_cast<float>(x), static_cast<float>(y), width, height);
}

void p64simlib_draw_sprite_rotated(p64_simlib_bitmap* bitmap, int center_x, int center_y, double angle_degrees) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::draw_sprite_rotated(bitmap->bitmap,
    static_cast<float>(center_x),
    static_cast<float>(center_y),
    static_cast<float>(angle_degrees));
}

void p64simlib_draw_sprite_rotated_stretched(p64_simlib_bitmap* bitmap, int center_x, int center_y, double angle_degrees, int width, int height) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::draw_sprite_rotated_stretched(bitmap->bitmap,
    static_cast<float>(center_x),
    static_cast<float>(center_y),
    static_cast<float>(angle_degrees),
    width,
    height);
}

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
  int tint_alpha) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::draw_sprite_region_flipped(
    bitmap->bitmap,
    source_x,
    source_y,
    source_width,
    source_height,
    static_cast<float>(x),
    static_cast<float>(y),
    static_cast<float>(destination_width),
    static_cast<float>(destination_height),
    flip_horizontal != 0,
    flip_vertical != 0,
    flip_diagonal != 0,
    sl::Colour{static_cast<Uint8>(tint_red), static_cast<Uint8>(tint_green), static_cast<Uint8>(tint_blue), static_cast<Uint8>(tint_alpha)});
}

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
  int tint_alpha) {
  if (!bitmap || !bitmap->bitmap) return;
  sl::SpriteParams params;
  params.x = static_cast<float>(x);
  params.y = static_cast<float>(y);
  params.origin_x = static_cast<float>(origin_x);
  params.origin_y = static_cast<float>(origin_y);
  params.scale_x = static_cast<float>(scale_x);
  params.scale_y = static_cast<float>(scale_y);
  params.rotation = static_cast<float>(rotation_degrees);
  params.flip_x = flip_x != 0;
  params.flip_y = flip_y != 0;
  params.source_x = source_x;
  params.source_y = source_y;
  params.source_width = source_width;
  params.source_height = source_height;
  params.tint = sl::Colour{static_cast<Uint8>(tint_red), static_cast<Uint8>(tint_green), static_cast<Uint8>(tint_blue), static_cast<Uint8>(tint_alpha)};
  sl::draw_sprite_ex(bitmap->bitmap, params);
}

int p64simlib_begin_render_target(p64_simlib_bitmap* target) {
  if (!target || !target->bitmap) return 0;
  return sl::begin_render_target(target->bitmap) ? 1 : 0;
}

void p64simlib_end_render_target(void) {
  sl::end_render_target();
}

void p64simlib_clear_render_target(int red, int green, int blue, int alpha) {
  sl::clear_render_target(sl::Colour{static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), static_cast<Uint8>(alpha)});
}
}

