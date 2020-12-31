// Part of SimCoupe - A SAM Coupe emulator
//
// Options.cpp: Option saving, loading and command-line processing
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
//  Options specified on the command-line override options in the file.
//  The settings are only and written back when it's closed.

#include <psptypes.h>
#include <psppower.h>
#include "SimCoupe.h"
#include "Options.h"

#include "simcoupec.h"
#include "IO.h"
#include "OSD.h"
#include "Util.h"

const char* const OPTIONS_FILE = "SimCoupe.cfg";
const int CFG_VERSION = 4;      // increment to force a config reset, if incompatible changes are made

enum { OT_BOOL, OT_INT, OT_STRING };

typedef struct
{
    const char* pcszName;                                       // Option name used in config file
    int nType;                                                  // Option type

    union { void* pv;  char* ppsz;  bool* pf;  int* pn; };      // Address of config variable

    const char* pcszDefault;                                    // Default value of option, with only appropriate type used
    int nDefault;
    bool fDefault;

    bool fSpecified;
}
OPTION;

// Helper macros for structure definition below
#define OPT_S(o,v,s)        { o, OT_STRING, {&Options::s_Options.v}, (s), 0,  false }
#define OPT_N(o,v,n)        { o, OT_INT,    {&Options::s_Options.v}, "", (n), false }
#define OPT_F(o,v,f)        { o, OT_BOOL,   {&Options::s_Options.v}, "",  0,  (f) }

OPTIONS Options::s_Options;

