program Exceptions;

var
  s: String;

procedure raise_in_proc;
var
  t: String;
begin
  t := 'temp';
  raise;
end;

begin
  s := 'before';
  try
    s := s + '!';
    raise_in_proc;
    WriteLn('not reached');
  except
    WriteLn('caught ', s);
  end;
  WriteLn('after ', s);

  try
    try
      WriteLn('enter finally');
      raise;
    finally
      WriteLn('in finally');
    end;
    WriteLn('not reached2');
  except
    WriteLn('caught2');
  end;

  try
    try
      WriteLn('no raise');
    finally
      WriteLn('finally2');
    end;
  except
    WriteLn('not reached3');
  end;
end.

