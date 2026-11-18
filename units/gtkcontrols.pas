unit GtkControls;
interface
uses Gtk3;
type
  GtkButton = class
    handle: Pointer;
    constructor Create(label_text: PChar);
    destructor Destroy;
    procedure on_click(handler: DestroyCallback; data: Pointer);
  end;
  GtkWindow = class
    handle: Pointer;
    constructor Create(title: PChar; width, height: Integer);
    destructor Destroy;
    procedure add(button: GtkButton);
    procedure on_destroy(handler: DestroyCallback; data: Pointer);
    procedure show;
  end;
implementation

constructor GtkButton.Create(label_text: PChar);
begin
  Self.handle := g_object_ref_sink(gtk_button_new_with_label(label_text));
end;

destructor GtkButton.Destroy;
begin
  gtk_widget_destroy(Self.handle);
  g_object_unref(Self.handle);
end;

procedure GtkButton.on_click(handler: DestroyCallback; data: Pointer);
var connection: Int64;
begin
  connection := g_signal_connect_data(Self.handle, 'clicked', handler, data, nil, 0);
end;

constructor GtkWindow.Create(title: PChar; width, height: Integer);
begin
  Self.handle := g_object_ref_sink(gtk_window_new(0));
  gtk_window_set_title(Self.handle, title);
  gtk_window_set_default_size(Self.handle, width, height);
end;

destructor GtkWindow.Destroy;
begin
  gtk_widget_destroy(Self.handle);
  g_object_unref(Self.handle);
end;

procedure GtkWindow.add(button: GtkButton);
begin
  gtk_container_add(Self.handle, button.handle);
end;

procedure GtkWindow.on_destroy(handler: DestroyCallback; data: Pointer);
var connection: Int64;
begin
  connection := g_signal_connect_data(Self.handle, 'destroy', handler, data, nil, 0);
end;

procedure GtkWindow.show;
begin
  gtk_widget_show_all(Self.handle);
end;
end.
