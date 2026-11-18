program VclHello;

{ Minimal VCL-like GTK3 demo.

  Build:
    ./build/p64 examples/vcl_hello.pas -o build/vcl_hello --unit-path units --clang clang-18 --link -lgtk-3 --link -lgobject-2.0

  Run:
    ./build/vcl_hello
}

uses P64Vcl;

var
  MainForm: TForm;
  Button1: TButton;
  Clicks: Integer;

procedure ButtonClicked(Sender: Pointer);
begin
  Clicks := Clicks + 1;
  WriteLn('Clicked ', Clicks, ' times');
end;

begin
  Application.Initialize;

  MainForm := TForm.Create('Pascal64 VCL-like demo', 320, 160);
  Button1 := TButton.Create(MainForm, 'Click me');
  Button1.OnClick := @ButtonClicked;

  MainForm.Show;
  Application.Run;

  Button1.Free;
  MainForm.Free;
end.

