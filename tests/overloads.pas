program Overloads;

procedure q(x: Integer);
begin
  WriteLn('i ', x);
end;

procedure q(x: Int64);
begin
  WriteLn('l ', x);
end;

function inc1(x: Integer): Integer;
begin
  Result := x + 1;
end;

function inc1(x: Int64): Int64;
begin
  Result := x + 2;
end;

begin
  q(1);
  q(2147483648);
  WriteLn(inc1(1));
  WriteLn(inc1(2147483648));
end.

