// Part of SimCoupe - A SAM Coupe emulator
//
// Main.cpp: Main entry point
//
//  Copyright (c) 1999-2004  Simon Owen
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

#include "SimCoupe.h"
#include "Main.h"

#include "CPU.h"
#include "Frame.h"
#include "Input.h"
#include "Options.h"
#include "Sound.h"
#include "UI.h"
#include "Util.h"
#include "Display.h"

#include "psp_sdl.h"

bool Main::Init (int argc_, char* argv_[])
{
    // Load settings and check command-line options
    if (!Util::Init() || !Options::Load(argc_, argv_))
        return 0;

    // Initialise all modules
    bool f = OSD::Init(true) && 
           Sound::Init(true) && 
           Frame::Init(true) && 
           Input::Init(true) && 
           CPU::Init(true);
   psp_sdl_black_screen();
   return f;
}

void
Main::Exit ()
{
    CPU::Exit();
    Input::Exit();
    Sound::Exit();
    Frame::Exit();
    OSD::Exit();

    Options::Save();

    Util::Exit();
}
