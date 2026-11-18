program ExceptionsDemo;

{ Demonstrates try/except, try/finally, and raise. }

var
  s: String;

procedure fail;
var
  temp: String;
begin
  temp := 'temporary';
  raise;
end;

begin
  s := 'before';
  try
    s := s + '!';
    fail;
    WriteLn('not reached');
  except
    WriteLn('caught: ', s);
  end;
  WriteLn('after: ', s);

  try
    try
      WriteLn('enter finally');
      raise;
    finally
      WriteLn('in finally');
    end;
  except
    WriteLn('caught again');
  end;
end.

