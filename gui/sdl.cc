/////////////////////////////////////////////////////////////////////////
// $Id: svga.cc 11501 2012-10-14 18:29:44Z vruppert $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009-2012  The Bochs Project
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

#define _MULTI_THREAD

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "bochs.h"
#include "param_names.h"
#include "iodev.h"
#if BX_WITH_SDL

#include <stdlib.h>

extern "C" {
    void vgajs_setpalette(int index, int red, int green, int blue);
    void vgajs_dimension_update(int x, int y, int bpp);
    void vgajs_tile_update_in_place(int x, int y, int x_tilesize, int y_tilesize, Bit32u * subtile, int pitch);
    void vgajs_tile_update(int x, int y, int x_tilesize, int y_tilesize, unsigned char * snapshot, int bpp);
    void vgajs_clear_screen();
    void kbdjs_keydown(int jskeycode);
    void kbdjs_keyup(int jskeycode);
    void mousejs_motion(int dx, int dy, int dz, unsigned mouse_button_state, bx_bool a);
};

class bx_sdl_gui_c : public bx_gui_c {
public:
  bx_sdl_gui_c (void);
  DECLARE_GUI_VIRTUAL_METHODS()
  DECLARE_GUI_NEW_VIRTUAL_METHODS()
  virtual void set_display_mode (disp_mode_t newmode);
};

// declare one instance of the gui object and call macro to insert the
// plugin code
static bx_sdl_gui_c *theGui = NULL;

IMPLEMENT_GUI_PLUGIN_CODE(sdl)

#define LOG_THIS theGui->

static unsigned disp_bpp=8;
static unsigned res_x, res_y;
int fontwidth = 8, fontheight = 16;
static unsigned char vgafont[256 * 16];
static int clut8 = 0;
static int save_vga_mode;
static int save_vga_pal[256 * 3];
static Bit8u fontbuffer[0x2000];
Bit8u h_panning = 0, v_panning = 0;
Bit16u line_compare = 1023;

static bx_bool ctrll_pressed = 0;
static unsigned int text_rows=25, text_cols=80;
static unsigned prev_cursor_x=0;
static unsigned prev_cursor_y=0;
Bit32u *screen;
bx_bool *tile_updated;


