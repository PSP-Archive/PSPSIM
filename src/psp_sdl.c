/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <png.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <psprtc.h>

#include "simcoupec.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"

  extern unsigned char psp_font[];

  SDL_Surface *back_surface;
  SDL_Surface *back2_surface;
  SDL_Surface *back3_surface;
  SDL_Surface *blit_surface;
  SDL_Surface *splash_surface;

unsigned char
psp_convert_utf8_to_iso_8859_1(unsigned char c1, unsigned char c2)
{
  unsigned char res = 0;
  if (c1 == 0xc2) res = c2;
  else
  if (c1 == 0xc3) res = c2 | 0x40;
  return res;
}

uint
psp_sdl_rgb(uchar R, uchar G, uchar B)
{
  return SDL_MapRGB(back_surface->format, R,G,B);
}

ushort *
psp_sdl_get_vram_addr(uint x, uint y)
{
  ushort *vram = (ushort *)back_surface->pixels;
  return vram + x + (y*PSP_LINE_SIZE);
}

void
loc_psp_debug(char *file, int line, char *message)
{
  static int current_line = 0;
  static int current_col  = 10;

  char buffer[128];
  current_line += 10;
  if (current_line > 250)
  {
    if (current_col == 200) {
      psp_sdl_clear_screen(psp_sdl_rgb(0, 0, 0xff));
      current_col = 10;
    } else {
      current_col = 200;
    }
    
    current_line = 10;
  }
  sprintf(buffer,"%s:%d %s", file, line, message);
  psp_sdl_print(current_col, current_line, buffer, psp_sdl_rgb(0xff,0xff,0xff) );
}

void 
psp_sdl_print(int x,int y, char *str, int color)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    psp_sdl_put_char(x, y, color, 0, str[index], 1, 0);
    x += 8;
    if (x >= (PSP_SDL_SCREEN_WIDTH - 8)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - 8)) break;
  }
}

void
psp_sdl_clear_screen(int color)
{
  int x; int y;
  ushort *vram = psp_sdl_get_vram_addr(0,0);
  
  for (y = 0; y < PSP_SDL_SCREEN_HEIGHT; y++) {
    for (x = 0; x < PSP_SDL_SCREEN_WIDTH; x++) {
      vram[x + (y*PSP_LINE_SIZE)] = color;
    }
  }
}

void
psp_sdl_black_screen()
{
  SDL_FillRect(back_surface,NULL,SDL_MapRGB(back_surface->format,0x0,0x0,0x0));
  SDL_Flip(back_surface);
  SDL_FillRect(back_surface,NULL,SDL_MapRGB(back_surface->format,0x0,0x0,0x0));
  SDL_Flip(back_surface);
}

void
psp_sdl_clear_blit(int color)
{
  if (blit_surface) {
    ushort *vram = (ushort *)blit_surface->pixels;
    int my_size = blit_surface->h * blit_surface->w;
    while (my_size > 0) {
      vram[--my_size] = color;
    }
  }
}

void 
psp_sdl_draw_rectangle(int x, int y, int w, int h, int border, int mode) 
{
  ushort *vram = (ushort *)psp_sdl_get_vram_addr(x, y);
  int xo, yo;
  if (mode == PSP_SDL_XOR) {
    for (xo = 0; xo < w; xo++) {
      vram[xo] ^=  border;
      vram[xo + h * PSP_LINE_SIZE] ^=  border;
    }
    for (yo = 0; yo < h; yo++) {
      vram[yo * PSP_LINE_SIZE] ^=  border;
      vram[w + yo * PSP_LINE_SIZE] ^=  border;
    }
    vram[w + h * PSP_LINE_SIZE] ^=  border;
  } else {
    for (xo = 0; xo < w; xo++) {
      vram[xo] =  border;
      vram[xo + h * PSP_LINE_SIZE] =  border;
    }
    for (yo = 0; yo < h; yo++) {
      vram[yo * PSP_LINE_SIZE] =  border;
      vram[w + yo * PSP_LINE_SIZE] =  border;
    }
    vram[w + h * PSP_LINE_SIZE] =  border;
  }
}

void 
psp_sdl_fill_rectangle(int x, int y, int w, int h, int color, int mode)
{
  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);
  int xo, yo;
  if (mode == PSP_SDL_XOR) {
    for (xo = 0; xo <= w; xo++) {
      for (yo = 0; yo <= h; yo++) {
        if ( ((xo == 0) && ((yo == 0) || (yo == h))) ||
             ((xo == w) && ((yo == 0) || (yo == h))) ) {
          /* Skip corner */
        } else {
          vram[xo + yo * PSP_LINE_SIZE] ^=  color;
        }
      }
    }
  } else {
    for (xo = 0; xo <= w; xo++) {
      for (yo = 0; yo <= h; yo++) {
        vram[xo + yo * PSP_LINE_SIZE] =  color;
      }
    }
  }
}

