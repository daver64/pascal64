unit P64Vcl;

{ Tiny Delphi/VCL-like wrapper layer over GTK3.

  Notes:
  - Events are currently implemented using `cdecl` callback variables.
  - Handlers must be program- or unit-scope routines.
  - The event signature uses `Pointer` for `Sender` for now.
}

interface

uses Gtk3, GtkControls;

type
  TNotifyEvent = procedure(Sender: Pointer); cdecl;

  TApplication = class
    procedure Initialize;
    procedure Run;
    procedure Terminate;
  end;

  TForm = class
    Window: GtkWindow;
    OnClose: TNotifyEvent;
    constructor Create(title: PChar; width, height: Integer);
    destructor Destroy;
    procedure Show;
    procedure DoClose;
  end;

  TButton = class
    Button: GtkButton;
    OnClick: TNotifyEvent;
    constructor Create(parent: TForm; caption: PChar);
    destructor Destroy;
    procedure DoClick;
  end;

var
  Application: TApplication;

implementation

var
  appArgc: Integer;
  appArgv: Pointer;

procedure p64vcl_on_destroy(widget, data: Pointer);
var
  form: TForm;
begin
  form := data;
  if form <> nil then
    form.DoClose
  else
    gtk_main_quit;
end;

procedure p64vcl_on_clicked(widget, data: Pointer);
var
  button: TButton;
begin
  button := data;
  if button <> nil then
    button.DoClick;
end;

procedure TApplication.Initialize;
begin
  appArgc := 0;
  appArgv := nil;
  gtk_init_check(appArgc, appArgv);
end;

procedure TApplication.Run;
begin
  gtk_main;
end;

procedure TApplication.Terminate;
begin
  gtk_main_quit;
end;

constructor TForm.Create(title: PChar; width, height: Integer);
begin
  Self.Window := GtkWindow.Create(title, width, height);
  Self.Window.on_destroy(@p64vcl_on_destroy, Self);
end;

destructor TForm.Destroy;
begin
  Self.Window.Free;
end;

procedure TForm.Show;
begin
  Self.Window.show;
end;

procedure TForm.DoClose;
begin
  if Self.OnClose <> nil then
    Self.OnClose(Self)
  else
    Application.Terminate;
end;

constructor TButton.Create(parent: TForm; caption: PChar);
begin
  Self.Button := GtkButton.Create(caption);
  parent.Window.add(Self.Button);
  Self.Button.on_click(@p64vcl_on_clicked, Self);
end;

destructor TButton.Destroy;
begin
  Self.Button.Free;
end;

procedure TButton.DoClick;
begin
  if Self.OnClick <> nil then
    Self.OnClick(Self);
end;

initialization
  Application := TApplication.Create;

finalization
  Application.Free;

end.

