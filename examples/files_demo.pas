program FilesDemo;

{ Demonstrates basic file output using the P64Std unit.

  Build:
    ./build/p64 examples/files_demo.pas -o build/files_demo --unit-path units --clang clang-18 --run
}

uses P64Std;

var
  path: String;
  i: Integer;
  content: String;

begin
  path := 'p64_output.txt';

  WriteTextFileLn(path, 'Pascal64 file output demo');
  for i := 1 to 5 do
    AppendTextFileLn(path, 'line ' + IntToStr32(i));

  content := ReadAllTextFile(path);
  WriteLn('--- read back ---');
  WriteLn(content);

  WriteLn('wrote ', path);
end.

