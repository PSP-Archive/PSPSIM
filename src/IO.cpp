// Part of SimCoupe - A SAM Coupe emulator
//
// IO.cpp: SAM I/O port handling
//
//  Copyright (c) 1999-2006  Simon Owen
//  Copyright (c) 1996-2001  Allan Skillman
//  Copyright (c) 2000-2001  Dave Laundon
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

// Changes 1999-2003 by Simon Owen
//  - hooked up newly supported hardware from other modules
//  - most ports now correctly initialised to zero on reset (allow reset screens)
//  - palette only pre-initialised on power-on
//  - HPEN and LPEN now correct
//  - ATTR port now allows on/off-screen detection (onscreen always zero tho)
//  - clut values now cached, to allow runtime changes in pixel format

// Changes 2000-2001 by Dave Laundon
//  - mode change done in two steps, with mode changed one block after page
//  - line interrupt now correctly activated/deactivated on port writes

// ToDo:
//  - tidy up a bit, and handle drive reinitialiseation in IO::Init() better

#include "SimCoupe.h"
#include "IO.h"

#include "Atom.h"
#include "CDrive.h"
#include "Clock.h"
#include "CPU.h"
#include "Floppy.h"
#include "Frame.h"
#include "GUI.h"
#include "HardDisk.h"
#include "Input.h"
#include "Memory.h"
#include "MIDI.h"
#include "Mouse.h"
#include "Options.h"
#include "Parallel.h"
#include "SAMDOS.h"
#include "SDIDE.h"
#include "Sound.h"
#include "Util.h"
#include "Video.h"
#include "YATBus.h"

#ifdef USE_TESTHW
#include "TestHW.cpp"
#endif

extern int g_nLine;
extern int g_nLineCycle;

CDiskDevice *pDrive1, *pDrive2, *pSDIDE, *pYATBus, *pBootDrive;
CIoDevice *pParallel1, *pParallel2;
CIoDevice *pSerial1, *pSerial2;
CIoDevice *pSambus, *pDallas;
CIoDevice *pMidi;
CIoDevice *pBeeper;

// Port read/write addresses for I/O breakpoints
WORD wPortRead, wPortWrite;

// Paging ports for internal and external memory
BYTE vmpr, hmpr, lmpr, lepr, hepr;
BYTE vmpr_mode, vmpr_page1, vmpr_page2;

BYTE border, border_col;

BYTE keyboard;
BYTE status_reg;
BYTE line_int;

BYTE lpen;

UINT clut[N_CLUT_REGS], clutval[N_CLUT_REGS], mode3clutval[4];

BYTE keyports[9];       // 8 rows of keys (+ 1 row for unscanned keys)
BYTE keybuffer[9];      // working buffer for key changed, activated mid-frame
bool fInputDirty;       // true if the input has been modified since the last frame

bool fASICStartup;      // If set, the ASIC will be unresponsive shortly after first power-on
bool g_fAutoBoot;       // Auto-boot the disk in drive 1 when we're at the startup screen


bool IO::Init (bool fFirstInit_/*=false*/)
{
    bool fRet = true;
    Exit(true);

    lepr = hepr = lpen = border = 0;

    OutLmpr(0);     // Page 0 in section C, page 1 in section D
    OutHmpr(0);     // Page 0 in section A, page 1 in section B, ROM0 on, ROM off
    OutVmpr(0);     // Video in page 0, screen mode 1

    // No extended keys pressed, no active interrupts
    status_reg = 0xff;


    // Also, if this is a power-on initialisation, set up the clut, etc.
    if (fFirstInit_)
    {
        // Set up the unresponsive ASIC unresponsive option for initial startup
        fASICStartup = GetOption(asicdelay);

        // Line interrupts aren't cleared by a reset
        line_int = 0xff;

        // No keys pressed initially
        ReleaseAllSamKeys();

        // Initialise all the devices
        fRet &= (InitDrives() && InitParallel() && InitSerial() && InitClocks() &&
                 InitMidi() && InitBeeper() && InitHDD());
    }

    // Initialise the drives back to a consistent state
    pDrive1->Reset();
    pDrive2->Reset();

    // Return true only if everything
    return fRet;
}

