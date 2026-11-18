unit P64LibC;

{ Shared libc declarations.

  Pascal64 currently enforces that each external C symbol is declared only once
  across the full program + unit graph, so we centralize common libc externals
  here.
}

interface

type
  FilePtr = Pointer;

function c_fopen(path, mode: PChar): FilePtr; cdecl; external name 'fopen';
function c_fclose(stream: FilePtr): Integer; cdecl; external name 'fclose';
function c_fputs(text: PChar; stream: FilePtr): Integer; cdecl; external name 'fputs';
function c_fputc(ch: Integer; stream: FilePtr): Integer; cdecl; external name 'fputc';
function c_getline(var line: PChar; var capacity: Int64; stream: FilePtr): Int64; cdecl; external name 'getline';

function c_getenv(name: PChar): PChar; cdecl; external name 'getenv';
function c_setenv(name: PChar; value: PChar; overwrite: Integer): Integer; cdecl; external name 'setenv';
function c_unsetenv(name: PChar): Integer; cdecl; external name 'unsetenv';

function c_chdir(path: PChar): Integer; cdecl; external name 'chdir';
function c_getcwd(buf: PChar; size: Int64): PChar; cdecl; external name 'getcwd';

function c_malloc(size: Int64): Pointer; cdecl; external name 'malloc';
procedure c_free(ptr: Pointer); cdecl; external name 'free';

procedure c_exit(code: Integer); cdecl; external name 'exit';

implementation

end.

