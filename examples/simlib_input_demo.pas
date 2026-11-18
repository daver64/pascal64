program SimLibInputDemo;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  x: Integer;
  y: Integer;
  speed: Integer;
  scLeft: Integer;
  scRight: Integer;
  scUp: Integer;
  scDown: Integer;
  scEsc: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 800, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;

  SimLib_SetWindowTitle('Pascal64 + simlib input demo');

  scLeft := SimLib_ScancodeLeft();
  scRight := SimLib_ScancodeRight();
  scUp := SimLib_ScancodeUp();
  scDown := SimLib_ScancodeDown();
  scEsc := SimLib_ScancodeEscape();

  x := 400;
  y := 300;
  speed := 5;

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

    if SimLib_KeyDownScancode(scEsc) <> 0 then
      running := 0;

    if SimLib_KeyDownScancode(scLeft) <> 0 then
      x := x - speed;
    if SimLib_KeyDownScancode(scRight) <> 0 then
      x := x + speed;
    if SimLib_KeyDownScancode(scUp) <> 0 then
      y := y - speed;
    if SimLib_KeyDownScancode(scDown) <> 0 then
      y := y + speed;

    { Clamp a bit so it's hard to lose the box }
    if x < 0 then x := 0;
    if y < 0 then y := 0;
    if x > SimLib_ScreenWidth() - 20 then x := SimLib_ScreenWidth() - 20;
    if y > SimLib_ScreenHeight() - 20 then y := SimLib_ScreenHeight() - 20;

    SimLib_ClearToColour(20, 20, 25, 255);
    SimLib_RectFill(x, y, x + 20, y + 20, 255, 80, 30, 255);
    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

