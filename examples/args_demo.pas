program ArgsDemo;

{ Demonstrates reading command-line arguments via the P64Args unit.

  Build:
    ./build/p64 examples/args_demo.pas -o build/args_demo --unit-path units --clang clang-18

  Run:
    ./build/args_demo a b c
}

uses P64Args, P64Std;

var
  i: Integer;

begin
  WriteLn('argc=', IntToStr32(ArgCount));
  for i := 0 to ArgCount - 1 do
    WriteLn('argv[', IntToStr32(i), ']=', Arg(i));
end.

