unit P64Std;

{ Small "standard" unit for Pascal64 programs.

  This is intentionally tiny and pragmatic: it provides just enough helpers to
  build real programs without requiring Delphi/Turbo Pascal compatibility.
}

interface

function IntToStr(value: Int64): String;
function IntToStr32(value: Integer): String;

function StrFromPChar(text: PChar): String;

function ReadAllTextFile(path: String): String;

procedure WriteTextFile(path: String; content: String);
procedure AppendTextFile(path: String; content: String);
procedure WriteTextFileLn(path: String; line: String);
procedure AppendTextFileLn(path: String; line: String);

implementation

type
  FilePtr = Pointer;

function fopen(path, mode: PChar): FilePtr; cdecl; external name 'fopen';
function fclose(stream: FilePtr): Integer; cdecl; external name 'fclose';
function fputs(text: PChar; stream: FilePtr): Integer; cdecl; external name 'fputs';
function fputc(ch: Integer; stream: FilePtr): Integer; cdecl; external name 'fputc';
function getline(var line: PChar; var capacity: Int64; stream: FilePtr): Int64; cdecl; external name 'getline';
procedure free(ptr: Pointer); cdecl; external name 'free';
procedure c_exit(code: Integer); cdecl; external name 'exit';

function digit_str(value: Int64): String;
begin
  if value = 0 then Result := '0'
  else if value = 1 then Result := '1'
  else if value = 2 then Result := '2'
  else if value = 3 then Result := '3'
  else if value = 4 then Result := '4'
  else if value = 5 then Result := '5'
  else if value = 6 then Result := '6'
  else if value = 7 then Result := '7'
  else if value = 8 then Result := '8'
  else Result := '9';
end;

function IntToStr(value: Int64): String;
begin
  if value < 0 then
    Result := '-' + IntToStr(-value)
  else if value < 10 then
    Result := digit_str(value)
  else
    Result := IntToStr(value div 10) + digit_str(value mod 10);
end;

function IntToStr32(value: Integer): String;
begin
  Result := IntToStr(value);
end;

function StrFromPChar(text: PChar): String;
begin
  Result := text;
end;

function ReadAllTextFile(path: String): String;
var
  stream: FilePtr;
  line: PChar;
  cap: Int64;
  len: Int64;
begin
  Result := '';
  stream := fopen(path, 'rb');
  if stream <> nil then
  begin
    line := nil;
    cap := 0;
    while true do
    begin
      len := getline(line, cap, stream);
      if len < 0 then
        break;
      Result := Result + StrFromPChar(line);
    end;
    if line <> nil then
      free(line);
    fclose(stream);
  end;
end;

procedure write_impl(path: String; mode: PChar; content: String);
var stream: FilePtr;
begin
  stream := fopen(path, mode);
  if stream = nil then
  begin
    WriteLn('P64Std: cannot open ', path);
    c_exit(1);
  end;
  fputs(content, stream);
  fclose(stream);
end;

procedure write_line_impl(path: String; mode: PChar; line: String);
var stream: FilePtr;
begin
  stream := fopen(path, mode);
  if stream = nil then
  begin
    WriteLn('P64Std: cannot open ', path);
    c_exit(1);
  end;
  fputs(line, stream);
  fputc(10, stream);
  fclose(stream);
end;

procedure WriteTextFile(path: String; content: String);
begin
  write_impl(path, 'w', content);
end;

procedure AppendTextFile(path: String; content: String);
begin
  write_impl(path, 'a', content);
end;

procedure WriteTextFileLn(path: String; line: String);
begin
  write_line_impl(path, 'w', line);
end;

procedure AppendTextFileLn(path: String; line: String);
begin
  write_line_impl(path, 'a', line);
end;

end.

