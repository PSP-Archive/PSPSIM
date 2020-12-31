// Part of SimCoupe - A SAM Coupe emulator
//
// Main.h: Main entry point
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

#ifndef MAIN_H
#define MAIN_H

class Main
{
    public:
        static bool Init (int argc_, char* argv_[]);
        static void Exit ();
# if 0
        static int SaveStateFile(char *FileName);
        static int LoadStateFile(char *FileName);
# endif
};

int main (int argc, char *argv[]);

#endif  // MAIN_H
