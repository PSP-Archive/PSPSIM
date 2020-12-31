// Part of SimCoupe - A SAM Coupe emulator
//
// OSD.cpp: SDL common "OS-dependant" functions
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

// Notes:
//  This "OS-dependant" module for the SDL version required some yucky
//  conditional blocks, but it's probably forgiveable in here!

#include "SimCoupe.h"
#include "OSD.h"

#include "CPU.h"
#include "Frame.h"
#include "Main.h"
#include "Options.h"
#include "Parallel.h"

SDL_sem* pSemaphore;
SDL_TimerID pTimer;
int OSD::s_nTicks;


Uint32 TimerCallback (Uint32 uInterval_, void *pv_)
{
    OSD::s_nTicks++;

    if (SDL_SemValue(pSemaphore) <= 0)
        SDL_SemPost(pSemaphore);

    return uInterval_;
}


bool OSD::Init (bool fFirstInit_/*=false*/)
{
    bool fRet = false;
#ifdef _WINDOWS
    // We'll do our own error handling, so suppress any windows error dialogs
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

    // The only sub-system we _need_ is video
    if (SDL_Init(SDL_INIT_VIDEO))
        TRACE("!!! SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
    else if (SDL_Init(SDL_INIT_TIMER) < 0)
        TRACE("!!! SDL_Init(SDL_INIT_TIMER) failed: %s\n", SDL_GetError());
    else if (!(pSemaphore = SDL_CreateSemaphore(0)))
        TRACE("!!! SDL_CreateSemaphore failed: %s\n", SDL_GetError());
    else if (!(pTimer = SDL_AddTimer(1000/EMULATED_FRAMES_PER_SECOND, TimerCallback, NULL)))
            TRACE("!!! SDL_AddTimer failed: %s\n", SDL_GetError());
    else
        fRet = true;

    if (!fRet)
        TRACE("SDL_Init failed: %s\n", SDL_GetError());

    return fRet;
}

void OSD::Exit (bool fReInit_/*=false*/)
{
    if (pTimer) { SDL_RemoveTimer(pTimer); pTimer = NULL; }
    if (pSemaphore) { SDL_DestroySemaphore(pSemaphore); pSemaphore = NULL; }

    SDL_Quit();
}


// Return a DWORD containing a millisecond accurate time stamp
// Note: calling could should allow for the value wrapping by only comparing differences
DWORD OSD::GetTime ()
{
    return SDL_GetTicks();
}


// Do whatever is necessary to locate an external file
const char* OSD::GetFilePath (const char* pcszFile_/*=""*/)
{
    static char szPath[512];

    // If the supplied file path looks absolute, use it as-is
    if (*pcszFile_ == PATH_SEPARATOR
#if defined(_WINDOWS) || defined(__AMIGAOS4__)
        || strchr(pcszFile_, ':')
#endif
        )
        return strncpy(szPath, pcszFile_, sizeof szPath);

    // Relative paths need platform-specific treatment

#if defined(_WINDOWS)
    // All Win32 files are relative to the EXE path
    GetModuleFileName(NULL, szPath, sizeof szPath);
    strrchr(szPath, '\\')[1] = '\0';
#elif defined(__AMIGAOS4__)
    strcpy(szPath, "PROGDIR:");
#else
    // If no file is given, fall back on the home directory (or ~/Documents on the Mac)
    if (!*pcszFile_)
    {
#if !defined(__APPLE__)
//LUDO: strcat(strcpy(szPath, getenv("HOME")), "/");
       getcwd(szPath, 512);
       strcat(szPath, "/");
#else
       //LUDO: strcat(strcpy(szPath, getenv("HOME")), "/Documents/");
#endif
    }
    else
    {
#if defined(__BEOS__)
        // Zeta breaks compatability by moving find_directory to libzeta.so :-/
//      find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, szPath, sizeof szPath);
//      strcat(szPath, "/");

        // Use a hard-coded path for now - can be fixed if/when Zeta is truly multi-user
        strcpy(szPath, "/boot/home/config/settings/");
#elif defined(__APPLE__)
        // Files are relative to the user preferences directory
        //LUDO: strcat(strcpy(szPath, getenv("HOME")), "/Library/Preferences/");

        // Use a more appropriate settings file name
        if (!strcasecmp(pcszFile_, "SimCoupe.cfg"))
          pcszFile_ = "SimCoupe Preferences";
#elif defined(__AMIGAOS4__)
        // default is Progdir:
        strcpy(szPath, "PROGDIR:");
#else
        // Files are relative to the home directory
//LUDO: strcat(strcpy(szPath, getenv("HOME")), "/");
       getcwd(szPath, 512);
       strcat(szPath, "/");

        // Use a more Unixy name for the configuration file
        if (!strcasecmp(pcszFile_, "SimCoupe.cfg"))
            pcszFile_ = ".simcoupecfg";
#endif
    }
#endif

    // Append the supplied file/path, and return a pointer to it
    return strcat(szPath, pcszFile_);
}

// Same as GetFilePath but ensures a trailing [back]slash
const char* OSD::GetDirPath (const char* pcszDir_/*=""*/)
{
    char *psz = const_cast<char*>(GetFilePath(pcszDir_)), *pszEnd = psz+strlen(psz);

    // Append a [back]slash to non-empty strings that don't already have one
    if (*psz && pszEnd[-1] != PATH_SEPARATOR)
    {
#if !defined (__AMIGAOS4__)
        pszEnd[0] = PATH_SEPARATOR;
#endif
        pszEnd[1] = '\0';
    }

    return psz;
}


// Check whether the specified path is accessible
bool OSD::CheckPathAccess (const char* pcszPath_)
{
    return !access(pcszPath_, X_OK);
}


// Return whether a file/directory is normally hidden from a directory listing
bool OSD::IsHidden (const char* pcszPath_)
{
#ifdef _WINDOWS
    // Hide entries with the hidden or system attribute bits set
    DWORD dwAttrs = GetFileAttributes(pcszPath_);
    return (dwAttrs != 0xffffffff) && (dwAttrs & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM));
#else
    // Hide entries beginning with a dot
    pcszPath_ = strrchr(pcszPath_, PATH_SEPARATOR);
    return pcszPath_ && pcszPath_[1] == '.';
#endif
}


