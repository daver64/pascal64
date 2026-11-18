unit P64Graphics;

{ High-level, Delphi-ish graphics helpers for Pascal64.

  This wraps `P64SimLib` (the official graphics API) into small classes and
  helper functions so programs don't need to deal with raw C ABI calls.

  Spelling is British English: Colour, not Color.
}

interface

type
  TColour = record
    R: Integer;
    G: Integer;
    B: Integer;
    A: Integer;
  end;

  TBitmap = class
    Handle: Pointer;
    constructor Create(width: Integer; height: Integer);
    destructor Destroy;

    function Width: Integer;
    function Height: Integer;

    { These flags are read when the GPU texture is first created.
      Set them before the first Upload/Draw for consistent behaviour. }
    procedure SetSmooth(enabled: Integer);
    procedure SetMipmaps(enabled: Integer);
    procedure SetRepeat(enabled: Integer);
    procedure SetAnisotropy(anisotropy: Real);

    procedure Acquire;
    procedure Release;
    procedure Clear(colour: TColour);
    procedure PutPixel(x: Integer; y: Integer; colour: TColour);
    function GetPixel(x: Integer; y: Integer): TColour;
    procedure Upload;
    procedure Download;
    function SavePNG(path: String): Integer;

    procedure Draw(x: Integer; y: Integer);
    procedure DrawStretched(x: Integer; y: Integer; width: Integer; height: Integer);
    procedure DrawRotated(center_x: Integer; center_y: Integer; angle_degrees: Real);
    procedure DrawEx(x: Real; y: Real; origin_x: Real; origin_y: Real; scale_x: Real; scale_y: Real; rotation_degrees: Real; flip_x: Boolean; flip_y: Boolean; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; tint: TColour);

  end;

  TSpriteBatch = class
    Handle: Pointer;
    constructor Create;
    destructor Destroy;
    procedure Clear;
    procedure Add(x: Real; y: Real; scale_x: Real; scale_y: Real; rotation_degrees: Real; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; tint: TColour);
    procedure Draw(bitmap: TBitmap);
  end;

  TFont = class
    Handle: Pointer;
    constructor Create(path: String; pointSize: Integer);
    destructor Destroy;
    function TextWidth(text: String): Integer;
    function TextHeight: Integer;
    procedure DrawText(x: Integer; y: Integer; colour: TColour; text: String);
  end;

  TFrameProc = procedure; cdecl;

  { Simple application loop helper.

    - Handles window close and Escape-to-quit.
    - Calls `FrameProc` once per frame.
    - Calls Present + EndFrame automatically.
  }
  TApplication = class
    Event: Pointer;
    Running: Integer;
    constructor Create;
    destructor Destroy;
    procedure PumpEvents;
    procedure Run(FrameProc: TFrameProc);
    procedure Terminate;
  end;

function ColourRGB(r: Integer; g: Integer; b: Integer): TColour;
function ColourRGBA(r: Integer; g: Integer; b: Integer; a: Integer): TColour;

function LoadBitmap(path: String): TBitmap;
function CreateRenderTarget(width: Integer; height: Integer): TBitmap;
function OpenMonospaceFont(pointSize: Integer): TFont;

procedure InitWindow(width: Integer; height: Integer; title: String);
procedure ShutdownWindow;

procedure Clear(colour: TColour);
procedure RectFill(left: Integer; top: Integer; right: Integer; bottom: Integer; colour: TColour);
procedure Rect(left: Integer; top: Integer; right: Integer; bottom: Integer; colour: TColour; thickness: Real);
procedure Line(x1: Integer; y1: Integer; x2: Integer; y2: Integer; colour: TColour);
procedure LineThick(x1: Integer; y1: Integer; x2: Integer; y2: Integer; colour: TColour; thickness: Real);
procedure Circle(x: Integer; y: Integer; radius: Integer; colour: TColour; thickness: Real);
procedure CircleFill(x: Integer; y: Integer; radius: Integer; colour: TColour);
procedure Ellipse(x: Integer; y: Integer; radius_x: Integer; radius_y: Integer; colour: TColour; thickness: Real);
procedure EllipseFill(x: Integer; y: Integer; radius_x: Integer; radius_y: Integer; colour: TColour);
procedure Triangle(x1: Integer; y1: Integer; x2: Integer; y2: Integer; x3: Integer; y3: Integer; colour: TColour; thickness: Real);
procedure TriangleFill(x1: Integer; y1: Integer; x2: Integer; y2: Integer; x3: Integer; y3: Integer; colour: TColour);
procedure Present;
procedure EndFrame;

