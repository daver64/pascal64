program RealsConst;

const
  a = 1.5;
  b = 2;
  sum = a + b;
  divv = b / 4;
  ok = (sum = 3.5) and (divv < 1.0);

begin
  WriteLn(sum);
  WriteLn(divv);
  if ok then WriteLn('ok');
end.

