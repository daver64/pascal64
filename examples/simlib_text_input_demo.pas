program SimLibTextInputDemo;

{ Demonstrates text-input events and mouse wheel events.

  Notes:
  - Text input arrives as UTF-8 chunks via `text_input` events.
  - This demo intentionally keeps editing simple (click to clear).

  Build/run (with rsimlib enabled):
    ./build/p64 examples/simlib_text_input_demo.pas -o build/simlib_text_input_demo --clang clang --unit-path units --link build/simlib/libp64simlib.so --run
}

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  font: PSimLibFont;
  typed: String;
  wheel: Integer;
  et: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 900, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;
  SimLib_SetWindowTitle('Pascal64 + simlib text input demo');

  SimLib_StartTextInput();

  font := SimLib_OpenMonospaceFont(16);
  if font = nil then
    WriteLn('SimLib: failed to open monospace font (continuing)');

  event := SimLib_EventCreate();
  running := 1;
  typed := '';
  wheel := 0;

  while running <> 0 do
  begin
    while SimLib_PollEvent(event) <> 0 do
    begin
      et := SimLib_EventType(event);
      if et = SimLib_EventTypeQuit() then
        running := 0
      else if et = SimLib_EventTypeTextInput() then
        typed := typed + StrFromPChar(SimLib_EventText(event))
      else if et = SimLib_EventTypeMouseWheel() then
        wheel := wheel + SimLib_EventMouseWheelY(event)
      else if et = SimLib_EventTypeMouseButtonDown() then
        typed := '';
      SimLib_DisplayHandleEvent(event);
    end;

    if SimLib_KeyDownScancode(SimLib_ScancodeEscape()) <> 0 then
      running := 0;

    SimLib_ClearToColour(14, 16, 22, 255);

    if font <> nil then
    begin
      SimLib_TextOut(font, 20, 20, 255, 240, 170, 255, 'Text input demo');
      SimLib_TextOut(font, 20, 44, 200, 220, 255, 255, 'Type to append. Mouse wheel updates a counter. Click to clear.');
      SimLib_TextOut(font, 20, 68, 160, 255, 190, 255, 'Wheel: ' + IntToStr32(wheel));
      SimLib_TextOut(font, 20, 110, 255, 255, 255, 255, typed);
    end;

    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  if font <> nil then
    SimLib_CloseFont(font);
  SimLib_StopTextInput();
  SimLib_EventDestroy(event);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

