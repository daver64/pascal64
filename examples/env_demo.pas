program EnvDemo;

{ Demonstrates environment variables and current working directory.

  Build:
    ./build/p64 examples/env_demo.pas -o build/env_demo --unit-path units --clang clang-18

  Run:
    ./build/env_demo
    ./build/env_demo HOME
}

uses P64Args, P64Env, P64Std;

var
  name: String;
begin
  WriteLn('cwd=', GetCwd);

  if ArgCount > 1 then
    name := Arg(1)
  else
    name := 'HOME';

  WriteLn(name, '=', GetEnv(name));

  SetEnv('P64_DEMO', '123');
  WriteLn('P64_DEMO=', GetEnv('P64_DEMO'));
end.

