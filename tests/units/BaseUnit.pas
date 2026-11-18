unit BaseUnit;
interface
const unit_message = 'unit loaded';
type
  Count = Int64;
  DebugNumbers = array[-1..1] of Integer;
  DebugData = record
    marker: Boolean;
    numbers: DebugNumbers;
    label_text: String;
  end;
  UnitCounter = class
    value: Integer;
    constructor Create(initial: Integer);
    destructor Destroy;
    procedure add(amount: Integer); virtual;
  end;
var unit_count: Integer;
    debug_payload: DebugData;
function ffi_add(left, right: Integer): Integer; cdecl; external;
procedure advance(amount: Integer);
function secret_text: String;
function add_unit_count(value: Integer): Integer;
implementation
var hidden: String;

constructor UnitCounter.Create(initial: Integer);
begin
  Self.value := initial;
  unit_count := unit_count + 1;
end;

destructor UnitCounter.Destroy;
begin
  WriteLn('unit destroy ', Self.value, ' ', hidden);
end;

procedure UnitCounter.add(amount: Integer);
begin
  Self.value := Self.value + amount;
end;

procedure advance(amount: Integer);
begin
  unit_count := unit_count + amount;
end;

function secret_text: String;
begin
  Result := hidden;
end;

function add_unit_count(value: Integer): Integer;
begin
  Result := value + unit_count;
end;

initialization
  unit_count := 2;
  debug_payload.numbers[-1] := 23;
  debug_payload.label_text := 'debug data';
  hidden := 'private';
  WriteLn('base init');
finalization
  WriteLn('base fini ', hidden);
end.
