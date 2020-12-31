// Part of SimCoupe - A SAM Coupe emulator
//
// Floppy.cpp: SDL direct floppy access
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

// ToDo:
//  - implement custom track scanning, for copy-protected disks

#include "SimCoupe.h"

#include "Floppy.h"
#include "CDisk.h"
#include "Options.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__

#include <linux/fd.h>
#include <linux/fdreg.h>
#include <pthread.h>

void *thread_proc (void *pv_)
{
    return reinterpret_cast<CFloppyStream*>(pv_)->ThreadProc();
}

/*static*/ bool CFloppyStream::IsRecognised (const char* pcszStream_)
{
    struct stat st;
    char sz[MAX_PATH];
    int nMaxFollow = 10;

    // Work with a copy of the path as it may be updated to follow links
    strncpy(sz, pcszStream_, sizeof(sz));

    // Loop examining, in case there are links to follow
    while (!lstat(sz, &st))
    {
        // If it's a block device it must have a major of 2 (floppy)
        if (S_ISBLK(st.st_mode) && (st.st_rdev >> 8) == 2)
            return true;

        // Check for a link, and not too deep (or circular)
        if (!S_ISLNK(st.st_mode) || !--nMaxFollow)
            break;

        // Read the link target, failing if there's an error (no access or dangling link)
        char szLink[MAX_PATH];
        int nLen = readlink(sz, szLink, sizeof(szLink));
        if (nLen < 0)
            break;

        // Terminate the target string, as readlink doesn't do it
        szLink[nLen] = '\0';

        // Use absolute targets, or form the full path from the relative link
        if (szLink[0] == '/')
            strcpy(sz, szLink);
        else
        {
            strrchr(sz, '/')[1] = '\0';
            strcat(sz, szLink);
        }
    }

    // Not recognised
    return false;
}


bool CFloppyStream::Open ()
{
    if (!IsOpen())
    {
        // Open the floppy in the appropriate mode, falling back to read-only if necessary
        if (m_fReadOnly || (m_nFloppy = open(m_pszPath, O_EXCL|O_RDWR)) == -1)
        {
            m_nFloppy = open(m_pszPath, O_EXCL|O_RDONLY);
            m_fReadOnly = true;
        }

        // Set up the initial sector count if required
        m_uSectors = GetOption(stdfloppy) ? NORMAL_DISK_SECTORS : 0;
    }

    return IsOpen();
}

void CFloppyStream::Close ()
{
    // Back to the default setting when the device is closed
    m_uSectors = GetOption(stdfloppy) ? NORMAL_DISK_SECTORS : 0;
}


// Start executing a floppy command
BYTE CFloppyStream::StartCommand (BYTE bCommand_, PTRACK pTrack_, UINT uSector_, BYTE *pbData_)
{
    // Wait for any in-progress operation to complete
    BYTE bStatus;
    IsBusy(&bStatus, true);

    // Set up the command to perform
    m_bCommand = bCommand_;
    m_pTrack = pTrack_;
    m_uSector = uSector_;
    m_pbData = pbData_;
    m_bStatus = 0;

    // Create a new thread to perform it
    m_fThreadDone = false;
    return pthread_create(&m_hThread, NULL, thread_proc, (void*)this) ? LOST_DATA : BUSY;
}

bool CFloppyStream::IsBusy (BYTE* pbStatus_, bool fWait_)
{
    if (m_hThread)
    {
        void *pv;

        // If we're not to wait and the thread isn't complete, we're still busy
        if (!fWait_ && !m_fThreadDone)
            return true;

        // Reap the thread
        pthread_join(m_hThread, &pv);

        // Report the completion status, and invalidate the handles
        *pbStatus_ = m_bStatus;
        m_hThread = 0;
        m_bStatus = 0;
    }

    // Not busy
    return false;
}


