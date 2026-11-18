program SimLibSpriteBatchDemo;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  tex: PSimLibBitmap;
  batch: PSimLibSpriteBatch;
  i: Integer;
  j: Integer;
  tick: Integer;
  w: Integer;
  h: Integer;
  x: Integer;
  y: Integer;
  r: Integer;
  g: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 900, 700) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;
  SimLib_SetWindowTitle('Pascal64 + simlib sprite batch demo');

  { Small procedural sprite }
  tex := SimLib_CreateBitmap(16, 16);
  for j := 0 to 15 do
    for i := 0 to 15 do
      SimLib_PutPixel(tex, i, j, 30 + i * 12, 40 + j * 10, 230, 255);
  SimLib_UploadBitmap(tex);

  batch := SimLib_SpriteBatchCreate();
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
    w := SimLib_ScreenWidth();
    h := SimLib_ScreenHeight();

    SimLib_SpriteBatchClear(batch);

    { 600 sprites in a deterministic pattern }
    for i := 0 to 599 do
    begin
      x := (i * 37 + tick * 3) mod w;
      y := (i * 19 + tick * 2) mod h;
      r := (50 + (i * 7)) mod 256;
      g := (80 + (i * 11)) mod 256;
      SimLib_SpriteBatchAdd(batch, x, y, 1.0, 1.0, 0.0, 0, 0, 0, 0, r, g, 240, 220);
    end;

    SimLib_ClearToColour(10, 12, 16, 255);
    SimLib_SpriteBatchDraw(tex, batch);
    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_SpriteBatchDestroy(batch);
  SimLib_DestroyBitmap(tex);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

