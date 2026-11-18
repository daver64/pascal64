program ShortCircuit;

var
  x: Integer;

function side: Boolean;
begin
  x := x + 1;
  Result := true;
end;

begin
  x := 0;
  if false and side then WriteLn('bad');
  WriteLn(x);
  if true or side then ;
  WriteLn(x);
  if true and side then ;
  WriteLn(x);
  if false or side then ;
  WriteLn(x);
end.