// Read a single sector
static BYTE ReadSector (int hDevice_, PTRACK pTrack_, UINT uSector_)
{
    PTRACK pt = pTrack_;
    PSECTOR ps = reinterpret_cast<PSECTOR>(pt+1);
    ps += uSector_;

    struct floppy_raw_cmd rc;
    memset(&rc, 0, sizeof(rc));

    rc.flags = FD_RAW_READ | FD_RAW_INTR;
    rc.data  = ps->pbData;
    rc.length = 128U << (ps->size & 7);
    rc.rate = 2;
    rc.track = pt->cyl;

    BYTE cmd[] = { FD_READ, pt->head << 2, ps->cyl,ps->head,ps->sector,ps->size, ps->sector+1, 0x0a,0xff };
    memcpy(rc.cmd, cmd, sizeof(cmd));
    rc.cmd_count = sizeof(cmd);

    if (!ioctl(hDevice_, FDRAWCMD, &rc))
    {
        if (!(rc.reply[0] & 0x40))
            return (rc.reply[2] & 0x40) ? DELETED_DATA : 0;
        else if (rc.reply[1] & 0x20)
            return CRC_ERROR;
    }

    return RECORD_NOT_FOUND;
}

// Write a single sector
static BYTE WriteSector (int hDevice_, PTRACK pTrack_, UINT uSector_, BYTE *pbData_)
{
    PTRACK pt = pTrack_;
    PSECTOR ps = reinterpret_cast<PSECTOR>(pt+1);
    ps += uSector_;
    if (!pbData_) pbData_ = ps->pbData;

    struct floppy_raw_cmd rc;
    memset(&rc, 0, sizeof(rc));

    rc.flags = FD_RAW_WRITE | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    rc.data  = pbData_;
    rc.length = 128U << (ps->size & 7);
    rc.rate = 2;
    rc.track = pt->cyl;

    BYTE abCommand[] = { FD_WRITE, pt->head << 2, ps->cyl,ps->head,ps->sector,ps->size, ps->sector+1, 0x0a,0xff };
    memcpy(rc.cmd, abCommand, sizeof(abCommand));
    rc.cmd_count = sizeof(abCommand) / sizeof(abCommand[0]);

    if (!ioctl(hDevice_, FDRAWCMD, &rc))
    {
        // Successful?
        if (!(rc.reply[0] & 0x40))
            return 0;

        // Write protect error?
        if (rc.reply[1] & 0x02)
            return WRITE_PROTECT;

        // Sector not found?
        if (rc.reply[1] & 0x04)
            return RECORD_NOT_FOUND;
    }

    return WRITE_FAULT;
}

// Format a track
static BYTE FormatTrack (int hDevice_, PTRACK pTrack_)
{
    int i, step;
    BYTE bStatus = 0, ab[64*4] = {0}, *pb = ab;

    PTRACK pt = pTrack_;
    PSECTOR ps = reinterpret_cast<PSECTOR>(pt+1);
    UINT uSize = pt->sectors ? (128U << (ps->size & 7)) : 0;

    struct floppy_raw_cmd rc;
    memset(&rc, 0, sizeof(rc));

    BYTE abCommand[] = { FD_FORMAT, pt->head << 2, 6, pt->sectors, 1, 0x00 };
    memcpy(rc.cmd, abCommand, sizeof(abCommand));
    rc.cmd_count = sizeof(abCommand) / sizeof(abCommand[0]);

    // If the track contains any sectors, the size/gap/fill are tuned for the size/content
    if (pt->sectors)
    {
        UINT uGap = ((MAX_TRACK_SIZE-50) - (pt->sectors*(62+1+uSize))) / pt->sectors;
        if (uGap > 46) uGap = 46;

        rc.cmd[2] = ps->size;
        rc.cmd[4] = uGap;
        rc.cmd[5] = ps[pt->sectors-1].pbData[uSize-1];  // last byte of last sector used for fill byte
    }

    // Prepare the sector headers to write
    for (i = 0 ; i < pt->sectors ; i++)
    {
        *pb++ = ps[i].cyl;
        *pb++ = ps[i].head;
        *pb++ = ps[i].sector;
        *pb++ = ps[i].size;
    }

    // For blank tracks we still write a single long sector
    if (!rc.cmd[3])
        rc.cmd[3]++;

    rc.flags = FD_RAW_WRITE | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    rc.data  = ab;
    rc.length = rc.cmd[3]*4;
    rc.rate = 2;
    rc.track = pt->cyl;

    if (ioctl(hDevice_, FDRAWCMD, &rc))
        return WRITE_FAULT;

    if (rc.reply[0] & 0x40)
    {
        // Write protect error?
        if (rc.reply[1] & 0x02)
            return WRITE_PROTECT;

        // All other errors are fatal
        return WRITE_FAULT;
    }

    // Write any in-place format data, as needed by Pro-Dos (and future mixed-sector sizes)
    // 2 interleaved passes over the track is better than risking missing the next sector each time
    for (i = 0, step = 2 ; i < step ; i++)
    {
        for (int j = i ; !bStatus && j < pt->sectors ; j += step)
        {
            // Skip the write if the contents match the format filler
            if (ps[j].pbData[0] == rc.cmd[5] && !memcmp(ps[j].pbData, ps[j].pbData+1, uSize-1))
                continue;

            // Write the sector
            bStatus = WriteSector(hDevice_, pt, j, NULL);
        }
    }

    return bStatus;
}

