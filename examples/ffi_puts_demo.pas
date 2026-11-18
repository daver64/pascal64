program FFIPutsDemo;

{ Demonstrates a basic cdecl external declaration.

  This calls the C standard library function `puts`.
  On most Linux systems, you do not need extra link flags for libc.
}

var
  message: String;

function puts(text: PChar): Integer; cdecl; external name 'puts';

begin
  puts('hello from puts (PChar literal)');

  message := 'hello from puts (String value)';
  puts(message);
end.

