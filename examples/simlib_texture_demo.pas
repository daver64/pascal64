program SimLibTextureDemo;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  tex: PSimLibBitmap;
  x: Integer;
  y: Integer;
  i: Integer;
  j: Integer;
  c: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 800, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;

  SimLib_SetWindowTitle('Pascal64 + simlib texture demo');

  tex := SimLib_CreateBitmap(64, 64);
  if tex = nil then
  begin
    WriteLn('SimLib: CreateBitmap failed');
    Exit;
  end;

  { Generate a simple checker / gradient texture }
  for j := 0 to 63 do
  begin
    for i := 0 to 63 do
    begin
      c := ((i div 8) + (j div 8)) mod 2;
      if c = 0 then
        SimLib_PutPixel(tex, i, j, 220, 220, 255, 255)
      else
        SimLib_PutPixel(tex, i, j, 80 + i * 2, 40 + j * 2, 60, 255);
    end;
  end;
  if SimLib_UploadBitmap(tex) = 0 then
    WriteLn('SimLib: UploadBitmap failed (continuing anyway)');

  event := SimLib_EventCreate();
  running := 1;
  x := 100;
  y := 100;
  while running <> 0 do
  begin
    while SimLib_PollEvent(event) <> 0 do
    begin
      if SimLib_EventType(event) = SimLib_EventQuit() then
        running := 0;
      SimLib_DisplayHandleEvent(event);
    end;

    if SimLib_KeyDownScancode(SimLib_ScancodeEscape()) <> 0 then
      running := 0;

    { Follow the mouse a bit }
    x := SimLib_MouseX() - 64;
    y := SimLib_MouseY() - 64;

    SimLib_ClearToColour(15, 18, 22, 255);

    SimLib_DrawSpriteStretched(tex, x, y, 128, 128);
    SimLib_Line(0, 0, SimLib_ScreenWidth(), SimLib_ScreenHeight(), 0, 200, 120, 255);
    SimLib_CircleFill(SimLib_ScreenWidth() div 2, SimLib_ScreenHeight() div 2, 30, 255, 80, 30, 200);

    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_DestroyBitmap(tex);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