procedure SetBlendModeNormal;
procedure SetBlendModeAdditive;
procedure SetBlendModeMultiply;
procedure SetBlendModeScreen;
procedure SetBlendModePremultiplied;

procedure SetClipRect(x: Integer; y: Integer; width: Integer; height: Integer);
procedure ClearClipRect;

function BeginRenderTarget(target: TBitmap): Integer;
procedure EndRenderTarget;
procedure ClearRenderTarget(colour: TColour);

procedure StartTextInput;
procedure StopTextInput;

function ScreenWidth: Integer;
function ScreenHeight: Integer;

function MouseX: Integer;
function MouseY: Integer;
function MouseButtons: Integer;
function MouseButtonLeftMask: Integer;
function MouseButtonMiddleMask: Integer;
function MouseButtonRightMask: Integer;

procedure SetMouseMapping(x: Integer; y: Integer; scale_x: Real; scale_y: Real);
procedure ClearMouseMapping;
function SetRelativeMouseMode(enabled: Integer): Integer;
function RelativeMouseMode: Integer;
procedure MouseDelta(var dx: Integer; var dy: Integer);

implementation

uses P64SimLib;

function ColourRGB(r: Integer; g: Integer; b: Integer): TColour;
begin
  Result.R := r;
  Result.G := g;
  Result.B := b;
  Result.A := 255;
end;

function ColourRGBA(r: Integer; g: Integer; b: Integer; a: Integer): TColour;
begin
  Result.R := r;
  Result.G := g;
  Result.B := b;
  Result.A := a;
end;

function LoadBitmap(path: String): TBitmap;
begin
  Result := TBitmap.Create(1, 1);
  if Result <> nil then
  begin
    if Result.Handle <> nil then
      SimLib_DestroyBitmap(Result.Handle);
    Result.Handle := SimLib_LoadBitmap(path);
  end;
end;

function CreateRenderTarget(width: Integer; height: Integer): TBitmap;
begin
  Result := TBitmap.Create(1, 1);
  if Result <> nil then
  begin
    if Result.Handle <> nil then
      SimLib_DestroyBitmap(Result.Handle);
    Result.Handle := SimLib_CreateRenderTarget(width, height);
  end;
end;

function OpenMonospaceFont(pointSize: Integer): TFont;
begin
  Result := TFont.Create('', pointSize);
end;

procedure InitWindow(width: Integer; height: Integer; title: String);
begin
  if SimLib_SetGfxMode(SimLib_GfxAutodetectWindowed(), width, height) = 0 then
  begin
    WriteLn('P64Graphics: set_gfx_mode failed');
    Exit;
  end;
  SimLib_SetWindowTitle(title);
end;

procedure ShutdownWindow;
begin
  SimLib_DisplayShutdown();
  SimLib_Shutdown();
end;

procedure Clear(colour: TColour);
begin
  SimLib_ClearToColour(colour.R, colour.G, colour.B, colour.A);
end;

procedure RectFill(left: Integer; top: Integer; right: Integer; bottom: Integer; colour: TColour);
begin
  SimLib_RectFill(left, top, right, bottom, colour.R, colour.G, colour.B, colour.A);
end;

procedure Rect(left: Integer; top: Integer; right: Integer; bottom: Integer; colour: TColour; thickness: Real);
begin
  SimLib_Rect(left, top, right, bottom, colour.R, colour.G, colour.B, colour.A, thickness);
end;

procedure Line(x1: Integer; y1: Integer; x2: Integer; y2: Integer; colour: TColour);
begin
  SimLib_Line(x1, y1, x2, y2, colour.R, colour.G, colour.B, colour.A);
