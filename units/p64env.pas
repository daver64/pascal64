unit P64Env;

{ Environment and process helpers for Pascal64.

  This unit is intentionally small and libc-based.
}

interface

function GetEnv(name: String): String;
function SetEnv(name: String; value: String): Integer;
function UnsetEnv(name: String): Integer;

function GetCwd: String;
function ChDir(path: String): Integer;

implementation

uses P64LibC;

function GetEnv(name: String): String;
begin
  Result := c_getenv(name);
end;

function SetEnv(name: String; value: String): Integer;
begin
  Result := c_setenv(name, value, 1);
end;

function UnsetEnv(name: String): Integer;
begin
  Result := c_unsetenv(name);
end;

function GetCwd: String;
var
  buf: PChar;
  raw: PChar;
begin
  Result := '';
  buf := c_malloc(4096);
  if buf <> nil then
  begin
    raw := c_getcwd(buf, 4096);
    if raw <> nil then
      Result := raw;
    c_free(buf);
  end;
end;

function ChDir(path: String): Integer;
begin
  Result := c_chdir(path);
end;

end.

