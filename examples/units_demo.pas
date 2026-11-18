program UnitsDemo;

{ Demonstrates units, dependency-ordered initialization, and finalization.

  Build:
    ./build/p64 examples/units_demo.pas -o build/units_demo --unit-path examples/units --clang clang-18
}

uses Greeting, MathUtils;

begin
  WriteLn(greeting_for('Pascal64'));
  WriteLn('2 + 3 = ', add(2, 3));
end.

