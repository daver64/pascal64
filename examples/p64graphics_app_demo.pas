program P64GraphicsAppDemo;

{ Demonstrates the higher-level `P64Graphics` helpers.

  - `TApplication` runs a simple frame loop (Escape quits).
  - `TBitmap` wraps a simlib bitmap / texture.
  - `TSpriteBatch` draws many sprites efficiently.

  This is intended to be "Delphi-ish" rather than strictly Turbo Pascal.
}

uses P64Std, P64SimLib, P64Graphics;

var
  App: TApplication;
  Tex: TBitmap;
  Batch: TSpriteBatch;
  T0: Int64;
  FrameNo: Integer;

procedure BuildTexture;
var
  i: Integer;
  j: Integer;
  c: Integer;
begin
  Tex := TBitmap.Create(64, 64);
  if Tex = nil then
    Exit;

  { A small procedural texture (checker + gradient). }
  for j := 0 to 63 do
  begin
    for i := 0 to 63 do
    begin
      c := ((i div 8) + (j div 8)) mod 2;
      if c = 0 then
        Tex.PutPixel(i, j, ColourRGB(220, 220, 255))
      else
        Tex.PutPixel(i, j, ColourRGB(80 + i * 2, 40 + j * 2, 60));
    end;
  end;
  Tex.Upload;
end;

procedure Frame;
var
  now_ms: Int64;
  t: Real;
  k: Integer;
  cx: Real;
  cy: Real;
  x: Real;
  y: Real;
  ang: Real;
  rad: Real;
  sx: Real;
  sy: Real;
begin
  now_ms := SimLib_TimeMs();
  t := (now_ms - T0) / 1000.0;
  FrameNo := FrameNo + 1;

  Clear(ColourRGB(15, 18, 22));

  { Centre the swarm around the mouse. }
  cx := SimLib_MouseX();
  cy := SimLib_MouseY();

  Batch.Clear;
  for k := 0 to 300 do
  begin
    { Orbit around the mouse. }
    ang := (t * 1.3) + (k * 0.11);
    rad := 20.0 + ((k mod 60) * 2.4);
    x := cx + (rad * SimLib_Cos(ang));
    y := cy + (rad * SimLib_Sin(ang));
    sx := 0.7 + (0.5 * ((k mod 11) / 10.0));
    sy := sx;
    { Keep alpha modest; combined with non-additive blending avoids saturation. }
    Batch.Add(x, y, sx, sy, ang * 57.295779513, 0, 0, 64, 64, ColourRGBA(255, 255, 255, 90));
  end;

  { Additive blending looks nice, but it saturates quickly with lots of overlap.
    Normal blending keeps the texture visible and avoids the big white blob. }
  SetBlendModeNormal;
  Batch.Draw(Tex);

  { Some immediate-mode primitives over the top. }
  SimLib_Line(0, 0, ScreenWidth, ScreenHeight, 0, 200, 120, 255);
  SimLib_CircleFill(ScreenWidth div 2, ScreenHeight div 2, 24, 255, 80, 30, 220);

  if SimLib_MouseButtons() <> 0 then
  begin
    SimLib_RectFill(20, 20, 200, 60, 40, 60, 120, 220);
  end;
end;

begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  InitWindow(960, 540, 'Pascal64 + simlib (P64Graphics app demo)');

  App := TApplication.Create;
  Batch := TSpriteBatch.Create;
  BuildTexture;
  T0 := SimLib_TimeMs();

  App.Run(@Frame);

  Tex.Free;
  Batch.Free;
  App.Free;
  ShutdownWindow;
end.