static Bit32u sdl_sym_to_bx_key (Bit32u sym)
{
  switch (sym)
  {
    case 8:            return BX_KEY_BACKSPACE;
    case 9:                  return BX_KEY_TAB;
    case 13:               return BX_KEY_ENTER;
    case 19:                return BX_KEY_PAUSE;
    case 27:               return BX_KEY_ESC;
    case 32:                return BX_KEY_SPACE;
    case 222:                return BX_KEY_SINGLE_QUOTE;
    case 188:                return BX_KEY_COMMA;
    case 189:                return BX_KEY_MINUS;
    case 190:               return BX_KEY_PERIOD;
    case 191:                return BX_KEY_SLASH;
    case 49:                    return BX_KEY_1;
    case 50:                    return BX_KEY_2;
    case 51:                    return BX_KEY_3;
    case 52:                    return BX_KEY_4;
    case 53:                    return BX_KEY_5;
    case 54:                    return BX_KEY_6;
    case 55:                    return BX_KEY_7;
    case 56:                    return BX_KEY_8;
    case 57:                    return BX_KEY_9;

    case 186:            return BX_KEY_SEMICOLON;
    case 187:               return BX_KEY_EQUALS;
/*
 Skip uppercase letters
*/
    case 219:          return BX_KEY_LEFT_BRACKET;
    case 220:            return BX_KEY_BACKSLASH;
    case 221:         return BX_KEY_RIGHT_BRACKET;
    case 192:            return BX_KEY_GRAVE;
    case 65:                    return BX_KEY_A;
    case 66:                    return BX_KEY_B;
    case 67:                    return BX_KEY_C;
    case 68:                    return BX_KEY_D;
    case 69:                    return BX_KEY_E;
    case 70:                    return BX_KEY_F;
    case 71:                    return BX_KEY_G;
    case 72:                    return BX_KEY_H;
    case 73:                    return BX_KEY_I;
    case 74:                    return BX_KEY_J;
    case 75:                    return BX_KEY_K;
    case 76:                    return BX_KEY_L;
    case 77:                    return BX_KEY_M;
    case 78:                    return BX_KEY_N;
    case 79:                    return BX_KEY_O;
    case 80:                    return BX_KEY_P;
    case 81:                    return BX_KEY_Q;
    case 82:                    return BX_KEY_R;
    case 83:                    return BX_KEY_S;
    case 84:                    return BX_KEY_T;
    case 85:                    return BX_KEY_U;
    case 86:                    return BX_KEY_V;
    case 87:                    return BX_KEY_W;
    case 88:                    return BX_KEY_X;
    case 89:                    return BX_KEY_Y;
    case 90:                    return BX_KEY_Z;
/* End of ASCII mapped keysyms */
    case 46:               return BX_KEY_DELETE;

/* Numeric keypad */
    case 96:                    return BX_KEY_0;
    case 97:                    return BX_KEY_1;
    case 98:                    return BX_KEY_2;
    case 99:                    return BX_KEY_3;
    case 100:                    return BX_KEY_4;
    case 101:                    return BX_KEY_5;
    case 102:                    return BX_KEY_6;
    case 103:                    return BX_KEY_7;
    case 104:                    return BX_KEY_8;
    case 105:                    return BX_KEY_9;
    //case 45:                  return BX_KEY_KP_INSERT;
    //case 36:                  return BX_KEY_KP_HOME;
    //case 35:                  return BX_KEY_KP_END;
    //case 34:                  return BX_KEY_KP_PAGE_DOWN;
    //case 40:                  return BX_KEY_KP_DOWN;
    //case 37:                  return BX_KEY_KP_LEFT;
    //case 39:                  return BX_KEY_KP_RIGHT;
    //case 38:                  return BX_KEY_KP_UP;
    //case 33:                  return BX_KEY_KP_PAGE_UP;
    //case 46:            return BX_KEY_KP_DELETE;
    case 111:            return BX_KEY_KP_DIVIDE;
    case 106:          return BX_KEY_KP_MULTIPLY;
    case 109:             return BX_KEY_KP_SUBTRACT;
    case 107:              return BX_KEY_KP_ADD;
    //case 13:             return BX_KEY_KP_ENTER;

/* Arrows + Home/End pad */
    case 38:                   return BX_KEY_UP;
    case 40:                 return BX_KEY_DOWN;
    case 39:                return BX_KEY_RIGHT;
    case 37:                 return BX_KEY_LEFT;
    case 45:               return BX_KEY_INSERT;
    case 36:                 return BX_KEY_HOME;
    case 35:                  return BX_KEY_END;
    case 33:               return BX_KEY_PAGE_UP;
    case 34:             return BX_KEY_PAGE_DOWN;

/* Function keys */
    case 112:                   return BX_KEY_F1;
    case 113:                   return BX_KEY_F2;
    case 114:                   return BX_KEY_F3;
    case 115:                   return BX_KEY_F4;
    case 116:                   return BX_KEY_F5;
    case 117:                   return BX_KEY_F6;
    case 118:                   return BX_KEY_F7;
    case 119:                   return BX_KEY_F8;
    case 120:                   return BX_KEY_F9;
    case 121:                  return BX_KEY_F10;
    case 122:                  return BX_KEY_F11;
    case 123:                  return BX_KEY_F12;

/* Key state modifier keys */
    case 144:              return BX_KEY_NUM_LOCK;
    case 20:             return BX_KEY_CAPS_LOCK;
    case 145:            return BX_KEY_SCRL_LOCK;
    case 16:               return BX_KEY_SHIFT_R;
    //case 16:               return BX_KEY_SHIFT_L;
    case 17:                return BX_KEY_CTRL_R;
    //case 17:                return BX_KEY_CTRL_L;
    case 18:                 return BX_KEY_ALT_R;
    //case 18:                 return BX_KEY_ALT_L;
    //case 18:                return BX_KEY_ALT_R;
/*
    case :                return BX_KEY_WIN_L;
    case :               return BX_KEY_WIN_L;
    case :               return BX_KEY_WIN_R;
*/

/* Miscellaneous function keys */
/*
    case :                return BX_KEY_PRINT;
*/
    //case 19:                return BX_KEY_PAUSE;
    case 93:                 return BX_KEY_MENU;
    default:
      BX_ERROR(("sdl keysym %d not mapped", (int)sym));
      return BX_KEY_UNHANDLED;
  }
}

