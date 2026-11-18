program SimLibSpriteExDemo;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  tex: PSimLibBitmap;
  i: Integer;
  j: Integer;
  tick: Integer;
  angle: Real;
  x: Integer;
  y: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 800, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;

  SimLib_SetWindowTitle('Pascal64 + simlib sprite_ex demo');

  tex := SimLib_CreateBitmap(64, 64);
  for j := 0 to 63 do
    for i := 0 to 63 do
      SimLib_PutPixel(tex, i, j, 60 + i * 3, 30 + j * 2, 220, 255);
  SimLib_UploadBitmap(tex);

  event := SimLib_EventCreate();
  running := 1;
  tick := 0;

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

    tick := tick + 1;
    angle := tick;
    x := SimLib_MouseX();
    y := SimLib_MouseY();

    SimLib_ClearToColour(10, 12, 16, 255);

    { Draw a rotated sprite with origin at the centre }
    SimLib_DrawSpriteEx(tex,
      x, y,
      0.5, 0.5,
      2.0, 2.0,
      angle,
      0, 0,
      0, 0, 0, 0,
      255, 255, 255, 220);

    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_DestroyBitmap(tex);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