void IO::Exit (bool fReInit_/*=false*/)
{
    if (!fReInit_)
    {
        InitDrives(false, false);
        InitParallel(false, false);
        InitSerial(false, false);
        InitClocks(false, false);
        InitMidi(false, false);
        InitBeeper(false, false);
        InitHDD(false, false);
    }
}


bool IO::InitDrives (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    // The boot drive should never exist here, but we'll clean up if it does
    if (pBootDrive)
    {
        delete pDrive1;
        pDrive1 = pBootDrive;
        pBootDrive = NULL;
    }

    if (pDrive1 && ((!fInit_ && !fReInit_) || (pDrive1->GetType() != GetOption(drive1))))
    {
        if (pDrive1->GetType() == dskImage)
            SetOption(disk1, pDrive1->GetPath());
        delete pDrive1; pDrive1 = NULL;
    }

    if (pDrive2 && ((!fInit_ && !fReInit_) || (pDrive2->GetType() != GetOption(drive2))))
    {
        if (pDrive2->GetType() == dskImage)
            SetOption(disk2, pDrive2->GetPath());
        else if (pDrive2->GetType() == dskAtom)
            SetOption(atomdisk, pDrive2->GetPath());
        delete pDrive2; pDrive2 = NULL;
    }

    if (fInit_)
    {
        if (!pDrive1)
        {
            switch (GetOption(drive1))
            {
                case dskImage:
                    (pDrive1 = new CDrive())->Insert(GetOption(disk1));
                    break;

                default:
                    pDrive1 = new CDiskDevice;
                    break;
            }
        }

        if (!pDrive2)
        {
            switch (GetOption(drive2))
            {
                case dskImage:
                    (pDrive2 = new CDrive())->Insert(GetOption(disk2));
                    break;

                case dskAtom:
                {
                    CHardDisk* pDisk = CHardDisk::OpenObject(GetOption(atomdisk));
                    pDrive2 = pDisk ? new CAtomDiskDevice(pDisk) : new CDiskDevice;
                    break;
                }

                default:
                    pDrive2 = new CDiskDevice;
                    break;
            }
        }
    }

    return !fInit_ || (pDrive1 && pDrive2);
}

bool IO::InitParallel (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    delete pParallel1; pParallel1 = NULL;
    delete pParallel2; pParallel2 = NULL;

    if (fInit_)
    {
        switch (GetOption(parallel1))
        {
            case 1:
                if ((*GetOption(printerdev)))
                    pParallel1 =  new CPrinterDevice;
                else
                    pParallel1 = new CPrinterFile;
                break;

            case 2:     pParallel1 = new CMonoDACDevice;    break;
            case 3:     pParallel1 = new CStereoDACDevice;  break;
            default:    pParallel1 = new CIoDevice;         break;
        }

        switch (GetOption(parallel2))
        {
            case 1:
                if ((*GetOption(printerdev)))
                    pParallel2 =  new CPrinterDevice;
                else
                    pParallel2 = new CPrinterFile;
                break;

            case 2:     pParallel2 = new CMonoDACDevice;    break;
            case 3:     pParallel2 = new CStereoDACDevice;  break;
            default:    pParallel2 = new CIoDevice;         break;
        }
    }

    return !fInit_ || (pParallel1 && pParallel2);
}

bool IO::InitSerial (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    delete pSerial1; pSerial1 = NULL;
    delete pSerial2; pSerial2 = NULL;

    if (fInit_)
    {
        // Serial ports are dummy for now
        pSerial1 = new CIoDevice;
        pSerial2 = new CIoDevice;
    }

    return !fInit_ || (pSerial1 && pSerial2);
}

bool IO::InitClocks (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    delete pSambus; pSambus = NULL;
    delete pDallas; pDallas = NULL;

    if (fInit_)
    {
        pSambus = GetOption(sambusclock) ? new CSambusClock : new CIoDevice;
        pDallas = GetOption(dallasclock) ? new CDallasClock : new CIoDevice;
    }

    return !fInit_ || (pSambus && pDallas);
}

