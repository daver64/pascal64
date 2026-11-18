program ForwardTest;

procedure second; forward;

procedure first;
begin
  second;
end;

procedure second;
begin
  WriteLn('ok');
end;

begin
  first;
end.

