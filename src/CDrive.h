// Part of SimCoupe - A SAM Coupe emulator
//
// CDrive.h: VL1772-02 floppy disk controller emulation
//
//  Copyright (c) 1999-2006  Simon Owen
//  Copyright (c) 1996-2001  Allan Skillman
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

#ifndef CDRIVE_H
#define CDRIVE_H

#include "IO.h"
#include "Options.h"

#include "VL1772.h"
#include "CDisk.h"


// Time motor stays on after no further activity:  10 revolutions at 300rpm (2 seconds)
const int FLOPPY_MOTOR_ACTIVE_TIME = (10 / (FLOPPY_RPM/60)) * EMULATED_FRAMES_PER_SECOND;

class CDrive : public CDiskDevice
{
    public:
        CDrive (CDisk* pDisk_=NULL);
        ~CDrive () { Eject(); }

    public:
        BYTE In (WORD wPort_);
        void Out (WORD wPort_, BYTE bVal_);
        void FrameEnd ();

    public:
        BYTE ReadAddress (UINT uSide_, UINT uTrack_, IDFIELD* pIdField_);
        UINT ReadTrack (UINT uSide_, UINT uTrack_, BYTE* pbTrack_, UINT uSize_);
        BYTE VerifyTrack (UINT uSide_, UINT uTrack_);
        BYTE WriteTrack (UINT uSide_, UINT uTrack_, BYTE* pbTrack_, UINT uSize_);

    public:
        bool Insert (const char* pcszSource_, bool fReadOnly_=false);
        void Eject ();
        bool Save () { return (m_pDisk && m_pDisk->IsModified()) ? m_pDisk->Save() : true; }
        void Reset ();

    public:
        int GetDiskType () const { return m_pDisk ? m_pDisk->GetType() : dtNone; }
        const char* GetPath () const { return m_pDisk ? m_pDisk->GetPath() : ""; }
        const char* GetFile () const { return m_pDisk ? m_pDisk->GetFile() : ""; }

        bool IsInserted () const { return !!m_pDisk; }
        bool IsModified () const { return m_pDisk && m_pDisk->IsModified(); }
        bool IsLightOn () const { return IsMotorOn() && m_pDisk; }
        bool IsActive () const { return IsLightOn () && m_nMotorDelay > (FLOPPY_MOTOR_ACTIVE_TIME - GetOption(turboload)); }

        void SetModified (bool fModified_=true) { if (m_pDisk) m_pDisk->SetModified(fModified_); }

    public:
        static WORD CrcBlock (const void* pcv_, size_t uLen_, WORD wCRC_=0xffff);

    protected:
        CDisk*      m_pDisk;        // The disk currently inserted in the drive, if any
        VL1772Regs  m_sRegs;        // VL1772 controller registers
        int         m_nHeadPos;     // Physical track the drive head is above

        BYTE        m_abBuffer[MAX_TRACK_SIZE]; // Buffer big enough for anything we'll read or write
        BYTE*       m_pbBuffer;
        UINT        m_uBuffer;
        BYTE        m_bDataStatus;  // Status value for end of data, where the data CRC can be checked

        int         m_nState;       // Command state, for tracking multi-stage execution
        int         m_nMotorDelay;  // Delay before switching motor off

    protected:
        void ModifyStatus (BYTE bEnable_, BYTE bReset_);
        void ModifyReadStatus ();
        void ExecuteNext ();

        bool IsMotorOn () const { return (m_sRegs.bStatus & MOTOR_ON) != 0; }
};

#endif
