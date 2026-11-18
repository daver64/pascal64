program Ffi;
const greeting = 'hello';
type
  Numbers = array[1..2] of Integer;
  NumberCallback = function(value: Integer): Integer; cdecl;
  RealCallback = function(value: Real): Real; cdecl;
  ReferenceCallback = procedure(var value: Integer); cdecl;
var data: Numbers;
    text: PChar;
    managed: String;
    object_value: Pointer;
    callback: NumberCallback;
    callback_count: Integer;

function ffi_add(left, right: Integer): Integer; cdecl; external;
function wide(value: Int64): Int64; cdecl; external name 'ffi_wide';
procedure bump(var value: Integer); cdecl; external name 'ffi_bump';
function mixed: Integer; cdecl; external name 'ffi_MixedCase';
function abs(value: Integer): Integer; cdecl; external;
function text_length(value: PChar): Integer; cdecl; external name 'ffi_text_length';
function foreign_text: PChar; cdecl; external name 'ffi_text';
function foreign_object: Pointer; cdecl; external name 'ffi_object';
function read_object(value: Pointer): Integer; cdecl; external name 'ffi_read';
function real_add(left, right: Real): Real; cdecl; external name 'ffi_real_add';
procedure clear_object(var value: Pointer); cdecl; external name 'ffi_clear';
procedure set_callback(value: NumberCallback); cdecl; external name 'ffi_set_callback';
function run_callback(value: Integer): Integer; cdecl; external name 'ffi_run_callback';
procedure set_real_callback(value: RealCallback); cdecl; external name 'ffi_set_real_callback';
function run_real_callback(value: Real): Real; cdecl; external name 'ffi_run_real_callback';
procedure run_reference(value: ReferenceCallback; var number: Integer); cdecl; external name 'ffi_run_reference';

function on_number(value: Integer): Integer;
begin
  callback_count := callback_count + 1;
  Result := value + callback_count;
end;

function on_real(value: Real): Real;
begin
  Result := value + 0.5;
end;

procedure on_reference(var value: Integer);
begin
  value := value + 20;
end;

begin
  data[2] := -10;
  bump(data[2]);
  WriteLn(ffi_add(data[2], 12), ' ', wide(-2), ' ', mixed, ' ', abs(-19));
  if object_value = nil then WriteLn('initially nil');
  text := greeting;
  managed := 'hello!';
  WriteLn(text_length(text), ' ', text_length('abc'), ' ', text_length(foreign_text), ' ', text_length(managed));
  object_value := foreign_object;
  if object_value <> nil then WriteLn(read_object(object_value));
  clear_object(object_value);
  WriteLn(read_object(object_value), ' ', read_object(nil));
  object_value := text;
  if object_value <> nil then WriteLn('pchar converted');
  if callback = nil then WriteLn('no callback');
  callback := @on_number;
  set_callback(callback);
  WriteLn(run_callback(40), ' ', run_callback(40));
  run_reference(@on_reference, data[2]);
  WriteLn(data[2]);
  set_callback(nil);
  WriteLn(run_callback(40));
  WriteLn(real_add(1.5, 2));
  set_real_callback(@on_real);
  WriteLn(run_real_callback(1.25));
end.
