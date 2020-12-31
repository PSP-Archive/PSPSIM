// Part of SimCoupe - A SAM Coupe emulator
//
// Parallel.cpp: Parallel interface
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

#ifndef PARALLEL_H
#define PARALLEL_H

#include "Sound.h"
#include "IO.h"


class CPrintBuffer : public CIoDevice
{
    public:
        CPrintBuffer () : m_fOpen(false), m_bPrint(0), m_bStatus(0), m_uBuffer(0), m_uFlushDelay(0) { }

    public:
        BYTE In (WORD wPort_);
        void Out (WORD wPort_, BYTE bVal_);
        void FrameEnd ();

        bool IsFlushable() const { return !!m_uBuffer; }
        void Flush ();

    protected:
        bool m_fOpen;
        BYTE m_bPrint, m_bStatus;

        UINT m_uBuffer, m_uFlushDelay;
        BYTE m_abBuffer[2048];

    protected:
        bool IsOpen () const { return false; }

        virtual bool Open () = 0;
        virtual void Close () = 0;
        virtual void Write (BYTE *pb_, size_t uLen_) = 0;
};


class CPrinterFile : public CPrintBuffer
{
    public:
        CPrinterFile () : m_hFile(NULL) { }
        ~CPrinterFile () { Close(); }

    public:
        bool Open ();
        void Close ();
        void Write (BYTE *pb_, size_t uLen_);

    protected:
        int m_nPrint;
        FILE *m_hFile;
};

class CPrinterDevice : public CPrintBuffer
{
    public:
        CPrinterDevice ();
        ~CPrinterDevice ();

    public:
        bool Open ();
        void Close ();
        void Write (BYTE *pb_, size_t uLen_);

    protected:
#ifdef WIN32
        HANDLE m_hPrinter;  // temporary!
#endif
};


class CMonoDACDevice : public CIoDevice
{
    public:
        void Out (WORD wPort_, BYTE bVal_);
};


class CStereoDACDevice : public CIoDevice
{
    public:
        CStereoDACDevice () : m_bVal(0x80) { }

    public:
        void Out (WORD wPort_, BYTE bVal_);

    protected:
        BYTE m_bVal;
};

#endif  // PARALLEL_H