OPTION aOptions[] = 
{
    OPT_N("CfgVersion",   cfgversion,     0),         // Config compatability number
    OPT_F("FirstRun",     firstrun,       1),         // Non-zero if this is the first run

    OPT_N("Sync",         sync,           1),         // Sync to 50Hz
    OPT_N("FrameSkip",    frameskip,      0),         // Auto frame-skipping
    OPT_N("Scale",        scale,          2),         // Windowed display is 2x2
    OPT_F("Ratio5_4",     ratio5_4,       false),     // Don't use 5:4 screen ratio
    OPT_F("Scanlines",    scanlines,      true),      // TV scanlines
    OPT_N("ScanLevel",    scanlevel,      75),        // Scanlines are 75% brightness
    OPT_N("Mode3",        mode3,          0),         // Show only odd mode3 pixels on low-res displays
    OPT_N("Fullscreen",   fullscreen,     0),         // Not full screen
    OPT_N("Depth",        depth,          16),        // Full screen mode uses 16-bit colour
    OPT_N("Borders",      borders,        2),         // Same amount of borders as previous version
    OPT_F("StretchToFit", stretchtofit,   false),     // Don't stretch image to fit the display area
    OPT_F("Overlay",      overlay,        true),      // Use a video overlay surface, if available
    OPT_F("HWAccel",      hwaccel,        true),      // Use hardware accelerated video
    OPT_F("Greyscale",    greyscale,      false),     // Colour display
    OPT_F("Filter",       filter,         false),     // Filter the OpenGL image when stretching

    OPT_S("ROM",          rom,            ""),        // No custom ROM (use built-in)
    OPT_F("HDBootRom",    hdbootrom,      false),     // Don't use HDBOOT ROM patches
    OPT_F("FastReset",    fastreset,      true),      // Allow fast Z80 resets
    OPT_F("AsicDelay",    asicdelay,      false),     // No ASIC startup delay of ~50ms
    OPT_N("MainMemory",   mainmem,        512),       // 512K main memory
    OPT_N("ExternalMem",  externalmem,    0),         // No external memory

    OPT_N("Drive1",       drive1,         1),         // Floppy drive 1 present
    OPT_N("Drive2",       drive2,         1),         // Floppy drive 2 present
    OPT_N("TurboLoad",    turboload,      15),        // Accelerate disk access (medium sensitivity)
    OPT_F("SavePrompt",   saveprompt,     true),      // Prompt before saving changes
    OPT_F("AutoBoot",     autoboot,       true),      // Autoboot disks inserted at the startup screen
    OPT_F("DosBoot",      dosboot,        true),      // Automagically boot DOS from non-bootable disks
    OPT_S("DosDisk",      dosdisk,        ""),        // No override DOS disk, use internal SAMDOS 2.2
    OPT_F("StdFloppy",    stdfloppy,      true),      // Assume real disks are standard format, initially

    OPT_S("Disk1",        disk1,          ""),        // No disk in floppy drive 1
    OPT_S("Disk2",        disk2,          ""),        // No disk in floppy drive 2
    OPT_S("AtomDisk",     atomdisk,       ""),        // No Atom hard disk
    OPT_S("SDIDEDisk",    sdidedisk,      ""),        // No SD IDE hard disk
    OPT_S("YATBusDisk",   yatbusdisk,     ""),        // No YAMOD.ATBUS disk

    OPT_S("FloppyPath",   floppypath,     ""),        // Default floppy path
    OPT_S("HDDPath",      hddpath,        ""),        // Default hard disk path
    OPT_S("ROMPath",      rompath,        ""),        // Default ROM path
    OPT_S("DataPath",     datapath,       ""),        // Default data path
    OPT_S("MRU0",         mru0,           ""),        // No recently used files
    OPT_S("MRU1",         mru1,           ""),
    OPT_S("MRU2",         mru2,           ""),
    OPT_S("MRU3",         mru3,           ""),
    OPT_S("MRU4",         mru4,           ""),
    OPT_S("MRU5",         mru5,           ""),

    OPT_N("KeyMapping",   keymapping,     1),         // SAM keyboard mapping
    OPT_F("AltForCntrl",  altforcntrl,    false),     // Left-Alt not used for SAM Cntrl
    OPT_F("AltGrForEdit", altgrforedit,   true),      // Right-Alt used for SAM Edit
    OPT_F("KeypadReset",  keypadreset,    true),      // Keypad-minus for Reset
    OPT_F("SAMFKeys",     samfkeys,       false),     // PC function keys not mapped to SAM keypad
    OPT_F("Mouse",        mouse,          false),     // Mouse not connected
    OPT_F("MouseEsc",     mouseesc,       true),      // Allow Esc to release the mouse capture
    OPT_F("Swap23",       swap23,         false),     // Don't swap mouse buttons 2 and 3

    OPT_S("JoyDev1",      joydev1,        ""),        // Joystick 1 device
    OPT_S("JoyDev2",      joydev2,        ""),        // Joystick 2 device
    OPT_N("DeadZone1",    deadzone1,      20),        // Joystick 1 deadzone is 20% around central position
    OPT_N("DeadZone2",    deadzone2,      20),        // Joystick 2 deadzone is 20% around central position

    OPT_N("Parallel1",    parallel1,      0),         // Nothing on parallel port 1
    OPT_N("Parallel2",    parallel2,      0),         // Nothing on parallel port 2
    OPT_S("PrinterDev",   printerdev,     ""),        // No printer device (save to file)
    OPT_F("PrinterOnline",printeronline,  1),         // Printer is online
    OPT_N("FlushDelay",   flushdelay,     2),         // Auto-flush printer data after 2 seconds

    OPT_S("SerialDev1",   serialdev1,     ""),        // Serial port 1 device
    OPT_S("SerialDev2",   serialdev2,     ""),        // Serial port 2 device

    OPT_N("Midi",         midi,           0),         // Nothing on MIDI port
    OPT_S("MidiInDev",    midiindev,      ""),        // MIDI-In device
    OPT_S("MidiOutDev",   midioutdev,     ""),        // MIDI-Out device
//  OPT_N("NetworkId",    networkid,      1),         // Network station number, or something, eventually

    OPT_F("SambusClock",  sambusclock,    true),      // SAMBUS clock present
    OPT_F("DallasClock",  dallasclock,    false),     // DALLAS clock not present
    OPT_F("ClockSync",    clocksync,      true),      // Clocks advanced relative to real time

    OPT_F("Sound",        sound,          true),      // Sound enabled
    OPT_F("SAASound",     saasound,       true),      // SAA 1099 sound chip enabled
    OPT_F("Beeper",       beeper,         true),      // Spectrum-style beeper enabled

    OPT_F("Stereo",       stereo,         true),      // Stereo sound
    OPT_N("Latency",      latency,        5),         // Sound latency of five frames

    OPT_N("DriveLights",  drivelights,    0),         // Show drive activity lights
    OPT_N("Profile",      profile,        0),         // Show only speed and framerate
    OPT_F("Status",       status,       false),      // Show status line for changed options, etc.

    OPT_F("PauseInactive",pauseinactive,  false),     // Continue to run when inactive

    OPT_S("FnKeys",       fnkeys,
     "F1=1,SF1=2,AF1=0,CF1=3,F2=5,SF2=6,AF2=4,CF2=7,F3=30,F4=11,SF4=12,AF4=8,F5=25,SF5=23,F6=26,F7=21,F8=22,F9=14,SF9=13,F10=9,SF10=10,F11=16,F12=15,CF12=8"),

    { NULL, 0 }
};

