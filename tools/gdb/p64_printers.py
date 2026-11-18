"""GDB pretty-printers for Pascal64.

Usage (inside gdb):

  (gdb) source /path/to/pascal64/tools/gdb/p64_printers.py

This provides a basic pretty-printer for Pascal64 managed `String` values.
"""

from __future__ import annotations

import gdb


class Pascal64StringPrinter:
    """Pretty-printer for Pascal64 managed String.

    In DWARF, Pascal64 `String` is a pointer to a struct with:
      - references: size_t
      - length: size_t
      - bytes: char*
    """

    def __init__(self, value: gdb.Value):
        self.value = value

    def to_string(self) -> str:
        try:
            if int(self.value) == 0:
                return "''"
            obj = self.value.dereference()
            length = int(obj["length"])
            data_ptr = obj["bytes"]
            if int(data_ptr) == 0 or length == 0:
                return "''"
            # Read bytes from inferior memory.
            mem = gdb.selected_inferior().read_memory(int(data_ptr), length)
            raw = bytes(mem)
            try:
                text = raw.decode("utf-8", errors="replace")
            except Exception:
                text = raw.decode("latin-1", errors="replace")
            return "'" + text.replace("'", "''") + "'"
        except Exception:
            # Fallback: show pointer.
            return str(self.value)

    def display_hint(self) -> str:
        return "string"


def lookup_pascal64_printer(value: gdb.Value):
    try:
        t = value.type
        if t.code != gdb.TYPE_CODE_PTR:
            return None
        target = t.target().strip_typedefs()
        if target.code != gdb.TYPE_CODE_STRUCT:
            return None
        if target.tag != "String":
            return None
        return Pascal64StringPrinter(value)
    except Exception:
        return None


def register_printers(obj=None):
    if obj is None:
        obj = gdb
    obj.pretty_printers.append(lookup_pascal64_printer)


register_printers()

