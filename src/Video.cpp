// Part of SimCoupe - A SAM Coupe emulator
//
// Video.cpp: SDL video handling for surfaces, screen modes, palettes etc.
//
//  Copyright (c) 1999-2006  Simon Owen
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

//  ToDo:
//   - possibly separate out the OpenGL and SDL code further?

#include "SimCoupe.h"
#include "Video.h"

#include "Action.h"
#include "Display.h"
#include "Frame.h"
#include "GUI.h"
#include "Options.h"
#include "UI.h"

#include "psp_danzeff.h"
#include "psp_sdl.h"
#include "psp_gu.h"

const int N_TOTAL_COLOURS = N_PALETTE_COLOURS + N_GUI_COLOURS;

// SAM RGB values in appropriate format, and YUV values pre-shifted for overlay surface
WORD aulPalette[N_TOTAL_COLOURS], aulScanline[N_TOTAL_COLOURS];

  extern SDL_Surface *back_surface;

// Mask for the current SimCoupe.bmp image used as the SDL program icon
static BYTE abIconMask[128] =
{
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x00,
    0x00, 0x1f, 0xf0, 0x00, 0x00, 0x7f, 0xfc, 0x00,
    0x00, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xff, 0x00,
    0x03, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0x80,
    0x07, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xc0,
    0x07, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xc0,
    0x07, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xc0,
    0x28, 0x1f, 0xf0, 0x28, 0x7c, 0x7d, 0xdc, 0x7c,
    0xfc, 0xff, 0xfe, 0x7e, 0x7f, 0xff, 0xff, 0xfc,
    0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfc,
    0x03, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0x80,
    0x07, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xc0,
    0x07, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xc0,
    0x00, 0xf8, 0x3e, 0x00, 0x01, 0xfc, 0x7f, 0x00,
    0x03, 0xfe, 0xff, 0x80, 0x03, 0xfe, 0xff, 0x80,
    0x03, 0xfe, 0xff, 0x80, 0x03, 0xfe, 0xff, 0x80
};


// function to initialize DirectDraw in windowed mode
bool Video::Init (bool fFirstInit_/*=false*/)
{
    bool fRet = false;

    Exit(true);
    TRACE("-> Video::Init(%s)\n", fFirstInit_ ? "first" : "");

    if (fFirstInit_ && SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        TRACE("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: %s: %s\n", SDL_GetError());
    else
    {
        DWORD dwWidth = Frame::GetWidth(), dwHeight = Frame::GetHeight();

        int nDepth = GetOption(fullscreen) ? GetOption(depth) : 0;

        // Use a hardware surface if possible, and a palette if we're running in 8-bit mode
        DWORD dwOptions =  SDL_HWSURFACE | (nDepth == 8) ? SDL_HWPALETTE : 0;

        // Full screen mode requires a display mode change
# if 0 //LUDO:
        if (!GetOption(fullscreen)) 
# endif
        {
          dwWidth = 480;
          dwHeight = 272;
          nDepth = 16;
          back_surface = SDL_SetVideoMode(dwWidth, dwHeight, nDepth, dwOptions|SDL_DOUBLEBUF);
        }

        // Did we fail to create the front buffer?
        if (!back_surface) {
            TRACE("Failed to create front buffer!\n");
        }

        psp_sdl_gu_init();

        blit_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
           SIM_WIDTH, SIM_HEIGHT,
           back_surface->format->BitsPerPixel,
           back_surface->format->Rmask,
           back_surface->format->Gmask,
           back_surface->format->Bmask, 0);
        // HACK
        blit_surface->pixels = (void *)0x44088000;

        SDL_ShowCursor(SDL_DISABLE);

        psp_sdl_display_splash();

        // Create the appropriate palette needed for the surface (including a hardware palette for 8-bit mode)
        CreatePalettes();

        fRet = UI::Init(fFirstInit_);

        /* Danzeff Keyboard */
        danzeff_load();
        danzeff_set_screen(back_surface);
    }

    if (!fRet)
        Exit();

    TRACE("<- Video::Init() returning %s\n", fRet ? "true" : "FALSE");
    return fRet;
}

// Cleanup DirectX by releasing all the interfaces we have
void Video::Exit (bool fReInit_/*=false*/)
{
    TRACE("-> Video::Exit(%s)\n", fReInit_ ? "reinit" : "");

    if (back_surface) { SDL_FreeSurface(back_surface); back_surface = NULL; }

    if (!fReInit_)
        SDL_QuitSubSystem(SDL_INIT_VIDEO);

    TRACE("<- Video::Exit()\n");
}


// Create whatever's needed for actually displaying the SAM image
bool Video::CreatePalettes (bool fDimmed_)
{
    SDL_Color acPalette[N_TOTAL_COLOURS];
    bool fPalette = back_surface && (back_surface->format->BitsPerPixel == 8);
    TRACE("CreatePalette: fPalette = %s\n", fPalette ? "true" : "false");

    // Determine the scanline brightness level adjustment, in the range -100 to +100
    int nScanAdjust = GetOption(scanlines) ? (GetOption(scanlevel) - 100) : 0;
    if (nScanAdjust < -100) nScanAdjust = -100;

    fDimmed_ |= (g_fPaused && !g_fFrameStep) || GUI::IsActive() || (!g_fActive && GetOption(pauseinactive));
    const RGBA *pSAM = IO::GetPalette(fDimmed_), *pGUI = GUI::GetPalette();

    // Build the full palette from SAM and GUI colours
    for (int i = 0; i < N_TOTAL_COLOURS ; i++)
    {
        // Look up the colour in the appropriate palette
        const RGBA* p = (i < N_PALETTE_COLOURS) ? &pSAM[i] : &pGUI[i-N_PALETTE_COLOURS];
        BYTE r = p->bRed, g = p->bGreen, b = p->bBlue;

        // Ask SDL to map non-palettised colours
        if (!fPalette)
        {
            aulPalette[i] = SDL_MapRGB(back_surface->format, r,g,b);
            AdjustBrightness(r,g,b, nScanAdjust);
            aulScanline[i] = SDL_MapRGB(back_surface->format, r,g,b);
        }
        else
        {
            // Set the palette index and components
            aulPalette[i] = aulScanline[i] = i;
            acPalette[i].r = r;
            acPalette[i].g = g;
            acPalette[i].b = b;
        }
    }

    // If a palette is required, set it on both surfaces now
    if (fPalette)
    {
        SDL_SetPalette(back_surface, SDL_LOGPAL|SDL_PHYSPAL, acPalette, 0, N_TOTAL_COLOURS);
    }

    // Because the pixel format may have changed, we need to refresh the SAM CLUT pixel values
    for (int c = 0 ; c < 16 ; c++)
        clut[c] = aulPalette[clutval[c]];

    // Ensure the display is redrawn to reflect the changes
    Display::SetDirty();

    return true;
}