bool IO::InitMidi (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    delete pMidi; pMidi = NULL;

    if (fInit_)
    {
        switch (GetOption(midi))
        {
            case 1:     pMidi = new CMidiDevice;    break;
            default:    pMidi = new CIoDevice;      break;
        }
    }

    return !fInit_ || pMidi;
}

bool IO::InitBeeper (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    delete pBeeper; pBeeper = NULL;

    if (fInit_)
        pBeeper = GetOption(beeper) ? new CBeeperDevice : new CIoDevice;

    return fInit_ || pBeeper;
}

bool IO::InitHDD (bool fInit_/*=true*/, bool fReInit_/*=true*/)
{
    if (pSDIDE)
    {
        SetOption(sdidedisk, pSDIDE->GetPath());
        delete pSDIDE; pSDIDE = NULL;
    }

    if (pYATBus)
    {
        SetOption(yatbusdisk, pYATBus->GetPath());
        delete pYATBus; pYATBus = NULL;
    }

    if (fInit_)
    {
        CHardDisk* pDisk = CHardDisk::OpenObject(GetOption(sdidedisk));
        pSDIDE = pDisk ? new CSDIDEDevice(pDisk) : new CDiskDevice;

        pDisk = CHardDisk::OpenObject(GetOption(yatbusdisk));
        pYATBus = pDisk ? new CYATBusDevice(pDisk) : new CDiskDevice;
    }

    return fInit_ || (pSDIDE && pYATBus);
}

////////////////////////////////////////////////////////////////////////////////

static inline void PaletteChange (BYTE bHMPR_)
{
    // Update the 4 colours available to mode 3 (note: the middle colours are switched)
    BYTE mode3_bcd48 = (bHMPR_ & HMPR_MD3COL_MASK) >> 3;
    mode3clutval[0] = clutval[mode3_bcd48 | 0];
    mode3clutval[1] = clutval[mode3_bcd48 | 2];
    mode3clutval[2] = clutval[mode3_bcd48 | 1];
    mode3clutval[3] = clutval[mode3_bcd48 | 3];
}


void IO::OutLmpr(BYTE val)
{
    lmpr = val;

    // Put either RAM or ROM 0 into section A
    if (lmpr & LMPR_ROM0_OFF)
        PageIn(SECTION_A, LMPR_PAGE);
    else
        PageIn(SECTION_A, ROM0);

    // Put the relevant bank into section B
    PageIn(SECTION_B, (LMPR_PAGE + 1) & LMPR_PAGE_MASK);

    // Put either RAM or ROM 1 into section D
    if (lmpr & LMPR_ROM1)
        PageIn(SECTION_D, ROM1);
    else
        PageIn(SECTION_D, (HMPR_PAGE + 1) & HMPR_PAGE_MASK);
}

void IO::OutHmpr (BYTE bVal_)
{
    // Have the mode3 BCD4/8 bits changed?
    if ((hmpr ^ bVal_) & HMPR_MD3COL_MASK)
    {
        // The changes are effective immediately in mode 3
        if (vmpr_mode == MODE_3)
            Frame::Update();

        // Update the mode 3 colours
        PaletteChange(bVal_);
    }

    // Update the HMPR
    hmpr = bVal_;

    // Put the relevant RAM bank into section C
    PageIn(SECTION_C, HMPR_PAGE);

    // Put either RAM or ROM1 into section D
    if (lmpr & LMPR_ROM1)
        PageIn(SECTION_D, ROM1);
    else
        PageIn(SECTION_D,(HMPR_PAGE + 1) & HMPR_PAGE_MASK);

    // External RAM has priority, even over ROM1
    if (HMPR_MCNTRL)
    {
        OutLepr(lepr);
        OutHepr(hepr);
    }
}

void IO::OutVmpr (BYTE bVal_)
{
    // The ASIC changes mode before page, so consider an on-screen artifact from the mode change
    Frame::ChangeMode(bVal_);

    vmpr = (bVal_ & (VMPR_MODE_MASK|VMPR_PAGE_MASK));
    vmpr_mode = vmpr & VMPR_MODE_MASK;

    // Extract the page number(s) for faster access by the memory writing functions
    vmpr_page1 = VMPR_PAGE;
    vmpr_page2 = (vmpr_page1+1) & VMPR_PAGE_MASK;
}

