unit Greeting;

interface

uses MathUtils;

function greeting_for(name: String): String;

implementation

function greeting_for(name: String): String;
begin
  { Keep this unit self-contained: avoid implicit numeric-to-string conversions. }
  Result := 'Hello ' + name;
end;

initialization
  WriteLn('Greeting init');

finalization
  WriteLn('Greeting fini');

end.

