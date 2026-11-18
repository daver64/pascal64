program Routines;
const
  greeting = 'constant';
  copy_greeting = greeting;
  limit = 2147483648;
  low_value = -9223372036854775808;
  enabled = true;
  quantity = 3 * (4 + 1) div 5;
  big = limit + 2;
  inverse = -(quantity + 2);
  calc_enabled = enabled and (quantity = 3) and not false;
  null_value = nil;
  negative_division = -7 div 3;
  negative_remainder = -7 mod 3;
var total: Int64;
    number, inner_number: Integer;
    flag: Boolean;

function factorial(n: Integer): Int64;
begin
  if n <= 1 then
    Result := 1
  else
    factorial := n * factorial(n - 1);
end;

procedure accumulate(base: Integer);
var local: Integer;
  procedure add(value: Integer);
    procedure inner;
    begin
      total := total + base + local + value;
    end;
  begin
    inner;
  end;
begin
  local := 3;
  add(4);
end;

function answer: Integer;
begin
  Result := 42;
end;

procedure shadow(total: Integer);
  procedure show;
  begin
    WriteLn(total);
  end;
begin
  show;
end;

procedure bump(var value: Integer; amount: Integer);
  procedure inner;
  begin
    value := value + amount;
  end;
begin
  inner;
end;

procedure forward_value(var value: Integer);
begin
  bump(value, 3);
end;

procedure constants_scope;
const
  limit = 17;
  local_greeting = copy_greeting;
  local_enabled = enabled;
  procedure inner;
  begin
    if local_enabled then WriteLn(local_greeting, ' ', limit);
  end;
begin
  inner;
end;

procedure constant_shadow(limit: Integer);
begin
  WriteLn(limit);
end;

begin
  total := 0;
  accumulate(5);
  total := total + factorial(5);
  WriteLn(total, ' ', answer);
  flag := (total = 132) and not false;
  if flag then WriteLn('boolean works');
  shadow(7);
  number := 2;
  forward_value(number);
  WriteLn(number);
  repeat
    number := number + 1;
  until number = 7;
  WriteLn(number);
  repeat number := number + 1 until true;
  WriteLn(number);
  for number := 2147483647 to 2147483647 do WriteLn(number);
  for number := -2147483648 downto -2147483648 do WriteLn(number);
  if enabled then WriteLn(copy_greeting, ' ', limit, ' ', low_value);
  constants_scope;
  constant_shadow(23);
  if calc_enabled and (null_value = nil) then
    WriteLn(quantity, ' ', big, ' ', inverse, ' ', negative_division, ' ', negative_remainder);
  number := 0;
  total := 0;
  while number < 5 do
  begin
    number := number + 1;
    if number = 2 then continue;
    if number = 5 then break;
    total := total + number;
  end;
  WriteLn(total, ' ', number);
  repeat
    number := number - 1;
    if number > 2 then continue;
    total := total + number;
  until number = 1;
  for number := 2147483647 to 2147483647 do continue;
  for number := -2147483648 downto -2147483648 do continue;
  for number := 1 to 3 do
  begin
    if number = 2 then continue;
    for inner_number := 1 to 3 do
    begin
      if inner_number = 2 then break;
      total := total + 1;
    end;
  end;
  WriteLn(total);
end.