void IO::OutLepr (BYTE bVal_)
{
    lepr = bVal_;

    if (HMPR_MCNTRL)
        PageIn(SECTION_C, EXTMEM+bVal_);
}

void IO::OutHepr (BYTE bVal_)
{
    hepr = bVal_;

    if (HMPR_MCNTRL)
        PageIn(SECTION_D, EXTMEM+bVal_);
}


void IO::OutClut (WORD wPort_, BYTE bVal_)
{
    wPort_ &= (N_CLUT_REGS-1);          // 16 clut registers, so only the bottom 4 bits are significant
    bVal_ &= (N_PALETTE_COLOURS-1);     // 128 colours, so only the bottom 7 bits are significant

    // Has the clut value actually changed?
    if (clut[wPort_] != aulPalette[bVal_])
    {
        // Draw up to the current point with the previous settings
        Frame::Update();

        // Update the clut entry and the mode 3 palette
        clut[wPort_] = static_cast<DWORD>(aulPalette[clutval[wPort_] = bVal_]);
        PaletteChange(hmpr);
    }
}


BYTE IO::In (WORD wPort_)
{
    BYTE bPortLow = (wPortRead = wPort_) & 0xff;

    // The ASIC doesn't respond to I/O immediately after power-on
    if (fASICStartup && (fASICStartup = g_dwCycleCounter < ASIC_STARTUP_DELAY) && bPortLow >= BASE_ASIC_PORT)
        return 0x00;

    switch (bPortLow)
    {
        // keyboard 1 / mouse
        case KEYBOARD_PORT:
        {
            int highbyte = wPort_ >> 8, res = 0x1f;

            if (highbyte == 0xff)
            {
                res = keyports[8] & 0x1f;

                if (GetOption(mouse) && res == 0x1f)
                    res = Mouse::Read(g_dwCycleCounter) & 0x1f;
            }
            else
            {
                if (!(highbyte & 0x80)) res &= keyports[7] & 0x1f;
                if (!(highbyte & 0x40)) res &= keyports[6] & 0x1f;
                if (!(highbyte & 0x20)) res &= keyports[5] & 0x1f;
                if (!(highbyte & 0x10)) res &= keyports[4] & 0x1f;
                if (!(highbyte & 0x08)) res &= keyports[3] & 0x1f;
                if (!(highbyte & 0x04)) res &= keyports[2] & 0x1f;
                if (!(highbyte & 0x02)) res &= keyports[1] & 0x1f;
                if (!(highbyte & 0x01)) res &= keyports[0] & 0x1f;
            }

            return (keyboard & 0xe0) | res;
        }

        // keyboard 2
        case STATUS_PORT:
        {
            // Add on the instruction time to the line counter and the global counter
            // This will make sure the value in status_reg is up to date
            CheckCpuEvents();

            int highbyte = wPort_ >> 8, res = 0xe0;

            if (!(highbyte & 0x80)) res &= keyports[7] & 0xe0;
            if (!(highbyte & 0x40)) res &= keyports[6] & 0xe0;
            if (!(highbyte & 0x20)) res &= keyports[5] & 0xe0;
            if (!(highbyte & 0x10)) res &= keyports[4] & 0xe0;
            if (!(highbyte & 0x08)) res &= keyports[3] & 0xe0;
            if (!(highbyte & 0x04)) res &= keyports[2] & 0xe0;
            if (!(highbyte & 0x02)) res &= keyports[1] & 0xe0;
            if (!(highbyte & 0x01)) res &= keyports[0] & 0xe0;

            return (res & 0xe0) | (status_reg & 0x1f);
        }


        // Banked memory management
        case VMPR_PORT:     return vmpr | 0x80;     // RXMIDI bit always one for now
        case HMPR_PORT:     return hmpr;
        case LMPR_PORT:     return lmpr;

        // SAMBUS and DALLAS clock ports
        case CLOCK_PORT:    return (wPort_ < 0xfe00) ? pSambus->In(wPort_) : pDallas->In(wPort_);

        // HPEN and LPEN ports
        case LPEN_PORT:
        {
            if ((wPort_ & PEN_MASK) == LPEN_PORT)
            {
                // LPEN reflects the horizontal scan position in the main screen area only
                BYTE bX = (g_nLine < TOP_BORDER_LINES || g_nLine >= (TOP_BORDER_LINES+SCREEN_LINES) ||
                           g_nLineCycle < BORDER_PIXELS || g_nLineCycle >= (BORDER_PIXELS+SCREEN_PIXELS)) ? 0 :
                            static_cast<BYTE>(g_nLineCycle - BORDER_PIXELS);

                // Take the top 6 bits from the position, and the rest from the existing value
                return (bX & 0xfc) | (lpen & 0x03);
            }
            else
            {
                // HPEN treats the start of a line from the right border area, so adjust for it
                int nLine = g_nLine + (g_nLineCycle + BORDER_PIXELS) / TSTATES_PER_LINE;

                // Return 192 for the top/bottom border areas, or the main screen line number
                return (nLine < TOP_BORDER_LINES || nLine >= (TOP_BORDER_LINES+SCREEN_LINES)) ?
                    static_cast<BYTE>(SCREEN_LINES) : nLine - TOP_BORDER_LINES;
            }
        }

        // Spectrum ATTR port
        case ATTR_PORT:
        {
            // Border lines return 0xff
            if (g_nLine < TOP_BORDER_LINES && g_nLine >= (TOP_BORDER_LINES+SCREEN_LINES))
                return 0xff;

            // Strictly we need to check the mode and return the appropriate value, but for now we'll return zero
            return 0x00;
        }

        // Parallel ports
        case PRINTL1_STAT:
        case PRINTL1_DATA:
            return pParallel1->In(wPort_);

        case PRINTL2_STAT:
        case PRINTL2_DATA:
            return pParallel2->In(wPort_);

        // Serial ports
        case SERIAL1:
            return pSerial1->In(wPort_);

        case SERIAL2:
            return pSerial2->In(wPort_);

        // MIDI IN/Network
        case MIDI_PORT:
            return pMidi->In(wPort_);

        // S D Software IDE interface
        case SDIDE_REG:
        case SDIDE_DATA:
            return pSDIDE->In(wPort_);

        default:
        {
            // Floppy drive 1
            if ((wPort_ & FLOPPY_MASK) == FLOPPY1_BASE)
            {
                // Read from floppy drive 1, if present
                if (GetOption(drive1))
                    return pDrive1->In(wPort_);
            }

            // Floppy drive 2 *OR* the ATOM hard disk
            else if ((wPort_ & FLOPPY_MASK) == FLOPPY2_BASE)
            {
                // Read from floppy drive 2 or Atom hard disk, if present
                if (GetOption(drive2))
                    return pDrive2->In(wPort_);
            }

            // YAMOD.ATBUS hard disk interface
            else if ((bPortLow & YATBUS_MASK) == YATBUS_BASE)
                return pYATBus->In(wPort_);

            // Only unsupported hardware should reach here
            else
            {
#ifdef USE_TESTHW
                return TestHW::In(wPort_);
#else
                TRACE("*** Unhandled read: %#04x (%d)\n", wPort_, wPort_&0xff);
#endif
            }
        }
    }

    // Default to returning 0xff
    return 0xff;
}


