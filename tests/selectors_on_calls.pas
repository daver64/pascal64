program SelectorsOnCalls;

type
  Values = array[0..1] of Integer;
  Item = record
    title: String;
    values: Values;
  end;

function make_item: Item;
begin
  Result.title := 'hello';
  Result.values[0] := 7;
  Result.values[1] := 9;
end;

begin
  WriteLn(make_item.title);
  WriteLn(make_item.values[0]);
  WriteLn(make_item.values[1]);
end.

