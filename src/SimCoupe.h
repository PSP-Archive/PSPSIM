// Part of SimCoupe - A SAM Coupe emulator
//
// SimCoupe.h: Common SimCoupe header, included by all modules
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

#ifndef SIMCOUPE_H
#define SIMCOUPE_H

// If it's not one of these we'll assume big endian (we have a run-time check to fall back on anyway)
#if (defined(__LITTLE_ENDIAN__) || defined(__i386__) || defined(__ia64__) || defined(__x86_64__) || \
    (defined(__alpha__) || defined(__alpha)) || (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__arm__) || defined(__SYMBIAN32__) || defined(_WIN32_WCE) || defined(WIN32)) \
     && !defined(__BIG_ENDIAN__)
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#else
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#endif

#ifdef DEBUG
#define _DEBUG
#endif

typedef unsigned int        UINT;

#include "OSD.h"            // OS-dependant stuff

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

// To avoid relying on STL, we'll define our own swap template
template <class T> void swap (T& a, T& b) { T tmp=a; a=b; b=tmp; }

// Windows CE lacks some headers
#ifndef _WIN32_WCE
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#endif

#ifdef USE_ZLIB
#include "unzip.h"    // for unzOpen, unzClose, etc.  Part of the contrib/minizip in the ZLib source package
#include "zlib.h"               // for gzopen, gzclose, etc.
#endif

#include "SAM.h"        // Various SAM constants
#include "Util.h"       // TRACE macro and other utility functions

# define SIM_RENDER_NORMAL     0
# define SIM_RENDER_FIT_WIDTH  1
# define SIM_RENDER_FIT_HEIGHT 2
# define SIM_RENDER_FIT        3
# define SIM_RENDER_X125       4
# define SIM_RENDER_MAX_WIDTH  5
# define SIM_RENDER_MAX_HEIGHT 6
# define SIM_RENDER_MAX        7
# define SIM_LAST_RENDER       7

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define MAX_PATH            512

#endif  // SIMCOUPE_H