// The actual port input and output routines
void IO::Out (WORD wPort_, BYTE bVal_)
{
    BYTE bPortLow = (wPortWrite = wPort_) & 0xff;

    // The ASIC doesn't respond to I/O immediately after power-on
    if (fASICStartup && (fASICStartup = g_dwCycleCounter < ASIC_STARTUP_DELAY) && bPortLow >= BASE_ASIC_PORT)
        return;

    switch (bPortLow)
    {
        case BORDER_PORT:
        {
            bool fScreenOffChange = ((border ^ bVal_) & BORD_SOFF_MASK) && VMPR_MODE_3_OR_4;

            // Has the border colour has changed colour or the screen been enabled/disabled?
            if (fScreenOffChange || ((border ^ bVal_) & BORD_COLOUR_MASK))
                Frame::Update();

            // If the screen enable state has changed, consider a border change artefact
            if (fScreenOffChange && (border & BORD_SOFF))
                Frame::ChangeScreen(bVal_);

            // If the speaker bit has been toggled, generate a click
            if ((border ^ bVal_) & BORD_BEEP_MASK)
                pBeeper->Out(wPort_, bVal_);

            // Store the new border value and extract the border colour for faster access by the video routines
            border = bVal_;
            border_col = BORD_VAL(bVal_);

            // Update the port read value, including the screen off status and reflecting MIC back to EAR
            keyboard = (keyboard & BORD_KEY_MASK) | (border & BORD_SOFF) | ((border & BORD_MIC_MASK) << 3);

            // If the screen state has changed, we need to reconsider memory contention changes
            if (fScreenOffChange)
                CPU::UpdateContention();
        }
        break;

        // Banked memory management
        case VMPR_PORT:
        {
            // Changes to screen mode and screen page are visible at different times

            // Has the screen mode changed?
            if (vmpr_mode != (bVal_ & VMPR_MODE_MASK))
            {
                // Are either the current mode or the new mode 3 or 4?  i.e. bit MDE1 is set
                if ((bVal_ | vmpr) & VMPR_MDE1_MASK)
                {
                    // Changes to the screen MODE are visible straight away
                    Frame::Update();

                    // Change only the screen MODE for the transition block
                    OutVmpr((bVal_ & VMPR_MODE_MASK) | (vmpr & ~VMPR_MODE_MASK));
                }
                // Otherwise both modes are 1 or 2
                else
                {
                    // There are no visible changes in the transition block
                    g_nLineCycle += VIDEO_DELAY;
                    Frame::Update();
                    g_nLineCycle -= VIDEO_DELAY;

                    // Do the whole change here - the check below will not be triggered
                    OutVmpr(bVal_);
                }

                // Video mode change may have affected memory contention
                CPU::UpdateContention();
            }

            // Has the screen page changed?
            if (vmpr_page1 != (bVal_ & VMPR_PAGE_MASK))
            {
                // Changes to screen PAGE aren't visible until 8 tstates later
                // as the memory has been read by the ASIC already
                g_nLineCycle += VIDEO_DELAY;
                Frame::Update();
                g_nLineCycle -= VIDEO_DELAY;

                OutVmpr(bVal_);
            }
        }
        break;

        case HMPR_PORT:
            if (hmpr != bVal_)
                OutHmpr(bVal_);
            break;

        case LMPR_PORT:
            if (lmpr != bVal_)
                OutLmpr(bVal_);
            break;

        // SAMBUS and DALLAS clock ports
        case CLOCK_PORT:
            if (wPort_ < 0xfe00)
                pSambus->Out(wPort_, bVal_);
            else
                pDallas->Out(wPort_, bVal_);
            break;

        case CLUT_BASE_PORT:
            OutClut(wPort_ >> 8, bVal_);
            break;

        // External memory
        case HEPR_PORT:
            OutHepr(bVal_);
            break;

        case LEPR_PORT:
            OutLepr(bVal_);
            break;

        case LINE_PORT:
            // Line changed?
            if (line_int != bVal_)
            {
                // Set the new value
                line_int = bVal_;

                // Because of this change, does the LINE interrupt signal now need to be active, or not?
                if (line_int < SCREEN_LINES)
                {
                    // Offset Line and LineCycle from the point in the scan line that interrupts occur
                    int nLine_ = g_nLine, nLineCycle_ = g_nLineCycle - INT_START_TIME;
                    if (nLineCycle_ < 0)
                        nLineCycle_ += TSTATES_PER_LINE;
                    else
                        nLine_++;

                    // Make sure timing and events are right up-to-date
                    CheckCpuEvents();

                    // Does the interrupt need to be active?
                    if ((nLine_ == (line_int + TOP_BORDER_LINES)) && (nLineCycle_ < INT_ACTIVE_TIME))
                    {
                        // If the interrupt is already active then we have only just passed into the right border
                        // and the CheckCpuEvents above has taken care of it, so do nothing
                        if (status_reg & STATUS_INT_LINE)
                        {
                            // Otherwise, set the interrupt ourselves and create an event to end it
                            status_reg &= ~STATUS_INT_LINE;
                            AddCpuEvent(evtStdIntEnd, g_dwCycleCounter - nLineCycle_ + INT_ACTIVE_TIME);
                        }
                    }
                    else
                        // Else make sure it isn't active
                        status_reg |= STATUS_INT_LINE;
                }
                else
                    // Else make sure it isn't active
                    status_reg |= STATUS_INT_LINE;
            }
            break;

        case SOUND_DATA:
            Sound::Out(wPort_, bVal_);
            break;

        // Parallel ports 1 and 2
        case PRINTL1_STAT:
        case PRINTL1_DATA:
            pParallel1->Out(wPort_, bVal_);
            break;

        case PRINTL2_STAT:
        case PRINTL2_DATA:
            pParallel2->Out(wPort_, bVal_);
            break;


        // Serial ports 1 and 2
        case SERIAL1:
            pSerial1->Out(wPort_, bVal_);
            break;

        case SERIAL2:
            pSerial2->Out(wPort_, bVal_);
            break;


        // MIDI OUT/Network
        case MIDI_PORT:
        {
            // Add on the instruction time to the line counter and the global counter
            // This will make sure the value in lpen is up to date
            CheckCpuEvents();

            // Only transmit a new byte if one isn't already being sent
            if (!(lpen & LPEN_TXFMST))
            {
                // Set the TXFMST bit in LPEN to show that we're transmitting something
                lpen |= LPEN_TXFMST;

                // Create an event to begin an interrupt at the required time
                AddCpuEvent(evtMidiOutIntStart, g_dwCycleCounter +
                            A_ROUND(MIDI_TRANSMIT_TIME + 16, 32) - 16 - 32 - MIDI_INT_ACTIVE_TIME + 2);

                // Output the byte using the platform specific implementation
                pMidi->Out(wPort_, bVal_);
            }
            break;
        }

        // S D Software IDE interface
        case SDIDE_REG:
        case SDIDE_DATA:
            pSDIDE->Out(wPort_, bVal_);
            break;

        default:
        {
            // Floppy drive 1
            if ((wPort_ & FLOPPY_MASK) == FLOPPY1_BASE)
            {
                // Write to floppy drive 1, if present
                if (GetOption(drive1))
                    pDrive1->Out(wPort_, bVal_);
            }

            // Floppy drive 2 *OR* the ATOM hard disk
            else if ((wPort_ & FLOPPY_MASK) == FLOPPY2_BASE)
            {
                // Write to floppy drive 2 or Atom hard disk, if present...
                if (GetOption(drive2))
                    pDrive2->Out(wPort_, bVal_);
            }

            // YAMOD.ATBUS hard disk interface
            else if ((bPortLow & YATBUS_MASK) == YATBUS_BASE)
                pYATBus->Out(wPort_, bVal_);

            // Only unsupported hardware should reach here
            else
            {
#ifdef USE_TESTHW
                TestHW::Out(wPort_, bVal_);
#else
                TRACE("*** Unhandled write: %#06x (LSB=%d), %#02x (%d)\n", wPort_, bPortLow, bVal_, bVal_);
#endif
            }
        }
    }
}

