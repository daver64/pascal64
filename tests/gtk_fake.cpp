#include <cstdlib>
#include <cstdint>

namespace {
using Callback = void (*)(void*, void*);
struct Widget {
  int references = 1;
  bool floating = true;
  bool destroyed = false;
  Widget* child = nullptr;
  Callback callback = nullptr;
  void* data = nullptr;
};
Widget* window = nullptr;
Widget* button = nullptr;
int objects = 0;
bool quit = false;

void require(bool condition) {
  if (!condition) std::abort();
}

Widget* create() {
  ++objects;
  return new Widget;
}
void release(Widget* widget) {
  require(widget && widget->references > 0);
  if (--widget->references == 0) {
    --objects;
    delete widget;
  }
}
}

extern "C" int gtk_init_check(int*, void**) { return 1; }
extern "C" void* gtk_window_new(int) { return window = create(); }
extern "C" void* gtk_button_new_with_label(const char*) { return button = create(); }
extern "C" void gtk_window_set_title(void*, const char*) {}
extern "C" void gtk_window_set_default_size(void*, int, int) {}
extern "C" void* g_object_ref_sink(void* object) {
  auto* widget = static_cast<Widget*>(object);
  if (widget->floating) widget->floating = false;
  else ++widget->references;
  return widget;
}
extern "C" void g_object_unref(void* object) { release(static_cast<Widget*>(object)); }
extern "C" void gtk_container_add(void* container, void* child) {
  auto* parent = static_cast<Widget*>(container);
  auto* widget = static_cast<Widget*>(child);
  require(!parent->child && !widget->floating);
  parent->child = widget;
  ++widget->references;
}
extern "C" std::int64_t g_signal_connect_data(void* object, const char*, Callback callback, void* data, void*, int) {
  auto* widget = static_cast<Widget*>(object);
  widget->callback = callback;
  widget->data = data;
  return 1;
}
extern "C" void gtk_widget_show_all(void*) {}
extern "C" void gtk_widget_destroy(void* object) {
  auto* widget = static_cast<Widget*>(object);
  if (widget->destroyed) return;
  widget->destroyed = true;
  if (widget->child) {
    gtk_widget_destroy(widget->child);
    release(widget->child);
    widget->child = nullptr;
  }
  if (widget->callback && widget == window) widget->callback(widget, widget->data);
}
extern "C" void gtk_main_quit() { quit = true; }
extern "C" void gtk_main() {
  require(button && button->callback && window);
  button->callback(button, button->data);
  button->callback(button, button->data);
  gtk_widget_destroy(window);
  require(quit);
}

struct LeakCheck {
  ~LeakCheck() { require(objects == 0); }
} leakCheck;
