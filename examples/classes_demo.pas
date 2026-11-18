program ClassesDemo;

{ Demonstrates basic classes, inheritance, and virtual dispatch. }

type
  Animal = class
    name: String;
    constructor Create(value: String);
    destructor Destroy;
    function speak: String; virtual;
  end;

  Dog = class(Animal)
    function speak: String; override;
  end;

var
  a: Animal;
  d: Dog;

constructor Animal.Create(value: String);
begin
  Self.name := value;
end;

destructor Animal.Destroy;
begin
  WriteLn('destroy ', Self.name);
end;

function Animal.speak: String;
begin
  Result := '...';
end;

function Dog.speak: String;
begin
  Result := 'woof';
end;

begin
  d := Dog.Create('spot');
  a := d;
  WriteLn(a.name, ' says ', a.speak());
  a.Free;
end.

