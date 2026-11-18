program P64GraphicsDemo;

uses P64Graphics, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  tex: TBitmap;
  batch: TSpriteBatch;
  i: Integer;
  j: Integer;
  tick: Integer;
  w: Integer;
  h: Integer;
  x: Integer;
  y: Integer;
begin
  InitWindow(900, 700, 'Pascal64 P64Graphics demo');

  tex := TBitmap.Create(16, 16);
  for j := 0 to 15 do
    for i := 0 to 15 do
      tex.PutPixel(i, j, ColourRGBA(30 + i * 12, 40 + j * 10, 230, 255));
  tex.Upload();

  batch := TSpriteBatch.Create();
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
    w := ScreenWidth();
    h := ScreenHeight();

    batch.Clear();
    for i := 0 to 599 do
    begin
      x := (i * 37 + tick * 3) mod w;
      y := (i * 19 + tick * 2) mod h;
      batch.Add(x, y, 1.0, 1.0, 0.0, 0, 0, 0, 0, ColourRGBA((50 + (i * 7)) mod 256, (80 + (i * 11)) mod 256, 240, 220));
    end;

    Clear(ColourRGBA(10, 12, 16, 255));
    batch.Draw(tex);
    Present();
    EndFrame();
  end;

  SimLib_EventDestroy(event);
  batch.Free();
  tex.Free();
  ShutdownWindow();
end.

