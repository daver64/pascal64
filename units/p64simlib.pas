unit P64SimLib;

{ Pascal declarations for the optional `p64simlib` C-ABI wrapper.

  This is intended to wrap a C++17 library (rsimlib / simlib) behind a stable C
  ABI that Pascal64 can call via `cdecl; external`.

  For now, the C++ side is a small stub so we can validate the toolchain end-to-
  end.
}

interface

type
  PSimLibContext = Pointer;
  PSimLibEvent = Pointer;
  PSimLibBitmap = Pointer;

{ NOTE: Pascal64 reserves external symbol names starting with "p64_".
  The C ABI uses the "p64simlib_" prefix instead.
}
function SimLib_Create: PSimLibContext; cdecl; external name 'p64simlib_create';
procedure SimLib_Destroy(ctx: PSimLibContext); cdecl; external name 'p64simlib_destroy';
function SimLib_Backend: PChar; cdecl; external name 'p64simlib_backend';
function SimLib_Hello(ctx: PSimLibContext; message: PChar): Integer; cdecl; external name 'p64simlib_hello';

function SimLib_SetGfxMode(driver: Integer; width: Integer; height: Integer): Integer; cdecl; external name 'p64simlib_set_gfx_mode';
procedure SimLib_DisplayHandleEvent(ev: PSimLibEvent); cdecl; external name 'p64simlib_display_handle_event';
procedure SimLib_SetWindowTitle(title: PChar); cdecl; external name 'p64simlib_set_window_title';
procedure SimLib_ClearToColour(red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_clear_to_colour';
procedure SimLib_ShowVideoBitmap; cdecl; external name 'p64simlib_show_video_bitmap';
procedure SimLib_EndFrame; cdecl; external name 'p64simlib_end_frame';
procedure SimLib_DisplayShutdown; cdecl; external name 'p64simlib_display_shutdown';
procedure SimLib_Shutdown; cdecl; external name 'p64simlib_shutdown';
function SimLib_TimeMs: Int64; cdecl; external name 'p64simlib_time_ms';
procedure SimLib_Rest(milliseconds: Integer); cdecl; external name 'p64simlib_rest';
function SimLib_ScreenWidth: Integer; cdecl; external name 'p64simlib_screen_width';
function SimLib_ScreenHeight: Integer; cdecl; external name 'p64simlib_screen_height';

function SimLib_GfxAutodetectWindowed: Integer; cdecl; external name 'p64simlib_gfx_autodetect_windowed';
function SimLib_GfxAutodetectFullscreen: Integer; cdecl; external name 'p64simlib_gfx_autodetect_fullscreen';

function SimLib_EventCreate: PSimLibEvent; cdecl; external name 'p64simlib_event_create';
procedure SimLib_EventDestroy(ev: PSimLibEvent); cdecl; external name 'p64simlib_event_destroy';
function SimLib_PollEvent(ev: PSimLibEvent): Integer; cdecl; external name 'p64simlib_poll_event';
function SimLib_EventType(ev: PSimLibEvent): Integer; cdecl; external name 'p64simlib_event_type';
function SimLib_EventKey(ev: PSimLibEvent): Integer; cdecl; external name 'p64simlib_event_key';
function SimLib_EventWindowWidth(ev: PSimLibEvent): Integer; cdecl; external name 'p64simlib_event_window_width';
function SimLib_EventWindowHeight(ev: PSimLibEvent): Integer; cdecl; external name 'p64simlib_event_window_height';

function SimLib_KeyDownScancode(scancode: Integer): Integer; cdecl; external name 'p64simlib_key_down_scancode';
function SimLib_MouseX: Integer; cdecl; external name 'p64simlib_mouse_x';
function SimLib_MouseY: Integer; cdecl; external name 'p64simlib_mouse_y';
function SimLib_MouseButtons: Integer; cdecl; external name 'p64simlib_mouse_buttons';

function SimLib_ScancodeEscape: Integer; cdecl; external name 'p64simlib_scancode_escape';
function SimLib_ScancodeLeft: Integer; cdecl; external name 'p64simlib_scancode_left';
function SimLib_ScancodeRight: Integer; cdecl; external name 'p64simlib_scancode_right';
function SimLib_ScancodeUp: Integer; cdecl; external name 'p64simlib_scancode_up';
function SimLib_ScancodeDown: Integer; cdecl; external name 'p64simlib_scancode_down';

procedure SimLib_RectFill(left: Integer; top: Integer; right: Integer; bottom: Integer; red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_rectfill';
procedure SimLib_Line(x1: Integer; y1: Integer; x2: Integer; y2: Integer; red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_line';
procedure SimLib_CircleFill(x: Integer; y: Integer; radius: Integer; red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_circlefill';

procedure SimLib_SetBlendMode(mode: Integer); cdecl; external name 'p64simlib_set_blend_mode';
function SimLib_BlendModeNormal: Integer; cdecl; external name 'p64simlib_blend_mode_normal';
function SimLib_BlendModeAdditive: Integer; cdecl; external name 'p64simlib_blend_mode_additive';
function SimLib_BlendModeMultiply: Integer; cdecl; external name 'p64simlib_blend_mode_multiply';
function SimLib_BlendModeScreen: Integer; cdecl; external name 'p64simlib_blend_mode_screen';
function SimLib_BlendModePremultiplied: Integer; cdecl; external name 'p64simlib_blend_mode_premultiplied';

procedure SimLib_SetClipRect(x: Integer; y: Integer; width: Integer; height: Integer); cdecl; external name 'p64simlib_set_clip_rect';
procedure SimLib_ClearClipRect; cdecl; external name 'p64simlib_clear_clip_rect';

function SimLib_CreateBitmap(width: Integer; height: Integer): PSimLibBitmap; cdecl; external name 'p64simlib_create_bitmap';
function SimLib_CreateRenderTarget(width: Integer; height: Integer): PSimLibBitmap; cdecl; external name 'p64simlib_create_render_target';
procedure SimLib_DestroyBitmap(bitmap: PSimLibBitmap); cdecl; external name 'p64simlib_destroy_bitmap';
procedure SimLib_PutPixel(bitmap: PSimLibBitmap; x: Integer; y: Integer; red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_putpixel';
function SimLib_UploadBitmap(bitmap: PSimLibBitmap): Integer; cdecl; external name 'p64simlib_upload_bitmap';
function SimLib_LoadBitmap(path: PChar): PSimLibBitmap; cdecl; external name 'p64simlib_load_bitmap';
procedure SimLib_DrawSprite(bitmap: PSimLibBitmap; x: Integer; y: Integer); cdecl; external name 'p64simlib_draw_sprite';
procedure SimLib_DrawSpriteStretched(bitmap: PSimLibBitmap; x: Integer; y: Integer; width: Integer; height: Integer); cdecl; external name 'p64simlib_draw_sprite_stretched';
procedure SimLib_DrawSpriteRotated(bitmap: PSimLibBitmap; center_x: Integer; center_y: Integer; angle_degrees: Real); cdecl; external name 'p64simlib_draw_sprite_rotated';
procedure SimLib_DrawSpriteRotatedStretched(bitmap: PSimLibBitmap; center_x: Integer; center_y: Integer; angle_degrees: Real; width: Integer; height: Integer); cdecl; external name 'p64simlib_draw_sprite_rotated_stretched';
procedure SimLib_DrawSpriteRegionFlipped(bitmap: PSimLibBitmap; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; x: Integer; y: Integer; destination_width: Integer; destination_height: Integer; flip_horizontal: Integer; flip_vertical: Integer; flip_diagonal: Integer; tint_red: Integer; tint_green: Integer; tint_blue: Integer; tint_alpha: Integer); cdecl; external name 'p64simlib_draw_sprite_region_flipped';
procedure SimLib_DrawSpriteEx(bitmap: PSimLibBitmap; x: Real; y: Real; origin_x: Real; origin_y: Real; scale_x: Real; scale_y: Real; rotation_degrees: Real; flip_x: Integer; flip_y: Integer; source_x: Integer; source_y: Integer; source_width: Integer; source_height: Integer; tint_red: Integer; tint_green: Integer; tint_blue: Integer; tint_alpha: Integer); cdecl; external name 'p64simlib_draw_sprite_ex';

function SimLib_BeginRenderTarget(target: PSimLibBitmap): Integer; cdecl; external name 'p64simlib_begin_render_target';
procedure SimLib_EndRenderTarget; cdecl; external name 'p64simlib_end_render_target';
procedure SimLib_ClearRenderTarget(red: Integer; green: Integer; blue: Integer; alpha: Integer); cdecl; external name 'p64simlib_clear_render_target';

{ Event types (matches `sl::Event::Type`). }
function SimLib_EventQuit: Integer;

implementation

function SimLib_EventQuit: Integer;
begin
  Result := 2;
end;

end.

