program SimLibClear;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 800, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed (backend not available?)');
    Exit;
  end;

  SimLib_SetWindowTitle('Pascal64 + simlib');
  event := SimLib_EventCreate();
  running := 1;

  while running <> 0 do
  begin
    while SimLib_PollEvent(event) <> 0 do
    begin
      if SimLib_EventType(event) = SimLib_EventQuit() then
        running := 0;
      SimLib_DisplayHandleEvent(event);
    end;

    SimLib_ClearToColour(40, 60, 100, 255);
    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