void IO::FrameUpdate ()
{
    pDrive1->FrameEnd();
    pDrive2->FrameEnd();
    pParallel1->FrameEnd();
    pParallel2->FrameEnd();

    CClockDevice::FrameUpdate();
    Sound::FrameUpdate();
    Input::Update();
}

void IO::UpdateInput()
{
    // To avoid accidents, purge keyboard input during accelerated disk access
    if (GetOption(turboload) && (pDrive1 && pDrive1->IsActive()) || (pDrive2 && pDrive2->IsActive()))
        Input::Purge(false);

    // Non-zero to tap the F9 key
    static int nAutoBoot = 0;

    // If an auto-boot is required, make sure we're at the stripey startup screen
    if (g_fAutoBoot && IsAtStartupScreen())
    {
        g_fAutoBoot = false;
        nAutoBoot = 10 * pDrive1->IsInserted();
    }

    // If actively auto-booting, press and release F9 to trigger the boot
    if (nAutoBoot)
    {
        if (--nAutoBoot < 5)
            PressSamKey(SK_F9);
        else
            ReleaseSamKey(SK_F9);
    }

    if (fInputDirty)
    {
        // Copy the working buffer to the live port buffer
        memcpy(keyports, keybuffer, sizeof keyports);
        fInputDirty = false;
    }
}

