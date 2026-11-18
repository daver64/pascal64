unit P64Args;

{ Command-line argument access for Pascal64.

  Notes:
  - Arguments are exposed as UTF-8 bytes via `PChar` and converted to `String`.
  - `argv[0]` is the program path like in C.
}

interface

function ArgCount: Integer;
function Arg(index: Integer): String;

implementation

function p64rt_get_argc: Integer; cdecl; external name 'p64rt_get_argc';
function p64rt_get_argv(index: Int64): PChar; cdecl; external name 'p64rt_get_argv';

function ArgCount: Integer;
begin
  Result := p64rt_get_argc();
end;

function Arg(index: Integer): String;
begin
  Result := p64rt_get_argv(index);
end;

end.

