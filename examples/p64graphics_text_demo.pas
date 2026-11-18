program P64GraphicsTextDemo;

{ Demonstrates text rendering via the higher-level `P64Graphics` wrappers.

  Build/run (with rsimlib enabled):
    ./build/p64 examples/p64graphics_text_demo.pas -o build/p64graphics_text_demo --clang clang --unit-path units --link build/simlib/libp64simlib.so --run
}

uses P64Std, P64SimLib, P64Graphics;

var
  App: TApplication;
  Font: TFont;
  LastMs: Int64;
  Frames: Integer;
  Fps: Int64;

procedure Frame;
var
  nowMs: Int64;
  dtMs: Int64;
  s: String;
begin
  nowMs := SimLib_TimeMs();
  dtMs := nowMs - LastMs;
  Frames := Frames + 1;
  if dtMs >= 500 then
  begin
    if dtMs > 0 then
      Fps := (Frames * 1000) div dtMs
    else
      Fps := 0;
    Frames := 0;
    LastMs := nowMs;
  end;

  Clear(ColourRGB(14, 16, 22));

  Font.DrawText(20, 20, ColourRGBA(255, 240, 170, 255), 'P64Graphics text demo');
  Font.DrawText(20, 44, ColourRGBA(200, 220, 255, 255), 'Move the mouse; press Escape to quit');
  s := 'Mouse: ' + IntToStr32(SimLib_MouseX()) + ', ' + IntToStr32(SimLib_MouseY());
  Font.DrawText(20, 68, ColourRGBA(160, 190, 255, 255), s);
  Font.DrawText(20, 92, ColourRGBA(160, 255, 190, 255), 'FPS: ' + IntToStr(Fps));

  { A small visual marker at the mouse position. }
  SimLib_CircleFill(SimLib_MouseX(), SimLib_MouseY(), 8, 255, 80, 30, 220);
end;

begin
  InitWindow(900, 600, 'Pascal64 + simlib (P64Graphics text demo)');

  App := TApplication.Create;
  Font := OpenMonospaceFont(16);

  LastMs := SimLib_TimeMs();
  Frames := 0;
  Fps := 0;

  App.Run(@Frame);

  Font.Free;
  App.Free;
  ShutdownWindow;
end.