const RGBA* IO::GetPalette (bool fDimmed_/*=false*/)
{
    static RGBA asPalette[N_PALETTE_COLOURS];

    // Look-up table for an even intensity spread, used to map SAM colours to RGB
    static const BYTE abIntensities[] = { 0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff };

    // Build the full palette: SAM's 128 colours and the extra colours for the GUI
    for (int i = 0; i < N_PALETTE_COLOURS ; i++)
    {
        // Convert from SAM palette position to 8-bit RGB
        BYTE bRed   = abIntensities[(i&0x02)     | ((i&0x20) >> 3) | ((i&0x08) >> 3)];
        BYTE bGreen = abIntensities[(i&0x04) >> 1| ((i&0x40) >> 4) | ((i&0x08) >> 3)];
        BYTE bBlue  = abIntensities[(i&0x01) << 1| ((i&0x10) >> 2) | ((i&0x08) >> 3)];

        // Dim if required
        if (fDimmed_)
        {
            bRed   = bRed   * 2/3;
            bGreen = bGreen * 2/3;
            bBlue  = bBlue  * 2/3;
        }

        // If greyscale is enabled, convert the colour a suitable intensity grey
        if (GetOption(greyscale))
        {
            BYTE bGrey = static_cast<BYTE>(0.30 * bRed + 0.59 * bGreen + 0.11 * bBlue);
            bRed = bGreen = bBlue = bGrey;
        }

        // Store the calculated values for the entry
        asPalette[i].bRed = bRed;
        asPalette[i].bGreen = bGreen;
        asPalette[i].bBlue = bBlue;
        asPalette[i].bAlpha = 0xff;
    }

    // Return the freshly prepared palette
    return asPalette;
}

