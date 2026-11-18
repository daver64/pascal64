program RecordsDemo;

{ Demonstrates records, nested routines, and var parameters. }

type
  Point = record
    x: Integer;
    y: Integer;
  end;

var
  p: Point;

function make_point(x, y: Integer): Point;
begin
  Result.x := x;
  Result.y := y;
end;

procedure move_by(var value: Point; dx, dy: Integer);
  procedure bump_x;
  begin
    value.x := value.x + dx;
  end;
  procedure bump_y;
  begin
    value.y := value.y + dy;
  end;
begin
  bump_x;
  bump_y;
end;

begin
  p := make_point(2, 3);
  WriteLn('start: ', p.x, ' ', p.y);
  move_by(p, 5, -2);
  WriteLn('moved: ', p.x, ' ', p.y);
end.

