# Pascal64 — experimental Pascal compiler (prototype)

Pascal64 is an experimental native Pascal compiler for Linux x86-64. The C++20
frontend generates textual LLVM IR, and Clang links it with a small C++ runtime.
It is a prototype, not a Delphi-compatible compiler.

## Requirements

- CMake 3.20+, a C++20 compiler, and Clang 17+ on `PATH`
- Linux x86-64

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/p64 examples/hello.pas -o build/hello --clang clang-18
./build/hello
```

Note: the `string_lifetime` test uses AddressSanitizer and can also use
LeakSanitizer when available.

LeakSanitizer does not work under `ptrace` (common in sandboxed/CI
environments), so leak detection is **disabled by default** for reliability.
To force leak detection on/off explicitly, set `P64_ASAN_DETECT_LEAKS` to `1`
or `0` when running CTest.

## Examples

The `examples/` directory contains small curated programs demonstrating the
currently supported language subset.

```sh
./build/p64 examples/records_demo.pas -o build/records_demo --clang clang-18
./build/p64 examples/arrays_demo.pas -o build/arrays_demo --clang clang-18
./build/p64 examples/strings_demo.pas -o build/strings_demo --clang clang-18
./build/p64 examples/exceptions_demo.pas -o build/exceptions_demo --clang clang-18
./build/p64 examples/classes_demo.pas -o build/classes_demo --clang clang-18
./build/p64 examples/ffi_puts_demo.pas -o build/ffi_puts_demo --clang clang-18
./build/p64 examples/units_demo.pas -o build/units_demo --unit-path examples/units --clang clang-18
./build/p64 examples/files_demo.pas -o build/files_demo --unit-path units --clang clang-18
./build/p64 examples/args_demo.pas -o build/args_demo --unit-path units --clang clang-18
./build/p64 examples/env_demo.pas -o build/env_demo --unit-path units --clang clang-18
./build/p64 examples/readln_demo.pas -o build/readln_demo --clang clang-18