static int
psp_sdl_get_back2_color(int x, int y)
{
  uchar *back2 = (uchar *)back2_surface->pixels;
  int bytes_per_pixels = back2_surface->format->BytesPerPixel;
  int pitch            = back2_surface->pitch;
  Uint8 r = back2[0 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 g = back2[1 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 b = back2[2 + (y * pitch) + (x * bytes_per_pixels)];
	int color = psp_sdl_rgb(r, g, b);

  return color;
}

void 
psp_sdl_back2_rectangle(int x, int y, int w, int h)
{
  if (! back2_surface) {
    psp_sdl_fill_rectangle(x, y, w, h, 0x0, 0);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  int xo, yo;
  for (xo = 0; xo <= w; xo++) {
    for (yo = 0; yo <= h; yo++) {
      vram[xo + yo * PSP_LINE_SIZE] = psp_sdl_get_back2_color(x + xo, y + yo);
    }
  }
}

static int
psp_sdl_get_back3_color(int x, int y)
{
  uchar *back3 = (uchar *)back3_surface->pixels;
  int bytes_per_pixels = back3_surface->format->BytesPerPixel;
  int pitch            = back3_surface->pitch;
  Uint8 r = back3[0 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 g = back3[1 + (y * pitch) + (x * bytes_per_pixels)];
  Uint8 b = back3[2 + (y * pitch) + (x * bytes_per_pixels)];
	int color = psp_sdl_rgb(r, g, b);

  return color;
}

void 
psp_sdl_back3_rectangle(int x, int y, int w, int h)
{
  if (! back3_surface) {
    psp_sdl_fill_rectangle(x, y, w, h, 0x0, 0);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  int xo, yo;
  for (xo = 0; xo <= w; xo++) {
    for (yo = 0; yo <= h; yo++) {
      vram[xo + yo * PSP_LINE_SIZE] = psp_sdl_get_back3_color(x + xo, y + yo);
    }
  }
}


void 
psp_sdl_put_char(int x, int y, int color, int bgcolor, uchar c, int drawfg, int drawbg)
{
  int cx;
  int cy;
  int b;
  int index;

  ushort *vram = (ushort *)psp_sdl_get_vram_addr(x, y);
  index = ((ushort)c) * 8;

  for (cy=0; cy<8; cy++) {
    b=0x80;
    for (cx=0; cx<8; cx++) {
      if (psp_font[index] & b) {
        if (drawfg) vram[cx + cy * PSP_LINE_SIZE] = color;
      } else {
        if (drawbg) vram[cx + cy * PSP_LINE_SIZE] = bgcolor;
      }
      b = b >> 1;
    }
    index++;
  }
}

void 
psp_sdl_back2_put_char(int x, int y, int color, uchar c)
{
  int cx;
  int cy;
  int bmask;
  int index;

  if (! back2_surface) {
    psp_sdl_put_char(x, y, color, 0x0, c, 1, 1);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  index = ((ushort)c) * 8;

  for (cy=0; cy<8; cy++) {
    bmask=0x80;
    for (cx=0; cx<8; cx++) {
      if (psp_font[index] & bmask) {
        vram[cx + cy * PSP_LINE_SIZE] = color;
      } else {
        vram[cx + cy * PSP_LINE_SIZE] = psp_sdl_get_back2_color(x + cx, y + cy);
      }
      bmask = bmask >> 1;
    }
    index++;
  }
}

void 
psp_sdl_back3_put_char(int x, int y, int color, uchar c)
{
  int cx;
  int cy;
  int bmask;
  int index;

  if (! back3_surface) {
    psp_sdl_put_char(x, y, color, 0x0, c, 1, 1);
    return;
  }

  ushort *vram  = (ushort *)psp_sdl_get_vram_addr(x, y);

  index = ((ushort)c) * 8;

  for (cy=0; cy<8; cy++) {
    bmask=0x80;
    for (cx=0; cx<8; cx++) {
      if (psp_font[index] & bmask) {
        vram[cx + cy * PSP_LINE_SIZE] = color;
      } else {
        vram[cx + cy * PSP_LINE_SIZE] = psp_sdl_get_back3_color(x + cx, y + cy);
      }
      bmask = bmask >> 1;
    }
    index++;
  }
}


void 
psp_sdl_fill_print(int x,int y,const char *str, int color, int bgcolor)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    uchar c = str[index];
    if ((c == 0xc2) || (c == 0xc3)) {
      uchar new_c = psp_convert_utf8_to_iso_8859_1(c, str[index+1]);
      if (new_c) { c = new_c; index++; }
    }
    psp_sdl_put_char(x, y, color, bgcolor, c, 1, 1);
    x += 8;
    if (x >= (PSP_SDL_SCREEN_WIDTH - 8)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - 8)) break;
  }
}

void
psp_sdl_back2_print(int x,int y,const char *str, int color)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    uchar c = str[index];
    if ((c == 0xc2) || (c == 0xc3)) {
      uchar new_c = psp_convert_utf8_to_iso_8859_1(c, str[index+1]);
      if (new_c) { c = new_c; index++; }
    }
    psp_sdl_back2_put_char(x, y, color, c);
    x += 8;
    if (x >= (PSP_SDL_SCREEN_WIDTH - 8)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - 8)) break;
  }
}

