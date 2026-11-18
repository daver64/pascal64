program UnitTest;
uses BaseUnit, BridgeUnit;
type
  NumberCallback = function(value: Integer): Integer; cdecl;
  LocalCounter = class(UnitCounter)
    destructor Destroy;
    procedure add(amount: Integer); override;
  end;
var value: WideCount;
    counter: UnitCounter;
    override_calls: Integer;
procedure set_callback(value: NumberCallback); cdecl; external name 'ffi_set_callback';
function run_callback(value: Integer): Integer; cdecl; external name 'ffi_run_callback';

destructor LocalCounter.Destroy;
begin
  WriteLn('local destroy ', Self.value);
end;

procedure LocalCounter.add(amount: Integer);
begin
  Self.value := Self.value + amount * 2;
  override_calls := override_calls + 1;
end;

begin
  value := wide(3);
  WriteLn(unit_message, ' ', ffi_add(2, 5), ' ', value);
  bridge_advance;
  WriteLn(unit_count, ' ', secret_text);
  set_callback(@add_unit_count);
  WriteLn('unit callback ', run_callback(10));
  set_callback(nil);
  counter := LocalCounter.Create(3);
  counter.add(8);
  WriteLn('unit class ', counter.value, ' ', override_calls);
  counter.Free;
end.
