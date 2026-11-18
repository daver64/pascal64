program StringsDemo;

{ Demonstrates Pascal64 managed strings (immutable), concatenation, comparisons,
  records/arrays containing strings, and var parameters. }

type
  Text = String;
  TextArray = array[0..2] of Text;
  ItemRecord = record
    title: Text;
    tags: TextArray;
  end;

var
  s: String;
  item: ItemRecord;

procedure append(var value: String; suffix: String);
begin
  value := value + suffix;
end;

begin
  s := 'Hello';
  append(s, ' from Pascal64');
  WriteLn(s);

  if s > 'A' then
    WriteLn('comparison works');

  item.title := 'demo';
  item.tags[0] := 'strings';
  item.tags[1] := 'records';
  item.tags[2] := 'arrays';

  WriteLn(item.title, ': ', item.tags[0], ', ', item.tags[1], ', ', item.tags[2]);
end.