// Return the path to use for a given drive with direct floppy access
const char* OSD::GetFloppyDevice (int nDrive_)
{
#if defined (__AMIGAOS4__)
    static char szDevice[] = "DF0:";
#else
    static char szDevice[] = "/dev/fd_";
#endif

    szDevice[7] = '0' + nDrive_-1;
    return szDevice;
}


void OSD::DebugTrace (const char* pcsz_)
{
#ifdef _WINDOWS
    OutputDebugString(pcsz_);
#elif defined (__AMIGAOS4__)
    printf("%s", pcsz_);
#else
    fprintf(stderr, "%s", pcsz_);
#endif
}


int OSD::FrameSync (bool fWait_/*=true*/)
{
    if (fWait_)
        SDL_SemWait(pSemaphore);

    return s_nTicks;
}

////////////////////////////////////////////////////////////////////////////////

// Dummy printer device implementation
CPrinterDevice::CPrinterDevice () { }
CPrinterDevice::~CPrinterDevice () { }
bool CPrinterDevice::Open () { return false; }
void CPrinterDevice::Close () { }
void CPrinterDevice::Write (BYTE *pb_, size_t uLen_) { }

////////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS

WIN32_FIND_DATA s_fd;
struct dirent s_dir;

DIR* opendir (const char* pcszDir_)
{
    static char szPath[_MAX_PATH];

    memset(&s_dir, 0, sizeof s_dir);

    // Append a wildcard to match all files
    lstrcpy(szPath, pcszDir_);
    if (szPath[lstrlen(szPath)-1] != '\\')
        lstrcat(szPath, "\\");
    lstrcat(szPath, "*");

    // Find the first file, saving the details for later
    HANDLE h = FindFirstFile(szPath, &s_fd);

    // Return the handle if successful, otherwise NULL
    return (h == INVALID_HANDLE_VALUE) ? NULL : reinterpret_cast<DIR*>(h);
}

struct dirent* readdir (DIR* hDir_)
{
    // All done?
    if (!s_fd.cFileName[0])
        return NULL;

    // Copy the filename and set the length
    s_dir.d_reclen = lstrlen(lstrcpyn(s_dir.d_name, s_fd.cFileName, sizeof s_dir.d_name));

    // If we'd already reached the end
    if (!FindNextFile(reinterpret_cast<HANDLE>(hDir_), &s_fd))
        s_fd.cFileName[0] = '\0';

    // Return the current entry
    return &s_dir;
}

int closedir (DIR* hDir_)
{
    return FindClose(reinterpret_cast<HANDLE>(hDir_)) ? 0 : -1;
}

#endif

////////////////////////////////////////////////////////////////////////////////

int 
SDL_main(int argc_, char* argv_[])
{
#ifdef __APPLE__
    // OS X lacks a unique Right-Ctrl key, so enable the "Left-Alt for Cntrl" option by default
    SetDefault(altforcntrl,true);
#endif
    if (Main::Init(argc_, argv_)) {
      CPU::Run();
    }

    Main::Exit();

    return 0;
}
