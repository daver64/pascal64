unit BridgeUnit;
interface
uses BaseUnit;
type WideCount = Count;
function wide(value: WideCount): WideCount; cdecl; external name 'ffi_wide';
procedure bridge_advance;
implementation

procedure bridge_advance;
begin
  advance(3);
end;

initialization
  WriteLn('bridge init ', unit_count);
finalization
  WriteLn('bridge fini ', unit_count);
end.
