program Classes;
type
  Names = array[0..1] of String;
  Node = class
    name: String;
    values: Names;
    next: Node;
    count: Integer;
    procedure add(amount: Integer);
    procedure reset;
    function title: String;
  end;
  NodeAlias = Node;
  Holder = record item: Node end;
  Base = class
    text: String;
    constructor Create(value: String; var count: Integer);
    destructor Destroy;
    procedure set_text(value: String); virtual;
    function describe: String; virtual;
  end;
  Child = class(Base)
    extra: String;
    destructor Destroy;
    procedure set_text(value: String); override;
    function describe: String; override;
    procedure show;
  end;
  OwnChild = class(Base)
    constructor Create(value: String);
  end;
var
  first, alias_node: Node;
  container: Holder;
  descendant: Child;
  base_ref: Base;
  own: OwnChild;
  created: Integer;

constructor Base.Create(value: String; var count: Integer);
begin
  Self.text := value;
  count := count + 1;
end;

destructor Base.Destroy;
begin
  WriteLn('base destroy ', Self.text);
end;

destructor Child.Destroy;
begin
  WriteLn('child destroy ', Self.extra);
end;

constructor OwnChild.Create(value: String);
begin
  Self.text := value + ' own';
end;

procedure Node.add(amount: Integer);
begin
  Self.count := Self.count + amount;
end;

procedure Node.reset;
begin
  Self.count := 0;
end;

function Node.title: String;
begin
  Result := Self.name + '!';
end;

procedure Base.set_text(value: String);
begin
  Self.text := value;
end;

function Base.describe: String;
begin
  Result := 'base description';
end;

procedure Child.set_text(value: String);
begin
  Self.text := value + '!';
end;

function Child.describe: String;
begin
  Result := Self.extra + '?';
end;

procedure Child.show;
begin
  WriteLn(Self.text, ' ', Self.extra);
end;

function make_node: NodeAlias;
begin
  Result := Node.Create();
  Result.name := 'parent';
  Result.values[0] := 'one';
end;

procedure update(value: Node);
begin
  value.count := value.count + 1;
end;

begin
  if first = nil then WriteLn('initially nil');
  first.Free;
  first := make_node;
  first.next := Node.Create;
  first.next.name := 'child';
  alias_node := first;
  container.item := first.next;
  update(alias_node);
  WriteLn(first.name, ' ', first.values[0], ' ', container.item.name, ' ', first.count);
  first.add(4);
  WriteLn(first.title(), ' ', first.count);
  first.reset;
  WriteLn(first.count);
  if first = alias_node then WriteLn('same object');
  container.item.Free();
  first.next := nil;
  first.Free;
  first := nil;
  alias_node := nil;
  container.item := nil;
  descendant := Child.Create('constructed', created);
  WriteLn(descendant.text, ' ', created);
  descendant.extra := 'child';
  base_ref := descendant;
  base_ref.set_text('base');
  descendant.show;
  WriteLn(base_ref.describe());
  base_ref.Free;
  descendant := nil;
  base_ref := nil;
  own := OwnChild.Create('derived');
  WriteLn(own.text, ' ', created);
  own.Free;
end.
