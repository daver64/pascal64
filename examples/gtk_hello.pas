program GtkHello;
uses Gtk3, GtkControls;
type
  DemoWindow = class
    window: GtkWindow;
    button: GtkButton;
    clicks: Integer;
    destructor Destroy;
    procedure show;
    procedure clicked;
  end;
var
  argc: Integer;
  argv: Pointer;
  app: DemoWindow;

procedure on_destroy(widget, data: Pointer);
begin
  gtk_main_quit;
end;

procedure on_clicked(widget, data: Pointer);
begin
  app.clicked;
end;

procedure DemoWindow.show;
begin
  Self.window := GtkWindow.Create('Hello from Pascal64', 320, 160);
  Self.button := GtkButton.Create('Click me');
  Self.window.add(Self.button);
  Self.button.on_click(@on_clicked, nil);
  Self.window.on_destroy(@on_destroy, nil);
  Self.window.show;
end;

destructor DemoWindow.Destroy;
begin
  Self.window.Free;
  Self.button.Free;
end;

procedure DemoWindow.clicked;
begin
  Self.clicks := Self.clicks + 1;
  WriteLn('Button clicked ', Self.clicks, ' times');
end;

begin
  argc := 0;
  argv := nil;
  if gtk_init_check(argc, argv) = 0 then
    WriteLn('GTK could not open a display')
  else
  begin
    app := DemoWindow.Create;
    app.show;
    gtk_main;
    app.Free;
  end;
end.
