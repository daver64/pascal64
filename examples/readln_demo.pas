program readln_demo;

{ Minimal stdin reading support.

  This uses Pascal64's built-in ReadLn statement.
}

var
  name: String;
  age: Integer;

begin
  WriteLn('Enter your name:');
  ReadLn(name);
  WriteLn('Enter your age:');
  ReadLn(age);

  WriteLn('Hello ', name, '! Next year you will be ', age + 1);
end.