void
psp_sdl_back3_print(int x,int y,const char *str, int color)
{
  int index;
  int x0 = x;

  for (index = 0; str[index] != '\0'; index++) {
    uchar c = str[index];
    if ((c == 0xc2) || (c == 0xc3)) {
      uchar new_c = psp_convert_utf8_to_iso_8859_1(c, str[index+1]);
      if (new_c) { c = new_c; index++; }
    }
    psp_sdl_back3_put_char(x, y, color, c);
    x += 8;
    if (x >= (PSP_SDL_SCREEN_WIDTH - 8)) {
      x = x0; y++;
    }
    if (y >= (PSP_SDL_SCREEN_HEIGHT - 8)) break;
  }
}

void
psp_sdl_wait_vn(uint count)
{
  for (; count>0; --count) {
    sceDisplayWaitVblankStart();
  }
}

void
psp_sdl_wait_vblank(void)
{
  sceDisplayWaitVblankStart();
}

void
psp_sdl_lock(void)
{
  SDL_LockSurface(back_surface);
}

void
psp_sdl_load_background()
{
  back2_surface = IMG_Load("./background.png");
}

void
psp_sdl_load_background3()
{
  back3_surface = IMG_Load("./background3.png");
}

void
psp_sdl_blit_background()
{
  static int first = 1;

  if (first && (back2_surface == NULL)) {
    psp_sdl_load_background();
    first = 0;
  }
  if (back2_surface != NULL) {
	  SDL_BlitSurface(back2_surface, NULL, back_surface, NULL);
  } else {
    psp_sdl_clear_screen(psp_sdl_rgb(0x00, 0x00, 0x00));
  }
}

void
psp_sdl_blit_background3()
{
  static int first = 1;

  if (first && (back3_surface == NULL)) {
    psp_sdl_load_background3();
    first = 0;
  }
  if (back3_surface != NULL) {
	  SDL_BlitSurface(back3_surface, NULL, back_surface, NULL);
  } else {
    psp_sdl_clear_screen(psp_sdl_rgb(0x00, 0x00, 0x00));
  }
}

void
psp_sdl_blit_splash()
{
  if (! splash_surface) {
    splash_surface = IMG_Load("./splash.png");
  }
	SDL_BlitSurface(splash_surface, NULL, back_surface, NULL);
}

void
psp_sdl_display_splash()
{
  int index = 0;
  SceCtrlData c;

  psp_sdl_blit_splash();
  psp_sdl_flip();
  psp_sdl_blit_splash();
  psp_sdl_flip();

  while (index < 50) {
    sceCtrlPeekBufferPositive(&c, 1);
    if (c.Buttons & (PSP_CTRL_START|PSP_CTRL_CROSS)) break;
    sceKernelDelayThread(100000); 
    index++;
  }
}

void
psp_sdl_unlock(void)
{
  SDL_UnlockSurface(back_surface);
}

void
psp_sdl_flip(void)
{
  SDL_Flip(back_surface);
}

#define  systemRedShift      (back_surface->format->Rshift)
#define  systemGreenShift    (back_surface->format->Gshift)
#define  systemBlueShift     (back_surface->format->Bshift)
#define  systemRedMask       (back_surface->format->Rmask)
#define  systemGreenMask     (back_surface->format->Gmask)
#define  systemBlueMask      (back_surface->format->Bmask)

void
psp_sdl_save_png(char *filename)
{
  int w = 480;
  int h = 272;
  u8* pix = (u8*)psp_sdl_get_vram_addr(0,0);
  u8 writeBuffer[512 * 3];

  FILE *fp = fopen(filename,"wb");

  if(!fp) return;
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                NULL,
                                                NULL);
  if(!png_ptr) {
    fclose(fp);
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return;
  }

  png_init_io(png_ptr,fp);

  png_set_IHDR(png_ptr,
               info_ptr,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr,info_ptr);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;
  int y;
  int x;

  u16 *p = (u16 *)pix;
  for(y = 0; y < sizeY; y++) {
     for(x = 0; x < sizeX; x++) {
      u16 v = p[x];

      *b++ = ((v & systemRedMask  ) >> systemRedShift  ) << 3; // R
      *b++ = ((v & systemGreenMask) >> systemGreenShift) << 2; // G 
      *b++ = ((v & systemBlueMask ) >> systemBlueShift ) << 3; // B
    }
    p += 512;
    png_write_row(png_ptr,writeBuffer);
     
    b = writeBuffer;
  }

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);
}

void
psp_sdl_save_screenshot(void)
{
  char TmpFileName[MAX_PATH];

  int psp_screenshot_id = sim_get_psp_screenshot_id();
  sprintf(TmpFileName,"%s/scr/screenshot_%d.png", sim_get_home_dir(), psp_screenshot_id++);
  if (psp_screenshot_id >= 10) psp_screenshot_id = 0;
  sim_set_psp_screenshot_id(psp_screenshot_id);
  psp_sdl_save_png(TmpFileName);
}

void
psp_sdl_exit(int status)
{
  SDL_CloseAudio();
  SDL_Quit();

  exit(status);
}
