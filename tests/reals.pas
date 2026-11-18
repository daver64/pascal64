program Reals;

var
  a: Real;
  b: Real;

begin
  a := 1.5;
  b := 2;
  WriteLn(a + b);
  WriteLn(a * b);
  WriteLn(b / 4);
  if (a < 2.0) and (b = 2) then WriteLn('ok');
end.

