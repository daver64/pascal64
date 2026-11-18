unit Gtk3;
interface
type DestroyCallback = procedure(widget, data: Pointer); cdecl;
function gtk_init_check(var count: Integer; var arguments: Pointer): Integer; cdecl; external;
function gtk_window_new(window_type: Integer): Pointer; cdecl; external;
procedure gtk_window_set_title(window: Pointer; title: PChar); cdecl; external;
procedure gtk_window_set_default_size(window: Pointer; width, height: Integer); cdecl; external;
function gtk_button_new_with_label(label_text: PChar): Pointer; cdecl; external;
procedure gtk_container_add(container, widget: Pointer); cdecl; external;
procedure gtk_widget_show_all(widget: Pointer); cdecl; external;
procedure gtk_widget_destroy(widget: Pointer); cdecl; external;
function g_object_ref_sink(instance: Pointer): Pointer; cdecl; external;
procedure g_object_unref(instance: Pointer); cdecl; external;
procedure gtk_main; cdecl; external;
procedure gtk_main_quit; cdecl; external;
function g_signal_connect_data(instance: Pointer; signal_name: PChar; handler: DestroyCallback;
                               data, destroy_data: Pointer; flags: Integer): Int64; cdecl; external;
implementation
end.