# Requires GTK3 installed.
./build/p64 examples/vcl_hello.pas -o build/vcl_hello --unit-path units --clang clang-18 --link -lgtk-3 --link -lgobject-2.0
```

Run the arguments example:

```sh
./build/args_demo a b c
```

The `P64Std` unit (in `units/p64std.pas`) contains small pragmatic helpers such
as `IntToStr` and simple file input/output wrappers.

The `P64Args` unit (in `units/p64args.pas`) provides basic access to program
arguments.

The `P64Env` unit (in `units/p64env.pas`) provides small environment and
process helpers.

The `P64LibC` unit (in `units/p64libc.pas`) centralizes common libc `external`
declarations. Pascal64 currently requires each external C symbol to be declared
only once across the full program + unit graph.

The LLVM IR is retained beside the executable. Use `--emit-llvm path` to choose
its location and `--clang path` to choose the Clang executable. The compiler
links against the runtime in its build directory; moving the compiler alone is
not currently supported. Use `--link path` (repeatable) to pass object files or
libraries to Clang when linking a program. Use `--unit-path directory` (repeatable)
to add directories searched for units, after the source file's directory.
Use `--debug` or `-g` for DWARF source-line and routine information, including
unit sources, primitive locals and parameters, and managed-string pointers.
Unit variables have global debug entries; records and fixed arrays include
layout information. Managed strings appear as opaque pointers, so their contents
can now be inspected as a runtime struct containing a byte pointer and length.

For a friendlier display in GDB, you can load the included pretty-printer:

```gdb
(gdb) source /path/to/pascal64/tools/gdb/p64_printers.py
```

## Language subset

- Programs, variables, assignments, `begin`/`end`, `if`, `while`, `repeat`,
  `for`, `break`, `continue` and `exit`.
- `Integer` and `LongInt` use signed 32-bit storage; `Int64` and `NativeInt`
  use signed 64-bit storage. `Boolean` values are `true` and `false`.
- `Real` values use 64-bit IEEE-754 storage.
- `Pointer` and `PChar` hold opaque pointers and begin as `nil`. `PChar` accepts
  string literals and is assignable to/from `Pointer`; pointer values support
  equality comparisons but not dereferencing or arithmetic. Literal-backed
  `PChar` values point to static, NUL-terminated UTF-8 bytes.
- `String` is an immutable, reference-counted UTF-8 byte string. String
  variables start empty and support assignment, concatenation, comparisons,
  value and `var` parameters, function results and output, including in records
  and arrays. String comparison uses byte order. Implicit `String` to `PChar`
  conversion is supported for string literals and when passing a `String`
  value to a `PChar` parameter in an external `cdecl` call (the `PChar` is only
  guaranteed valid for the duration of the call). String constant expressions
  are supported in `const` sections for concatenation and comparisons.
- Integer arithmetic, comparisons, and Boolean `not`, `and`, and `or`.
- Named constants with integer, real, Boolean, string or `nil` values. Integer,
  real, Boolean and string constant expressions can use previously declared constants; integer
  overflow and division by zero are diagnosed at compile time. Constant
  sections precede type and variable sections; typed constants are unsupported.
- Named record types declared in a `type` section, with nested record fields,
  field access, assignment, value and `var` parameters, and function results.
  Each record declaration defines a distinct type; variables start zeroed.
- Named fixed-size arrays with signed integer literal bounds, checked indexing,
  nested arrays and records, aggregate copies, parameters and function results.
  Each array declaration defines a distinct type; variables start zeroed.
- Basic class declarations have fields and reference semantics. `T.Create`
  allocates a zeroed instance, and `instance.Free` releases its managed fields
  and storage; dereferencing `nil` is diagnosed. Class instances are explicitly
  owned: `Free` does not clear aliases or recursively free class-valued fields.
  Procedures and functions declared in the class and defined as `T.Method` can
  access fields through `Self`; expression calls require `()`. Single inheritance,
  `virtual`/`override` dispatch and destruction through a base reference are
  supported. Classes may declare `constructor Create(...)` and `destructor
  Destroy`; `Create` calls the nearest inherited constructor when none is
  declared locally. `Free` runs destructors from the most-derived class to the
  base class automatically, then releases managed fields. This automatic chain
  differs from Delphi's explicit `inherited` calls. Lifecycle methods cannot be
  called directly, take virtual/override directives, or have alternate names;
  destructors take no arguments and require program- or unit-scope classes.
- Type aliases for built-in or previously declared types, including records;
  aliases retain the identity and compatibility of their underlying type.
- Procedures and functions with value and `var` parameters, recursion, local
  variables and lexically nested routines. `var` arguments must be variables
  with exactly the parameter type. Assign a function's return value to
  `Result` or the function name. Local variables and results start at zero.
  Routine overloading by parameter types is supported.
- Program-scope `cdecl; external` procedures and functions can call C symbols,
  optionally named with `external name 'symbol'`. Their parameters must be
  `Integer`/`LongInt`, `Int64`/`NativeInt`, `Real`, `Pointer` or `PChar` (or aliases),
  passed by value or `var`; functions may return those types. Named `cdecl`
  callback types can be passed to C using `@` on a matching program- or unit-scope
  Pascal routine. A callback can access its module variables and remains valid
  only while the module is initialized. Nested-routine callbacks, Boolean and aggregate C
  signatures are not yet supported. Foreign code controls the lifetime and
  validity of returned pointers. The C ABI is Linux x86-64 only.
- Units can export constants, types, variables and routines, with private
  implementation declarations and dependency-ordered initialization and
  reverse-order finalization. Unit files are found case-insensitively; cycles
  and ambiguous imported names are rejected when referenced without
  qualification. Dependencies are not implicitly
  re-exported. Qualified names in the form `UnitName.Symbol` are supported for
  exported variables, constants, types and routines.
  Interface `uses` dependencies are re-exported for name lookup and also visible
  for qualified names.
- A basic class-based GTK3 window, button and event callbacks example is in `examples/gtk_hello.pas`.
  With GTK3 installed, compile it with `--unit-path units --link -lgtk-3 --link -lgobject-2.0`;
  it reports an error instead of opening a window when no display is available.
  The `Gtk3` unit provides direct C declarations, while `GtkControls` supplies
  explicitly owned window and button wrappers with C callback registration.
  Free each wrapper exactly once; their GTK widget handles stay referenced until
  the wrapper is freed, even if a containing window has already been destroyed.
  Callbacks are program- or unit-scope routines, not bound object methods.
- A small VCL-like layer backed by GTK3 lives in `units/p64vcl.pas`, with a demo in
  `examples/vcl_hello.pas`. It provides `TApplication`, `TForm`, and `TButton` with
  `OnClick`/`OnClose` events.
- A stub C-ABI wrapper library lives in `simlib/` and is exposed to Pascal via
  `units/p64simlib.pas`, with a smoke-test program in `examples/simlib_smoke.pas`.
  This is scaffolding for wrapping a C++17 library (for example `rsimlib`) behind
  a stable C ABI.
  To build it against a local `rsimlib` checkout, configure with:
  `cmake -S . -B build -DP64_USE_RSIMLIB=ON -DP64_RSIMLIB_DIR=/home/dave/Documents/Projects/rsimlib`.
  A small windowed clear-screen loop example is in `examples/simlib_clear.pas`.
- `ReadLn` reads a single line from stdin into a `String`, `Integer`, `Int64`, or `Real`
  variable.
- `Write` and `WriteLn` accept integers, reals, strings and string literals. Comments use
  `{...}` or `(*...*)`; identifiers are case-insensitive.

Semantic analysis checks names, duplicate declarations, argument counts and
types, assignments and Boolean conditions. Integer values widen from 32 to 64
bits implicitly; narrowing is rejected. Integer literals use the smallest signed
type that can represent them. Boolean `and`/`or` use short-circuit evaluation. `/`
performs real division and yields a `Real` result; use `div` for integer division.
Integer overflow and division by zero are not checked.

Record field types, array element types and type aliases must refer to built-in
or previously declared types. Array bounds must fit signed 32-bit integers, and
the number of elements must not exceed 2147483647. Arrays use one index per
bracket pair. Recursive aggregate types, self-referential aliases, variant records
and record methods are not yet supported. Selectors apply to variables, not
directly to function calls.

Selectors on function call results are supported for field access and array indexing.
There are no
full units, Delphi-compatible lifecycle semantics or full Delphi-style exception semantics yet. Ordinary procedures
cannot be used as values (except for typed C callbacks), and function return values
may be discarded. The loop control variable must not be modified by the loop
body. String literals are interpreted as
NUL-terminated UTF-8 bytes. Diagnostics report source lines.

Basic exceptions are supported via `try .. except .. end`, `try .. finally .. end` and `raise`.
Only `raise` is catchable (runtime diagnostics still call `exit()`), and the runtime
and code generator include unwind cleanup for managed temporaries and locals.

## Architecture and roadmap

The frontend separates parsing into an AST, semantic analysis and LLVM IR
generation. Nested routines access enclosing locals through lexical frames.

Pascal64's official graphics API is `simlib` (via the optional `p64simlib` C ABI
wrapper). The long-term plan is to build Delphi-style UI and VCL-like layers on
top of it (and/or GTK3), but for games and immediate-mode rendering, `simlib` is
the primary target.

1. Establish the compiler pipeline and basic Pascal subset (done).
2. Expand the structured frontend, types and routines (in progress; additional
   Delphi type and language semantics remain).
3. Add C ABI external declarations, callbacks and Pascal units.
4. Bind GTK3 and build higher-level Delphi-like classes and event handling.
5. Add managed UTF-8 strings, classes, exceptions, ownership and debugging info.