inline bool IsTrue (const char* pcsz_)
{
    return pcsz_ && (!strcasecmp(pcsz_, "true") || !strcasecmp(pcsz_, "on") || !strcasecmp(pcsz_, "enabled") || 
                    !strcasecmp(pcsz_, "yes") || !strcasecmp(pcsz_, "1"));
}

// Find a named option in the options list above
OPTION* FindOption (const char* pcszName_)
{
    for (OPTION* p = aOptions ; p->pcszName ; p++)
    {
        if (!strcasecmp(pcszName_, p->pcszName))
            return p;
    }

    return NULL;
}

// Set (optionally unspecified) options to their default values
void Options::SetDefaults (bool fForce_/*=true*/)
{
    // Process the full options list
    for (OPTION* p = aOptions ; p->pcszName ; p++)
    {
        // Set the default if forcing defaults, or if we've not already 
        if (fForce_ || !p->fSpecified)
        {
            switch (p->nType)
            {
                case OT_BOOL:       *p->pf = p->fDefault;   break;
                case OT_INT:        *p->pn = p->nDefault;   break;
                case OT_STRING:     strcpy(p->ppsz, p->pcszDefault);   break;
            }
        }
    }

    // Force the current compatability number
    SetOption(cfgversion,CFG_VERSION);

    //LUDO:
    Options::s_Options.snd_enable = 1;
    Options::s_Options.speed_limiter = 50;
    Options::s_Options.display_lr = 0;
    Options::s_Options.view_fps = 0;
    Options::s_Options.render_mode = 0;
    Options::s_Options.render_x = 0;
    Options::s_Options.render_y = 0;
    Options::s_Options.psp_reverse_analog = 0;
    Options::s_Options.psp_cpu_clock = 222;
    Options::s_Options.psp_screenshot_id = 0;
    Options::s_Options.frameskip = 0;
    getcwd(Options::s_Options.home_dir, MAX_PATH);

    sim_update_save_name("");

    sim_load_settings();

    scePowerSetClockFrequency(Options::s_Options.psp_cpu_clock, 
                              Options::s_Options.psp_cpu_clock, 
                              Options::s_Options.psp_cpu_clock/2);
# if 0
    save_used[SIM_MAX_SAVE_STATE];
# endif
}

// Find the address of the variable holding the specified option default
void* Options::GetDefault (const char* pcszName_)
{
    OPTION* p = FindOption(pcszName_);

    if (p)
    {
        switch (p->nType)
        {
            case OT_BOOL:       return &p->fDefault;
            case OT_INT:        return &p->nDefault;
        }
    }

    // This should never happen, thanks to a compile-time check in the header
    static void* pv = NULL;
    return &pv;
}

bool Options::Load (int argc_, char* argv_[])
{
    // Set the default values for any missing options, or all if the config version has changed
    bool fIncompatible = GetOption(cfgversion) != CFG_VERSION;
    SetDefaults(fIncompatible);

    return true;
}


bool Options::Save ()
{
    return true;
}
