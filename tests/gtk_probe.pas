program GtkProbe;
function gtk_get_major_version: Integer; cdecl; external;
begin
  WriteLn(gtk_get_major_version);
end.