bx_sdl_gui_c::bx_sdl_gui_c()
{
  put("SVGA");
}

void bx_sdl_gui_c::specific_init(
    int argc,
    char **argv,
    unsigned header_bar_y)
{
//  new_gfx_api = 0;
  screen = new Bit32u[guest_xres * guest_yres];
  /*for (int tmpx = 0; tmpx < guest_xres; tmpx++) {
    for (int tmpy = 0; tmpy < guest_yres; tmpy++) {
        screen[tmpy*guest_xres + tmpx] = 255 | 255 << 8;
    }
  }*/
  tile_updated = new bx_bool[(guest_xres/x_tilesize +1) * (guest_yres/y_tilesize +1)];
}

int bx_sdl_gui_c::get_clipboard_text(Bit8u **bytes, Bit32s *nbytes)
{
  return 0;
}

int bx_sdl_gui_c::set_clipboard_text(char *text_snapshot, Bit32u len)
{
  return 0;
}

void bx_sdl_gui_c::handle_events(void)
{
}

void bx_sdl_gui_c::text_update(Bit8u *old_text, Bit8u *new_text,
    unsigned long cursor_x,
    unsigned long cursor_y,
    bx_vga_tminfo_t *tm_info)
{
  Bit8u *pfont_row, *old_line, *new_line, *text_base;
  unsigned int cs_y, i, x, y;
  unsigned int curs, hchars, offset;
  Bit8u fontline, fontpixels, fontrows;
  int rows;
  Bit32u fgcolor, bgcolor;
  Bit32u *buf, *buf_row, *buf_char;
  Bit32u disp;
  Bit16u font_row, mask;
  Bit8u cfstart, cfwidth, cfheight, split_fontrows, split_textrow;
  bx_bool cursor_visible, gfxcharw9, invert, forceUpdate, split_screen;
  bx_bool blink_mode, blink_state;
  Bit32u text_palette[16];

  forceUpdate = 0;
  blink_mode = (tm_info->blink_flags & BX_TEXT_BLINK_MODE) > 0;
  blink_state = (tm_info->blink_flags & BX_TEXT_BLINK_STATE) > 0;
  if (blink_mode) {
    if (tm_info->blink_flags & BX_TEXT_BLINK_TOGGLE)
      forceUpdate = 1;
  }
  if (charmap_updated) {
    forceUpdate = 1;
    charmap_updated = 0;
  }
  for (i=0; i<16; i++) {
    text_palette[i] = palette[tm_info->actl_palette[i]].red | palette[tm_info->actl_palette[i]].green << 8 | palette[tm_info->actl_palette[i]].blue << 16;
  }
  if ((tm_info->h_panning != h_panning) || (tm_info->v_panning != v_panning)) {
    forceUpdate = 1;
    h_panning = tm_info->h_panning;
    v_panning = tm_info->v_panning;
  }
  if (tm_info->line_compare != line_compare) {
    forceUpdate = 1;
    line_compare = tm_info->line_compare;
  }
  disp = res_x;
  buf_row = screen;
  // first invalidate character at previous and new cursor location
  if ((prev_cursor_y < text_rows) && (prev_cursor_x < text_cols)) {
    curs = prev_cursor_y * tm_info->line_offset + prev_cursor_x * 2;
    old_text[curs] = ~new_text[curs];
  }
  cursor_visible = ((tm_info->cs_start <= tm_info->cs_end) && (tm_info->cs_start < fontheight));
  if((cursor_visible) && (cursor_y < text_rows) && (cursor_x < text_cols)) {
    curs = cursor_y * tm_info->line_offset + cursor_x * 2;
    old_text[curs] = ~new_text[curs];
  } else {
    curs = 0xffff;
  }

  rows = text_rows;
  if (v_panning) rows++;
  y = 0;
  cs_y = 0;
  text_base = new_text - tm_info->start_address;
  if (line_compare < res_y) {
    split_textrow = (line_compare + v_panning) / fontheight;
    split_fontrows = ((line_compare + v_panning) % fontheight) + 1;
  } else {
    split_textrow = rows + 1;
    split_fontrows = 0;
  }
  split_screen = 0;

  do {
    buf = buf_row;
    hchars = text_cols;
    if (h_panning) hchars++;
    cfheight = fontheight;
    cfstart = 0;
    if (split_screen)
    {
      if (rows == 1)
      {
        cfheight = (res_y - line_compare - 1) % fontheight;
        if (cfheight == 0) cfheight = fontheight;
      }
    }
    else if (v_panning)
    {
      if (y == 0)
      {
        cfheight -= v_panning;
        cfstart = v_panning;
      }
      else if (rows == 1)
      {
        cfheight = v_panning;
      }
    }
    if (!split_screen && (y == split_textrow))
    {
      if ((split_fontrows - cfstart) < cfheight)
      {
        cfheight = split_fontrows - cfstart;
      }
    }
    new_line = new_text;
    old_line = old_text;
    x = 0;
    offset = cs_y * tm_info->line_offset;
    do
    {
      cfwidth = fontwidth;
      if (h_panning)
      {
        if (hchars > text_cols)
        {
          cfwidth -= h_panning;
        }
        else if (hchars == 1)
        {
          cfwidth = h_panning;
        }
      }
      // check if char needs to be updated
      if(forceUpdate || (old_text[0] != new_text[0])
         || (old_text[1] != new_text[1])) {
        // Get Foreground/Background pixel colors
        fgcolor = text_palette[new_text[1] & 0x0F];
        if (blink_mode) {
          bgcolor = text_palette[(new_text[1] >> 4) & 0x07];
          if (!blink_state && (new_text[1] & 0x80))
            fgcolor = bgcolor;
        } else {
          bgcolor = text_palette[(new_text[1] >> 4) & 0x0F];
        }
	invert = ((offset == curs) && (cursor_visible));
	gfxcharw9 = ((tm_info->line_graphics) && ((new_text[0] & 0xE0) == 0xC0));

	// Display this one char
	fontrows = cfheight;
	fontline = cfstart;
	if (y > 0)
	{
	  pfont_row = &vga_charmap[(new_text[0] << 5)];
	}
	else
	{
	  pfont_row = &vga_charmap[(new_text[0] << 5) + cfstart];
	}
	buf_char = buf;
	do
	{
	  font_row = *pfont_row++;
	  if (gfxcharw9)
	  {
	    font_row = (font_row << 1) | (font_row & 0x01);
	  }
	  else
	  {
	    font_row <<= 1;
	  }
	  if (hchars > text_cols)
	  {
	    font_row <<= h_panning;
	  }
	  fontpixels = cfwidth;
	  if ((invert) && (fontline >= tm_info->cs_start) && (fontline <= tm_info->cs_end))
	    mask = 0x100;
	  else
	    mask = 0x00;
	  do
	  {
	    if ((font_row & 0x100) == mask) {
          if (*buf != bgcolor) {

          }
	      *buf = bgcolor;
	    } else {
	      *buf = fgcolor;
        }
	    buf++;
	    font_row <<= 1;
	  } while(--fontpixels);
	  buf -= cfwidth;
	  buf += disp;
	  fontline++;
	} while(--fontrows);

	// restore output buffer ptr to start of this char
	buf = buf_char;
      }
      // move to next char location on screen
      buf += cfwidth;

      // select next char in old/new text
      new_text+=2;
      old_text+=2;
      offset+=2;
      x++;

    // process one entire horizontal row
    } while(--hchars);

    // go to next character row location
    buf_row += disp * cfheight;
    if (!split_screen && (y == split_textrow))
    {
      new_text = text_base;
      forceUpdate = 1;
      cs_y = 0;
      if (tm_info->split_hpanning) h_panning = 0;
      rows = ((res_y - line_compare + fontheight - 2) / fontheight) + 1;
      split_screen = 1;
    }
    else
    {
      new_text = new_line + tm_info->line_offset;
      old_text = old_line + tm_info->line_offset;
      cs_y++;
      y++;
    }
  } while(--rows);
  h_panning = tm_info->h_panning;
  prev_cursor_x = cursor_x;
  prev_cursor_y = cursor_y;
  graphics_tile_update_in_place(0, 0, res_x, res_y);
}

