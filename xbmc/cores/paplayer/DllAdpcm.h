#pragma once

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

#include "DynamicDll.h"

class DllADPCMInterface
{
public:
    virtual ~DllADPCMInterface() {}
    virtual int LoadXWAV(const char* szFileName)=0;
    virtual void FreeXWAV(int)=0;
    virtual long FillBuffer(int nsf, char* buffer, int size)=0;
    virtual int GetPlaybackRate(int nsf)=0;
    virtual int GetNumberOfChannels(int info)=0;
    virtual int GetSampleSize(int info)=0;
    virtual int GetLength(int info)=0;
    virtual int Seek(int info, int pos)=0;
};

class DllADPCM : public DllDynamic, DllADPCMInterface
{
  DECLARE_DLL_WRAPPER(DllADPCM, q:\\system\\players\\paplayer\\adpcm.dll)
  DEFINE_METHOD1(int, LoadXWAV, (const char* p1))
  DEFINE_METHOD1(void, FreeXWAV, (int p1))
  DEFINE_METHOD3(long, FillBuffer, (int p1, char* p2, int p3))
  DEFINE_METHOD1(int, GetPlaybackRate, (int p1))
  DEFINE_METHOD1(int, GetNumberOfChannels, (int p1))
  DEFINE_METHOD1(int, GetSampleSize, (int p1))
  DEFINE_METHOD1(int, GetLength, (int p1))
  DEFINE_METHOD2(int, Seek, (int p1, int p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadXWAV, LoadXWAV)
    RESOLVE_METHOD_RENAME(DLL_FreeXWAV, FreeXWAV)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
    RESOLVE_METHOD_RENAME(DLL_GetPlaybackRate, GetPlaybackRate)
    RESOLVE_METHOD_RENAME(DLL_Seek, Seek)
    RESOLVE_METHOD_RENAME(DLL_GetNumberOfChannels, GetNumberOfChannels)
    RESOLVE_METHOD_RENAME(DLL_GetSampleSize, GetSampleSize)
    RESOLVE_METHOD_RENAME(DLL_GetLength, GetLength)
  END_METHOD_RESOLVE()
};
