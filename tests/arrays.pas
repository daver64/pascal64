program Arrays;
type
  Numbers = array[-2..1] of Integer;
  NumbersAlias = Numbers;
  Point = record x: Integer end;
  Points = array[1..2] of Point;
  Container = record values: Numbers; points: Points end;
  Grid = array[0..1] of Numbers;
var
  data: Container;
  grid_data: Grid;
  copy: NumbersAlias;
  calls, counter: Integer;

function index: Integer;
begin
  calls := calls + 1;
  Result := -1;
end;

function make_numbers: Numbers;
begin
  Result[-2] := 7;
  Result[1] := 11;
end;

procedure update(var values: NumbersAlias);
  procedure nested;
  begin
    values[-1] := 42;
  end;
begin
  nested;
end;

procedure by_value(values: Numbers);
begin
  values[-2] := 99;
  WriteLn(values[-2]);
end;

begin
  WriteLn(data.values[-2], ' ', data.points[2].x);
  data.values := make_numbers;
  copy := data.values;
  update(data.values);
  data.points[1].x := data.values[index()];
  grid_data[1][-2] := data.points[1].x;
  by_value(copy);
  WriteLn(copy[-2], ' ', data.values[-1], ' ', grid_data[1][-2], ' ', calls);
  for grid_data[0][-2] := 1 to 3 do Write(grid_data[0][-2]);
  WriteLn;
  counter := 1;
  WriteLn(data.values[counter]);
end.