unsigned bx_sdl_gui_c::create_bitmap(
    const unsigned char *bmap,
    unsigned xdim,
    unsigned ydim)
{
  return 0;
}

unsigned bx_sdl_gui_c::headerbar_bitmap(
    unsigned bmap_id,
    unsigned alignment,
    void (*f)(void))
{
  return 0;
}

void bx_sdl_gui_c::replace_bitmap(unsigned hbar_id, unsigned bmap_id)
{
}

void bx_sdl_gui_c::show_headerbar(void)
{
}


void bx_sdl_gui_c::mouse_enabled_changed_specific (bx_bool val)
{
}


void headerbar_click(int x)
{
}

void bx_sdl_gui_c::exit(void)
{
}

void bx_sdl_gui_c::set_display_mode (disp_mode_t newmode)
{
}

void mousejs_motion(int dx, int dy, int dz, unsigned mouse_button_state, bx_bool a) {
    bx_devices.mouse_enabled_changed(1);
    bx_gui->mouse_enabled_changed(1);
    DEV_mouse_enabled_changed(1);
    DEV_mouse_motion(dx, dy, 0, mouse_button_state, 0);
}

void kbdjs_keydown(int jskeycode) {
  Bit32u key_event = sdl_sym_to_bx_key(jskeycode);
  if (key_event == BX_KEY_UNHANDLED) return;
  DEV_kbd_gen_scancode( key_event);
  if ((key_event == BX_KEY_NUM_LOCK) || (key_event == BX_KEY_CAPS_LOCK)) {
    DEV_kbd_gen_scancode(key_event | BX_KEY_RELEASED);
  }
}