end;

procedure LineThick(x1: Integer; y1: Integer; x2: Integer; y2: Integer; colour: TColour; thickness: Real);
begin
  SimLib_LineThick(x1, y1, x2, y2, colour.R, colour.G, colour.B, colour.A, thickness);
end;

procedure Circle(x: Integer; y: Integer; radius: Integer; colour: TColour; thickness: Real);
begin
  SimLib_Circle(x, y, radius, colour.R, colour.G, colour.B, colour.A, thickness);
end;

procedure CircleFill(x: Integer; y: Integer; radius: Integer; colour: TColour);
begin
  SimLib_CircleFill(x, y, radius, colour.R, colour.G, colour.B, colour.A);
end;

procedure Ellipse(x: Integer; y: Integer; radius_x: Integer; radius_y: Integer; colour: TColour; thickness: Real);
begin
  SimLib_Ellipse(x, y, radius_x, radius_y, colour.R, colour.G, colour.B, colour.A, thickness);
end;

procedure EllipseFill(x: Integer; y: Integer; radius_x: Integer; radius_y: Integer; colour: TColour);
begin
  SimLib_EllipseFill(x, y, radius_x, radius_y, colour.R, colour.G, colour.B, colour.A);
end;

procedure Triangle(x1: Integer; y1: Integer; x2: Integer; y2: Integer; x3: Integer; y3: Integer; colour: TColour; thickness: Real);
begin
  SimLib_Triangle(x1, y1, x2, y2, x3, y3, colour.R, colour.G, colour.B, colour.A, thickness);
end;

procedure TriangleFill(x1: Integer; y1: Integer; x2: Integer; y2: Integer; x3: Integer; y3: Integer; colour: TColour);
begin
  SimLib_TriangleFill(x1, y1, x2, y2, x3, y3, colour.R, colour.G, colour.B, colour.A);
end;

procedure Present;
begin
  SimLib_ShowVideoBitmap();
end;

procedure EndFrame;
begin
  SimLib_EndFrame();
end;

procedure SetBlendModeNormal;
begin
  SimLib_SetBlendMode(SimLib_BlendModeNormal());
end;

procedure SetBlendModeAdditive;
begin
  SimLib_SetBlendMode(SimLib_BlendModeAdditive());
end;

procedure SetBlendModeMultiply;
begin
  SimLib_SetBlendMode(SimLib_BlendModeMultiply());
end;

procedure SetBlendModeScreen;
begin
  SimLib_SetBlendMode(SimLib_BlendModeScreen());
end;

procedure SetBlendModePremultiplied;
begin
  SimLib_SetBlendMode(SimLib_BlendModePremultiplied());
end;

procedure SetClipRect(x: Integer; y: Integer; width: Integer; height: Integer);
begin
  SimLib_SetClipRect(x, y, width, height);
end;

procedure ClearClipRect;
begin
  SimLib_ClearClipRect();
end;

function BeginRenderTarget(target: TBitmap): Integer;
begin
  if (target = nil) or (target.Handle = nil) then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_BeginRenderTarget(target.Handle);
end;

procedure EndRenderTarget;
begin
  SimLib_EndRenderTarget();
end;

procedure ClearRenderTarget(colour: TColour);
begin
  SimLib_ClearRenderTarget(colour.R, colour.G, colour.B, colour.A);
end;

procedure StartTextInput;
begin
  SimLib_StartTextInput();
end;

procedure StopTextInput;
begin
  SimLib_StopTextInput();
end;

function ScreenWidth: Integer;
begin
  Result := SimLib_ScreenWidth();
end;

function ScreenHeight: Integer;
begin
  Result := SimLib_ScreenHeight();
end;

function MouseX: Integer;
begin
  Result := SimLib_MouseX();
end;

function MouseY: Integer;
begin
  Result := SimLib_MouseY();
end;

function MouseButtons: Integer;
begin
  Result := SimLib_MouseButtons();
end;

function MouseButtonLeftMask: Integer;
begin
  Result := SimLib_MouseButtonLeftMask();
end;

