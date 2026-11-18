program SimLibSmoke;

uses P64Std, P64SimLib;

var
  ctx: PSimLibContext;
begin
  WriteLn('SimLib backend: ', StrFromPChar(SimLib_Backend()));
  ctx := SimLib_Create();
  SimLib_Hello(ctx, 'from Pascal64');
  SimLib_Destroy(ctx);
end.

