/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CACHECIRCULAR_H
#define CACHECIRCULAR_H

#include "CacheStrategy.h"

namespace XFILE {

class CCircularCache : public CCacheStrategy
{
public:
    CCircularCache(size_t front, size_t back);
    virtual ~CCircularCache();

    virtual int Open() ;
    virtual void Close();

    virtual int WriteToCache(const char *buf, size_t len) ;
    virtual int ReadFromCache(char *buf, size_t len) ;
    virtual int64_t WaitForData(unsigned int minimum, unsigned int iMillis) ;

    virtual int64_t Seek(int64_t pos) ;
    virtual void Reset(int64_t pos) ;

protected:
    uint64_t          m_beg;       /**< index in file (not buffer) of beginning of valid data */
    uint64_t          m_end;       /**< index in file (not buffer) of end of valid data */
    uint64_t          m_cur;       /**< current reading index in file */
    uint8_t          *m_buf;       /**< buffer holding data */
    size_t            m_size;      /**< size of data buffer used (m_buf) */
    size_t            m_size_back; /**< guaranteed size of back buffer (actual size can be smaller, or larger if front buffer doesn't need it) */
    CCriticalSection  m_sync;
    CEvent            m_written;
};

} // namespace XFILE
#endif