function MouseButtonMiddleMask: Integer;
begin
  Result := SimLib_MouseButtonMiddleMask();
end;

function MouseButtonRightMask: Integer;
begin
  Result := SimLib_MouseButtonRightMask();
end;

procedure SetMouseMapping(x: Integer; y: Integer; scale_x: Real; scale_y: Real);
begin
  SimLib_SetMouseMapping(x, y, scale_x, scale_y);
end;

procedure ClearMouseMapping;
begin
  SimLib_ClearMouseMapping();
end;

function SetRelativeMouseMode(enabled: Integer): Integer;
begin
  Result := SimLib_SetRelativeMouseMode(enabled);
end;

function RelativeMouseMode: Integer;
begin
  Result := SimLib_RelativeMouseMode();
end;

procedure MouseDelta(var dx: Integer; var dy: Integer);
begin
  SimLib_MouseDelta(dx, dy);
end;

constructor TApplication.Create;
begin
  Self.Event := SimLib_EventCreate();
  Self.Running := 1;
end;

destructor TApplication.Destroy;
begin
  if Self.Event <> nil then
  begin
    SimLib_EventDestroy(Self.Event);
    Self.Event := nil;
  end;
end;

procedure TApplication.PumpEvents;
begin
  if Self.Event = nil then
    Exit;
  while SimLib_PollEvent(Self.Event) <> 0 do
  begin
    if SimLib_EventType(Self.Event) = SimLib_EventTypeQuit() then
      Self.Running := 0;
    SimLib_DisplayHandleEvent(Self.Event);
  end;
  if SimLib_KeyDownScancode(SimLib_ScancodeEscape()) <> 0 then
    Self.Running := 0;
end;

procedure TApplication.Run(FrameProc: TFrameProc);
begin
  Self.Running := 1;
  while Self.Running <> 0 do
  begin
    Self.PumpEvents;
    if Self.Running = 0 then
      break;
    if FrameProc <> nil then
      FrameProc;
    Present;
    EndFrame;
  end;
end;

procedure TApplication.Terminate;
begin
  Self.Running := 0;
end;

constructor TBitmap.Create(width: Integer; height: Integer);
begin
  Self.Handle := SimLib_CreateBitmap(width, height);
end;

destructor TBitmap.Destroy;
begin
  if Self.Handle <> nil then
  begin
    SimLib_DestroyBitmap(Self.Handle);
    Self.Handle := nil;
  end;
end;

function TBitmap.Width: Integer;
begin
  if Self.Handle = nil then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_BitmapWidth(Self.Handle);
end;

function TBitmap.Height: Integer;
begin
  if Self.Handle = nil then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_BitmapHeight(Self.Handle);
end;

procedure TBitmap.SetSmooth(enabled: Integer);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_BitmapSetSmooth(Self.Handle, enabled);
end;

procedure TBitmap.SetMipmaps(enabled: Integer);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_BitmapSetMipmaps(Self.Handle, enabled);
end;

procedure TBitmap.SetRepeat(enabled: Integer);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_BitmapSetRepeat(Self.Handle, enabled);
end;

procedure TBitmap.SetAnisotropy(anisotropy: Real);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_BitmapSetAnisotropy(Self.Handle, anisotropy);
end;

procedure TBitmap.Acquire;
begin
  if Self.Handle = nil then
    Exit;
  SimLib_AcquireBitmap(Self.Handle);
end;

procedure TBitmap.Release;
begin
  if Self.Handle = nil then
    Exit;
  SimLib_ReleaseBitmap(Self.Handle);
end;

procedure TBitmap.Clear(colour: TColour);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_ClearBitmap(Self.Handle, colour.R, colour.G, colour.B, colour.A);
end;

procedure TBitmap.PutPixel(x: Integer; y: Integer; colour: TColour);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_PutPixel(Self.Handle, x, y, colour.R, colour.G, colour.B, colour.A);
end;

