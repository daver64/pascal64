program SimLibRenderTargetDemo;

uses P64Std, P64SimLib;

var
  event: PSimLibEvent;
  running: Integer;
  tex: PSimLibBitmap;
  rt: PSimLibBitmap;
  i: Integer;
  j: Integer;
  tick: Integer;
  cx: Integer;
  cy: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), 800, 600) = 0 then
  begin
    WriteLn('SimLib: set_gfx_mode failed');
    Exit;
  end;

  SimLib_SetWindowTitle('Pascal64 + simlib render target demo');

  tex := SimLib_CreateBitmap(64, 64);
  for j := 0 to 63 do
    for i := 0 to 63 do
      SimLib_PutPixel(tex, i, j, (i * 4) mod 256, (j * 4) mod 256, 200, 255);
  SimLib_UploadBitmap(tex);

  rt := SimLib_CreateRenderTarget(256, 256);
  if rt = nil then
  begin
    WriteLn('SimLib: CreateRenderTarget failed');
    Exit;
  end;

  event := SimLib_EventCreate();
  running := 1;
  tick := 0;

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

    tick := tick + 1;
    cx := (tick * 2) mod 256;
    cy := (tick * 3) mod 256;

    { Render to an offscreen target }
    if SimLib_BeginRenderTarget(rt) <> 0 then
    begin
      SimLib_ClearRenderTarget(10, 10, 20, 255);
      SimLib_SetBlendMode(SimLib_BlendModeAdditive());
      SimLib_DrawSpriteStretched(tex, 0, 0, 256, 256);
      SimLib_RectFill(cx - 20, cy - 20, cx + 20, cy + 20, 255, 80, 30, 200);
      SimLib_CircleFill(128, 128, 40, 40, 180, 255, 180);
      SimLib_SetBlendMode(SimLib_BlendModeNormal());
      SimLib_EndRenderTarget();
    end;

    { Now draw to the window }
    SimLib_ClearToColour(15, 18, 22, 255);

    { Clip to a central area }
    SimLib_SetClipRect(100, 80, 600, 440);
    SimLib_DrawSpriteStretched(rt, 100, 80, 600, 440);
    SimLib_ClearClipRect();

    SimLib_Line(0, 0, SimLib_ScreenWidth(), SimLib_ScreenHeight(), 0, 200, 120, 255);
    SimLib_ShowVideoBitmap();
    SimLib_EndFrame();
  end;

  SimLib_EventDestroy(event);
  SimLib_DestroyBitmap(rt);
  SimLib_DestroyBitmap(tex);
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end.

