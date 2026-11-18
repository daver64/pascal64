program Strings;
const
  part1 = 'ab';
  part2 = 'cd';
  combined = part1 + part2 + 'e';
  combined_ok = combined = 'abcde';
type
  Text = String;
  TextArray = array[-1..1] of Text;
  TextRecord = record title: Text; items: TextArray end;
  TextGrid = array[0..1] of TextRecord;
var current, saved: Text;
    first, second: TextRecord;
    grid: TextGrid;

function mutate: String;
begin
  current := 'changed';
  Result := '!';
end;

function build(count: Integer): String;
begin
  if count = 0 then
    Result := ''
  else
    Result := build(count - 1) + 'x';
end;

function identity(value: String): String;
begin
  Result := value;
end;

procedure by_value(value: String);
begin
  value := value + '!';
  WriteLn(value);
end;

procedure by_reference(var value: String);
  procedure nested;
  begin
    value := value + ' ✓';
  end;
begin
  nested;
end;

function make_record: TextRecord;
begin
  Result.title := 'title';
  Result.items[-1] := 'item';
end;

function change_record: Integer;
begin
  first.title := 'changed';
  Result := 0;
end;

procedure inspect(value: TextRecord; ignored: Integer);
begin
  WriteLn(value.title, ' ', value.items[-1]);
  value.title := 'local';
  value.items[-1] := 'local item';
end;

procedure update_record(var value: TextRecord);
begin
  value.items[-1] := value.items[-1] + '!';
end;

begin
  if current = '' then WriteLn('empty');
  current := 'before';
  saved := current;
  current := current + mutate;
  WriteLn(current, ' ', saved);
  saved := saved;
  by_value(saved);
  WriteLn(saved);
  by_reference(saved);
  WriteLn(identity(saved));
  WriteLn(build(4));
  if (saved > 'before') and (identity('abc') = 'abc') then WriteLn('comparison');
  current := build(0);
  if current = '' then WriteLn('empty result');
  first := make_record;
  second := first;
  first := first;
  inspect(first, change_record);
  WriteLn(first.title, ' ', second.title);
  update_record(second);
  grid[0] := second;
  grid[1] := grid[0];
  grid[0].items[-1] := 'new';
  WriteLn(grid[0].items[-1], ' ', grid[1].items[-1]);
  grid := grid;
  WriteLn(combined);
  if combined_ok then WriteLn('const ok') else WriteLn('const bad');
end.
