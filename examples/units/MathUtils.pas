unit MathUtils;

interface

function add(left, right: Integer): Integer;

implementation

function add(left, right: Integer): Integer;
begin
  Result := left + right;
end;

initialization
  WriteLn('MathUtils init');

finalization
  WriteLn('MathUtils fini');

end.