void kbdjs_keyup(int jskeycode) {
  Bit32u key_event = sdl_sym_to_bx_key(jskeycode);
  if (key_event == BX_KEY_UNHANDLED) return;
  if ((key_event == BX_KEY_NUM_LOCK) || (key_event == BX_KEY_CAPS_LOCK)) {
    DEV_kbd_gen_scancode(key_event);
  }
  DEV_kbd_gen_scancode(key_event | BX_KEY_RELEASED);
}

bx_svga_tileinfo_t *bx_sdl_gui_c::graphics_tile_info(bx_svga_tileinfo_t *info)
{
  info->bpp = 32;
  info->pitch = res_x * 4;
  info->red_shift = 0 + 8;
  info->green_shift = 8 + 8;
  info->blue_shift = 16 + 8;
  info->red_mask = 0x0000ff;
  info->green_mask = 0x00ff00;
  info->blue_mask = 0xff0000;
  info->is_indexed = 0;

#ifdef BX_LITTLE_ENDIAN
  info->is_little_endian = 1;
#else
  info->is_little_endian = 0;
#endif

  return info;
}

Bit8u *bx_sdl_gui_c::graphics_tile_get(unsigned x0, unsigned y0, unsigned *w, unsigned *h)
{
  if (x0+x_tilesize > res_x) {
    *w = res_x - x0;
  }
  else {
    *w = x_tilesize;
  }

  if (y0+y_tilesize > res_y) {
    *h = res_y - y0;
  }
  else {
    *h = y_tilesize;
  }

  return (Bit8u *)(screen + res_x*y0 + x0);
}