// Read a simple 10-sector MGT or 9-sector DOS track, allowing no errors
static bool ReadSimpleTrack (int hDevice_, PTRACK pTrack_, UINT &ruSectors_)
{
    int i;

    PTRACK pt = pTrack_;
    PSECTOR ps = (PSECTOR)(pt+1);
    BYTE *pb = (BYTE*)(ps+(pt->sectors=NORMAL_DISK_SECTORS));

    // Prepare the track container
    for (i = 0 ; i < pt->sectors ; i++)
    {
        ps[i].cyl = pt->cyl;
        ps[i].head = pt->head;
        ps[i].sector = i+1;
        ps[i].size = 2;
        ps[i].status = 0;
        ps[i].pbData = pb + i*NORMAL_SECTOR_SIZE;
    }

    // Clear out the command structure
    struct floppy_raw_cmd rc;
    memset(&rc, 0, sizeof(rc));

    // Set up details for the command
    rc.flags = FD_RAW_READ  | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    rc.data  = pb;
    rc.length = pt->sectors*NORMAL_SECTOR_SIZE;
    rc.rate = 2;
    rc.track = pt->cyl;

    // Set up the command and its parameters
    BYTE abCommand[] = { FD_READ, pt->head << 2, pt->cyl, pt->head, ps->sector, ps->size, ps->sector+pt->sectors, 0x0a,0xff };
    memcpy(rc.cmd, abCommand, sizeof(abCommand));
    rc.cmd_count = sizeof(abCommand) / sizeof(abCommand[0]);

    if (ioctl(hDevice_, FDRAWCMD, &rc))
        return false;

    // Successful?
    if (!(rc.reply[0] & 0x40))
        return true;

    // ID mark not found?
    if (rc.reply[1] & 0x01)
    {
        // Accept blank tracks as normal
        pt->sectors = 0;
        return true;
    }

    // Failed to read 10th sector?
    if (rc.reply[1] & 0x04 && rc.reply[5] == NORMAL_DISK_SECTORS)
    {
        // Assume 9 sectors for the rest of this session
        pt->sectors = ruSectors_ = DOS_DISK_SECTORS;
        return true;
    }

    // For any other failures, fall back on non-regular mode for a more thorough scan
    pt->sectors = ruSectors_ = 0;
    return false;
}