// Check if we're at the stripey SAM startup screen
bool IO::IsAtStartupScreen ()
{
    // Use physical locations so we're not sensitive to the current paging state
    BYTE* pbPalette = &apbPageReadPtrs[0][0x55d9 - 0x4000];
    BYTE* pb = &apbPageReadPtrs[0][0x5600 - 0x4000];

    // There are 16 stripes to check
    for (int i = 0 ; i < 16 ; i++)
    {
        // Check the line interrupt position, CLUT value, and both colours
        if (*pb++ != (i*11) || *pb++ != 0x00 || *pb++ != *pbPalette || *pb++ != *pbPalette++)
            return false;
    }

    return true;
}

void IO::CheckAutoboot ()
{
    // Trigger autoboot if we're at the startup screen right now
    g_fAutoBoot |= GetOption(autoboot) && IO::IsAtStartupScreen();
}


bool IO::Rst8Hook ()
{
    // If a drive object exists, clean up after our boot attempt (which could fail if we're given a bad image)
    if (pBootDrive)
    {
        delete pDrive1;
        pDrive1 = pBootDrive;
        pBootDrive = NULL;
    }

    // Are we about to trigger "NO DOS" in ROM1, and with DOS booting enabled?
    else if (regs.PC.W == 0xd977 && GetSectionPage(SECTION_D) == ROM1 && GetOption(dosboot))
    {
        // If there's a custom boot disk, load it read-only
        CDisk* pDisk = CDisk::Open(GetOption(dosdisk), true);

        // Fall back on the built-in SAMDOS2 image
        if (!pDisk)
            pDisk = CDisk::Open(abSAMDOS, sizeof(abSAMDOS), "mem:SAMDOS.sbt");

        if (pDisk)
        {
            // Switch to the temporary boot image
            pBootDrive = pDrive1;
            pDrive1 = new CDrive(pDisk);

            // If successful, 
            if (pDrive1)
            {
                // Jump back to BOOTEX to try again, and return that we processed the RST
                regs.PC.W = 0xd8e5;
                return true;
            }
            else
            {
                delete pDisk;
                pDrive1 = pBootDrive;
                pBootDrive = NULL;
            }
        }
    }

    // RST not processed
    return false;
}
