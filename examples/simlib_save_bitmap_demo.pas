program SimLibSaveBitmapDemo;

{ Generates a small bitmap in RAM and saves it as a PNG.

  Build:
    ./build/p64 examples/simlib_save_bitmap_demo.pas -o build/simlib_save_bitmap_demo --clang clang --unit-path units --link build/simlib/libp64simlib.so

  Run:
    ./build/simlib_save_bitmap_demo

  Output:
    build/simlib_saved_bitmap.png
}

uses P64Std, P64SimLib;

var
  bmp: PSimLibBitmap;
  x: Integer;
  y: Integer;
  ok: Integer;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));

  bmp := SimLib_CreateBitmap(256, 256);
  if bmp = nil then
  begin
    WriteLn('SimLib: create_bitmap failed');
    Exit;
  end;

  { In rsimlib, acquire/release marks the bitmap for safe CPU access. }
  SimLib_AcquireBitmap(bmp);
  for y := 0 to 255 do
    for x := 0 to 255 do
      SimLib_PutPixel(bmp, x, y, x, y, 200, 255);
  SimLib_ReleaseBitmap(bmp);

  ok := SimLib_SaveBitmap(bmp, 'build/simlib_saved_bitmap.png');
  if ok = 0 then
    WriteLn('SimLib: save_bitmap failed')
  else
    WriteLn('Wrote build/simlib_saved_bitmap.png');

  SimLib_DestroyBitmap(bmp);
end.
