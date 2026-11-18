program Records;
type
  Coordinate = Integer;
  CoordinateAlias = Coordinate;
  Point = record
    x, y: CoordinateAlias;
    valid: Boolean
  end;
  PointAlias = Point;
  Box = record
    origin: PointAlias;
    weight: Int64;
  end;
var
  item: Box;
  saved: PointAlias;

function make_point(first, second: Integer): Point;
begin
  Result.x := first;
  Result.y := second;
  Result.valid := true;
end;

procedure shift(var target: PointAlias; amount: Coordinate);
  procedure nested;
  begin
    target.x := target.x + amount;
  end;
begin
  nested;
end;

procedure touch(var value: Integer);
begin
  value := 9;
end;

procedure by_value(value: Point);
begin
  value.x := 99;
  WriteLn(value.x);
end;

procedure local_type;
type Point = record x: Int64 end;
var local: Point;
begin
  local.x := 5000000000;
  WriteLn(local.x);
end;

procedure shadow_builtin;
type Integer = record value: Int64 end;
var local: Integer;
begin
  local.value := 12;
  WriteLn(local.value);
end;

procedure local_alias;
type Point = Int64;
var local: Point;
begin
  local := 6000000000;
  WriteLn(local);
end;

begin
  WriteLn(item.origin.x, ' ', item.weight);
  item.origin := make_point(2, 3);
  saved := item.origin;
  shift(item.origin, 5);
  touch(item.origin.y);
  WriteLn(item.origin.x, ' ', item.origin.y, ' ', saved.x);
  by_value(saved);
  WriteLn(saved.x);
  if item.origin.valid then WriteLn('valid');
  for item.origin.y := 1 to 3 do Write(item.origin.y);
  WriteLn;
  local_type;
  shadow_builtin;
  local_alias;
end.
