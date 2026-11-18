program DiscardResult;

var
  calls: Integer;

function bump: Integer;
begin
  calls := calls + 1;
  Result := calls;
end;

begin
  bump;
  bump;
  WriteLn(calls);
end.