static bool ReadCustomTrack (int hDevice_, PTRACK pTrack_)
{
    int i;

    PTRACK pt = pTrack_;
    PSECTOR ps = (PSECTOR)(pt+1);

    BYTE ab[128] = {0};
    BYTE sector=0xef, size=0;

    struct floppy_raw_cmd rc[64];
    memset(&rc, 0, sizeof(rc));

    rc[0].flags = FD_RAW_READ | FD_RAW_INTR | FD_RAW_NEED_SEEK;
    rc[0].data  = ab;
    rc[0].length = 128U << size;
    rc[0].rate = 2;
    rc[0].track = pt->cyl;

    BYTE cmd0[] = { FD_READ, pt->head << 2, 0xef,0xef,sector,size, sector+1, 0x0a,0xff };
    memcpy(rc[0].cmd, cmd0, sizeof(cmd0));
    rc[0].cmd_count = sizeof(cmd0);

    for (i = 1 ; i < (int)(sizeof(rc)/sizeof(rc[0])) ; i++)
    {
        rc[i].flags = FD_RAW_INTR;
        rc[i].rate = rc[0].rate;

        rc[i].cmd[0] = FD_READID;
        rc[i].cmd[1] = pt->head << 2;
        rc[i].cmd_count = 2;
    }

    if (ioctl(hDevice_, FDRAWCMD, &rc[0]))
        return false;

    // ID address mark missing?
    if (rc[0].reply[1] & 0x01)
    {
        // Track is blank
        pt->sectors = 0;
        return true;
    }

    for (i = 1 ; i < (int)(sizeof(rc)/sizeof(rc[0])) ; i++)
    {
        ioctl(hDevice_, FDRAWCMD, &rc[i]);

        // Track loop?
        if (i > 1 && rc[i].reply[5] == rc[1].reply[5])
        {
            pt->sectors = i-1;
            break;
        }
    }

    // Set up the data pointer after the sector headers
    BYTE *pb = (BYTE*)(ps+pt->sectors);

    // Copy from the scanned headers to the track sector headers
    // Use two passes in case we're not quick enough to do it in one
    for (int i = 0, step = 2 ; i < step ; i++)
    {
        for (int j = i ; j < pt->sectors ; j += step)
        {
            ps[j].cyl = rc[j+1].reply[3];
            ps[j].head = rc[j+1].reply[4];
            ps[j].sector = rc[j+1].reply[5];
            ps[j].size = rc[j+1].reply[6];

            ps[j].pbData = pb;
            pb += (128U << (ps[j].size & 7));

            // Read the sector data
            ps[j].status = ReadSector(hDevice_, pt, j);
        }
    }

    return true;
}


void *CFloppyStream::ThreadProc ()
{
    // Open the device, if not already open
    if (!IsOpen())
        Open();

    switch (m_bCommand)
    {
        // Load track contents?
        case READ_MSECTOR:
            // If we've got a sector count, read the track assuming that value
            if (m_uSectors)
                ReadSimpleTrack(m_nFloppy, m_pTrack, m_uSectors);

            // If we've not sector count, scan and read the track (slower)
            if (!m_uSectors)
                ReadCustomTrack(m_nFloppy, m_pTrack);

            break;

        // Write a sector?
        case WRITE_1SECTOR:
        {
            // Locate the sector to write
            PSECTOR ps = reinterpret_cast<PSECTOR>(m_pTrack+1);
            ps += m_uSector;

            m_bStatus = WriteSector(m_nFloppy, m_pTrack, m_uSector, m_pbData);
            break;
        }

        // Format track?
        case WRITE_TRACK:
            m_bStatus = FormatTrack(m_nFloppy, m_pTrack);
            break;

        default:
            TRACE("!!! Unknown fdc stream command: %u\n", m_bCommand);
            m_bStatus = LOST_DATA;
            break;
    }

    m_fThreadDone = true;
    return 0;
}

#else
// Dummy implementation for non-Linux SDL versions

/*static*/ bool CFloppyStream::IsRecognised (const char* pcszStream_)
{
    return false;
}

void CFloppyStream::Close ()
{
}

BYTE CFloppyStream::StartCommand (BYTE bCommand_, PTRACK pTrack_, UINT uSector_, BYTE *pbData_)
{
    return BUSY;
}

bool CFloppyStream::IsBusy (BYTE* pbStatus_, bool fWait_)
{
    *pbStatus_ = LOST_DATA;
    return false;
}

#endif