void bx_sdl_gui_c::graphics_tile_update_in_place(unsigned x0, unsigned y0,
                                        unsigned w, unsigned h)
{
  unsigned w0;
  unsigned h0;

//  for (int tmpx = 0; tmpx < w; tmpx++) {
//    for (int tmpy = 0; tmpy < h; tmpy++) {
//        if (screen[res_x*y0 + x0 + tmpy*guest_xres + tmpx] != 0) {
//            fprintf(stderr, "val: %d\n", screen[res_x*y0 + x0 + tmpy*guest_xres + tmpx]);
//        }
//    }
//  }
  vgajs_tile_update_in_place(x0, y0, w, h, (screen + res_x*y0 + x0), res_x*4);
}

/*
void bx_sdl_gui_c::graphics_tile_update(Bit8u *snapshot, unsigned x, unsigned y)
{
  vgajs_tile_update(x, y, x_tilesize, y_tilesize, snapshot, disp_bpp);
}
*/

void bx_sdl_gui_c::graphics_tile_update(Bit8u *snapshot, unsigned x, unsigned y)
{
  Bit32u *buf, disp;
  Bit32u *buf_row;
  int i,j; 
  int w,h;

  disp = res_x;
  buf = screen + y*disp + x; 

  i = y_tilesize;
  if(i + y > res_y) i = res_y - y; 

  h = i;
  w = x_tilesize;

  // FIXME
  if (i<=0) return;

  switch (disp_bpp) {                                                                                                                                                                                                                         
    case 8: /* 8 bpp */
      do { 
        buf_row = buf; 
        j = x_tilesize;
        do { 
          *buf++ = palette[*snapshot].red | palette[*snapshot].green << 8 | palette[*snapshot].blue << 16;
          snapshot++;
        } while(--j);
        buf = buf_row + disp;
      } while(--i);
      vgajs_tile_update_in_place(x, y, w, h, (screen + res_x*y + x), res_x*4);
      break;
    default:
      BX_PANIC(("%u bpp modes handled by new graphics API", disp_bpp));
      return;
  }
}

bx_bool bx_sdl_gui_c::palette_change(Bit8u index, Bit8u red, Bit8u green, Bit8u blue)
{
  vgajs_setpalette(index, red, green, blue);

  return 1;
}

void bx_sdl_gui_c::dimension_update(
    unsigned x,
    unsigned y,
    unsigned fheight, // Font height
    unsigned fwidth,  // Font width
    unsigned bpp)
{
  /*if (bpp > 8) {
    BX_PANIC(("%d bpp graphics mode not supported yet", bpp));
  }*/
  guest_xres = x;
  guest_yres = y;
  if (bpp == 8 || bpp == 15 || bpp == 16 || bpp == 24 || bpp == 32) {
    guest_bpp = disp_bpp= bpp;
  } else {
    BX_PANIC(("%d bpp graphics mode not supported", bpp));
  }
  guest_textmode = (fheight > 0);
  guest_xres = x;
  guest_yres = y;
  if (guest_textmode) {
    fontheight = fheight;
    fontwidth = fwidth;
    text_cols = x / fontwidth;
    text_rows = y / fontheight;
  }

  if((x == res_x) && (y == res_y) && (disp_bpp == bpp)) return;
  delete[] screen;
  screen = new Bit32u[guest_xres * guest_yres];
  /*for (int tmpx = 0; tmpx < guest_xres; tmpx++) {
    for (int tmpy = 0; tmpy < guest_yres; tmpy++) {
        screen[tmpy*guest_xres + tmpx] = 255 | 255 << 8;
    }
  }*/
  delete[] tile_updated;
  tile_updated = new bx_bool[(guest_xres/x_tilesize +1) * (guest_yres/y_tilesize +1)];
  vgajs_dimension_update(x, y, bpp);

  res_x = x;
  res_y = y;
  //host_xres = x;
  //host_yres = y;
  //host_bpp = bpp;
}

void bx_sdl_gui_c::flush(void) {
  //vgajs_tile_update_in_place(0, 0, res_x, res_y, (screen + res_x*0 + 0), res_x*4);
}

void bx_sdl_gui_c::clear_screen(void)
{
    vgajs_clear_screen();
}


#endif /* if BX_WITH_SVGA */