function TBitmap.GetPixel(x: Integer; y: Integer): TColour;
var r: Integer; g: Integer; b: Integer; a: Integer;
begin
  Result := ColourRGBA(0, 0, 0, 0);
  if Self.Handle = nil then
    Exit;
  r := 0; g := 0; b := 0; a := 0;
  SimLib_GetPixel(Self.Handle, x, y, r, g, b, a);
  Result := ColourRGBA(r, g, b, a);
end;

procedure TBitmap.Upload;
begin
  if Self.Handle = nil then
    Exit;
  SimLib_UploadBitmap(Self.Handle);
end;

procedure TBitmap.Download;
begin
  if Self.Handle = nil then
    Exit;
  SimLib_DownloadBitmap(Self.Handle);
end;

function TBitmap.SavePNG(path: String): Integer;
begin
  if Self.Handle = nil then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_SaveBitmap(Self.Handle, path);
end;

procedure TBitmap.Draw(x: Integer; y: Integer);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_DrawSprite(Self.Handle, x, y);
end;

procedure TBitmap.DrawStretched(x: Integer; y: Integer; width: Integer; height: Integer);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_DrawSpriteStretched(Self.Handle, x, y, width, height);
end;

procedure TBitmap.DrawRotated(center_x: Integer; center_y: Integer; angle_degrees: Real);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_DrawSpriteRotated(Self.Handle, center_x, center_y, angle_degrees);
end;

procedure TBitmap.DrawEx(x: Real; y: Real; origin_x: Real; origin_y: Real; scale_x: Real; scale_y: Real; rotation_degrees: Real; flip_x: Boolean; flip_y: Boolean; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; tint: TColour);
var fx: Integer; fy: Integer;
begin
  if Self.Handle = nil then
    Exit;
  if flip_x then fx := 1 else fx := 0;
  if flip_y then fy := 1 else fy := 0;
  SimLib_DrawSpriteEx(Self.Handle, x, y, origin_x, origin_y, scale_x, scale_y, rotation_degrees, fx, fy, source_x, source_y, source_width, source_height, tint.R, tint.G, tint.B, tint.A);
end;

constructor TSpriteBatch.Create;
begin
  Self.Handle := SimLib_SpriteBatchCreate();
end;

destructor TSpriteBatch.Destroy;
begin
  if Self.Handle <> nil then
  begin
    SimLib_SpriteBatchDestroy(Self.Handle);
    Self.Handle := nil;
  end;
end;

procedure TSpriteBatch.Clear;
begin
  if Self.Handle = nil then
    Exit;
  SimLib_SpriteBatchClear(Self.Handle);
end;

procedure TSpriteBatch.Add(x: Real; y: Real; scale_x: Real; scale_y: Real; rotation_degrees: Real; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; tint: TColour);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_SpriteBatchAdd(Self.Handle, x, y, scale_x, scale_y, rotation_degrees, source_x, source_y, source_width, source_height, tint.R, tint.G, tint.B, tint.A);
end;

procedure TSpriteBatch.Draw(bitmap: TBitmap);
begin
  if (Self.Handle = nil) or (bitmap = nil) then
    Exit;
  SimLib_SpriteBatchDraw(bitmap.Handle, Self.Handle);
end;

constructor TFont.Create(path: String; pointSize: Integer);
begin
  if path = '' then
    Self.Handle := SimLib_OpenMonospaceFont(pointSize)
  else
    Self.Handle := SimLib_OpenFont(path, pointSize);
end;

destructor TFont.Destroy;
begin
  if Self.Handle <> nil then
  begin
    SimLib_CloseFont(Self.Handle);
    Self.Handle := nil;
  end;
end;

function TFont.TextWidth(text: String): Integer;
begin
  if Self.Handle = nil then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_TextLength(Self.Handle, text);
end;

function TFont.TextHeight: Integer;
begin
  if Self.Handle = nil then
  begin
    Result := 0;
    Exit;
  end;
  Result := SimLib_TextHeight(Self.Handle);
end;

procedure TFont.DrawText(x: Integer; y: Integer; colour: TColour; text: String);
begin
  if Self.Handle = nil then
    Exit;
  SimLib_TextOut(Self.Handle, x, y, colour.R, colour.G, colour.B, colour.A, text);
end;

end.

