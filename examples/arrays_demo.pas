program ArraysDemo;

{ Demonstrates fixed-size arrays with signed bounds, nested arrays, and indexing. }

type
  Numbers = array[-2..2] of Integer;
  NumberGrid = array[0..1] of Numbers;

var
  values: Numbers;
  grid: NumberGrid;
  i: Integer;

begin
  { Fill an array. }
  for i := -2 to 2 do
    values[i] := i * i;

  { Copy arrays by assignment. }
  grid[0] := values;
  grid[1] := values;
  grid[1][0] := 99;

  WriteLn('values:');
  for i := -2 to 2 do
    Write(values[i], ' ');
  WriteLn;

  WriteLn('grid[0]:');
  for i := -2 to 2 do
    Write(grid[0][i], ' ');
  WriteLn;

  WriteLn('grid[1]:');
  for i := -2 to 2 do
    Write(grid[1][i], ' ');
  WriteLn;
end.

