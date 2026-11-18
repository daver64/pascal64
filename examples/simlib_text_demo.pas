program SimLibTextDemo;

{ Demonstrates basic font and text rendering via `P64SimLib`.

  Build/run (with rsimlib enabled):
    ./build/p64 examples/simlib_text_demo.pas -o build/simlib_text_demo --clang clang --unit-path units --link build/simlib/libp64simlib.so --run
}

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  font: PSimLibFont;
  lastMs: Int64;
  frames: Integer;
  fps: Int64;
  nowMs: Int64;
  dtMs: Int64;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 900, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;
  SimLib_SetWindowTitle('Pascal64 + simlib text demo');

  font := SimLib_OpenMonospaceFont(16);
  if font = nil then
    WriteLn('SimLib: failed to open monospace font (continuing)');

  event := SimLib_EventCreate();
  running := 1;
  lastMs := SimLib_TimeMs();
  frames := 0;
  fps := 0;

  while running <> 0 do
  begin
    while SimLib_PollEvent(event) <> 0 do
    begin
      if SimLib_EventType(event) = SimLib_EventTypeQuit() then
        running := 0;
      SimLib_DisplayHandleEvent(event);
    end;
    if SimLib_KeyDownScancode(SimLib_ScancodeEscape()) <> 0 then
      running := 0;

    SimLib_ClearToColour(14, 16, 22, 255);

    frames := frames + 1;
    nowMs := SimLib_TimeMs();
    dtMs := nowMs - lastMs;
    if dtMs >= 500 then
    begin
      if dtMs > 0 then
        fps := (frames * 1000) div dtMs
      else
        fps := 0;
      frames := 0;
      lastMs := nowMs;
    end;

    if font <> nil then
    begin
      SimLib_TextOut(font, 20, 20, 255, 240, 170, 255, 'SimLib text demo');
      SimLib_TextOut(font, 20, 44, 200, 220, 255, 255, 'Move the mouse; press Escape to quit');
      SimLib_TextOut(font, 20, 68, 160, 190, 255, 255, 'Mouse: ' + IntToStr32(SimLib_MouseX()) + ', ' + IntToStr32(SimLib_MouseY()));
      SimLib_TextOut(font, 20, 92, 160, 255, 190, 255, 'FPS: ' + IntToStr(fps));
    end;

    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  if font <> nil then
    SimLib_CloseFont(font);
  SimLib_EventDestroy(event);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

