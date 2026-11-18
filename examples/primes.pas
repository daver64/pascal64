program Primes;
var n, d, prime: Integer;
begin
  for n := 2 to 40 do
  begin
    prime := 1;
    d := 2;
    while d * d <= n do
    begin
      if n mod d = 0 then
        prime := 0;
      d := d + 1;
    end;
    if prime = 1 then Write(n, ' ');
  end;
  WriteLn;
end.
