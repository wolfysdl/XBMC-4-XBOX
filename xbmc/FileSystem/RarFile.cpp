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

#include "utils/log.h"
#include "RarFile.h"
#include <sys/stat.h>
#include "Util.h"
#include "utils/URIUtils.h"
#include "utils/CharsetConverter.h"
#include "URL.h"
#include "dialogs/GUIDialogOK.h"
#include "FileSystem/Directory.h"
#include "RarManager.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"

#include <process.h>

using namespace XFILE;
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************

#define SEEKTIMOUT 30000

CRarFileExtractThread::CRarFileExtractThread()
{
  m_pArc = NULL;
  m_pCmd = NULL;
  m_pExtract = NULL;
  hRunning = CreateEvent(NULL,true,false,NULL);
  hRestart = CreateEvent(NULL,false,false,NULL);
  hQuit = CreateEvent(NULL,true,false,NULL);
  StopThread();
  Create();
}

CRarFileExtractThread::~CRarFileExtractThread()
{
  SetEvent(hQuit);
  WaitForSingleObject(hRestart,INFINITE);
  StopThread();
  
  CloseHandle(hRunning);
  CloseHandle(hQuit);
  CloseHandle(hRestart);
}

void CRarFileExtractThread::Start(Archive* pArc, CommandData* pCmd, CmdExtract* pExtract, int iSize)
{
  m_pArc = pArc;
  m_pCmd = pCmd;
  m_pExtract = pExtract;
  m_iSize = iSize;
  
  m_pExtract->GetDataIO().hBufferFilled = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hBufferEmpty = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hSeek = CreateEvent(NULL,true,false,NULL);
  m_pExtract->GetDataIO().hSeekDone = CreateEvent(NULL,false,false,NULL);
  m_pExtract->GetDataIO().hQuit = CreateEvent(NULL,true,false,NULL);

  SetEvent(hRunning);
  SetEvent(hRestart);
}

void CRarFileExtractThread::OnStartup()
{
}

void CRarFileExtractThread::OnExit()
{
}

void CRarFileExtractThread::Process()
{
  while (WaitForSingleObject(hQuit,1) != WAIT_OBJECT_0)
  {
    if (WaitForSingleObject(hRestart,1) == WAIT_OBJECT_0)
    {
      bool Repeat = false;
      m_pExtract->ExtractCurrentFile(m_pCmd,*m_pArc,m_iSize,Repeat);
      ResetEvent(hRunning);
    }
  }
  SetEvent(hRestart);
}

CRarFile::CRarFile()
{
  m_strCacheDir.Empty();
  m_strRarPath.Empty();
  m_strPassword.Empty();
  m_strPathInRar.Empty();
  m_bFileOptions = 0;
  m_pArc = NULL;
  m_pCmd = NULL;
  m_pExtract = NULL;
  m_pExtractThread = NULL;
  m_szBuffer = NULL;
  m_szStartOfBuffer = NULL;
  m_iDataInBuffer = 0;
  m_bUseFile = false;
  m_bOpen = false;
  m_bSeekable = true;
}

//*********************************************************************************************
CRarFile::~CRarFile()
{
  if (!m_bOpen)
    return;
  
  if (m_bUseFile)
  {
    m_File.Close();
    g_RarManager.ClearCachedFile(m_strRarPath,m_strPathInRar); 
  }
  else
  {
    CleanUp();
    if (m_pExtractThread)
    {
      delete m_pExtractThread;
      m_pExtractThread = NULL;
    }
  }
}
//*********************************************************************************************
bool CRarFile::Open(const CURL& url)
{
  InitFromUrl(url);
  CFileItemList items;
  g_RarManager.GetFilesInRar(items,m_strRarPath,false);
  int i;
  for (i=0;i<items.Size();++i)
  {
    if (items[i]->GetLabel() == m_strPathInRar)
      break;
  }

  if (i<items.Size())
  {
    if (items[i]->m_idepth == 0x30) // stored
    {
      if (!OpenInArchive())
        return false;

      m_iFileSize = items[i]->m_dwSize;
      m_bOpen = true;
      
      // perform 'noidx' check
      CFileInfo* pFile = g_RarManager.GetFileInRar(m_strRarPath,m_strPathInRar);
      if (pFile)
      {
        if (pFile->m_iIsSeekable == -1)
        {
          if (Seek(-1,SEEK_END) == -1)
          {
            m_bSeekable = false;
            pFile->m_iIsSeekable = 0;
          }
        }
        else
          m_bSeekable = (pFile->m_iIsSeekable == 1);
      }
      return true;
    }
    else 
    {
      if (items[i]->m_dwSize > ((int64_t)4)*1024*1024*1024) // 4 gig limit of fat-x
      {
        CGUIDialogOK::ShowAndGetInput(257,21395,-1,-1);
        CLog::Log(LOGERROR,"CRarFile::Open: Can't cache files bigger than 4GB due to fat-x limits.");
        return false;
      }
      m_bUseFile = true;
      CStdString strPathInCache;
      
      if (!g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar, 
                                        EXFILE_AUTODELETE | m_bFileOptions, m_strCacheDir, 
                                        items[i]->m_dwSize))
      {
        CLog::Log(LOGERROR,"filerar::open failed to cache file %s",m_strPathInRar.c_str());
        return false;
      }

      if (!m_File.Open( strPathInCache ))
      {
        CLog::Log(LOGERROR,"filerar::open failed to open file in cache: %s",strPathInCache.c_str());
        return false;
      }
      
      m_bOpen = true;
      return true;
    }
  }
  return false;
}

bool CRarFile::Exists(const CURL& url)
{
  InitFromUrl(url);
  CStdString strPathInCache;
  bool bResult;
  
  if (!g_RarManager.IsFileInRar(bResult, m_strRarPath, m_strPathInRar)) 
    return false;
  
  return bResult;
}
//*********************************************************************************************
int CRarFile::Stat(const CURL& url, struct __stat64* buffer)
{
  memset(buffer, 0, sizeof(struct __stat64));
  if (Open(url))
  {
    buffer->st_size = GetLength();
    buffer->st_mode = _S_IFREG;
    Close();
    errno = 0;
    return 0;
  }

  if (CDirectory::Exists(url.Get()))
  {
    buffer->st_mode = _S_IFDIR;
    return 0;
  }

  errno = ENOENT;
  return -1;
}


//*********************************************************************************************
bool CRarFile::OpenForWrite(const CURL& url)
{
  return false;
}

//*********************************************************************************************
unsigned int CRarFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_bOpen)
    return 0;

  if (m_bUseFile)
    return m_File.Read(lpBuf,uiBufSize);
  
  if (m_iFilePosition >= GetLength()) // we are done
    return 0;
  
  if( WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,5000) == WAIT_TIMEOUT )
  {
    CLog::Log(LOGERROR, "%s - Timeout waiting for buffer to empty", __FUNCTION__);
    return 0;
  }


  byte* pBuf = (byte*)lpBuf;
  int64_t uicBufSize = uiBufSize;
  if (m_iDataInBuffer > 0)
  {
    int64_t iCopy = uiBufSize<m_iDataInBuffer?uiBufSize:m_iDataInBuffer;
    memcpy(lpBuf,m_szStartOfBuffer,size_t(iCopy));
    m_szStartOfBuffer += iCopy;
    m_iDataInBuffer -= int(iCopy);
    pBuf += iCopy;
    uicBufSize -= iCopy;
    m_iFilePosition += iCopy;
  }

  while ((uicBufSize > 0) && m_iFilePosition < GetLength() )
  {
    if (m_iDataInBuffer <= 0)
    {
      m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,MAXWINMEMSIZE);
      m_szStartOfBuffer = m_szBuffer;
      m_iBufferStart = m_iFilePosition;
    }
    
    SetEvent(m_pExtract->GetDataIO().hBufferFilled);
    WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,INFINITE);

    if (m_pExtract->GetDataIO().NextVolumeMissing)
      break;
   
    m_iDataInBuffer = MAXWINMEMSIZE-m_pExtract->GetDataIO().UnpackToMemorySize;

    if (m_iDataInBuffer < 0 ||
        m_iDataInBuffer > MAXWINMEMSIZE - (m_szStartOfBuffer - m_szBuffer))
    {
      // invalid data returned by UnrarXLib, prevent a crash
      CLog::Log(LOGERROR, "CRarFile::Read - Data buffer in inconsistent state");
      m_iDataInBuffer = 0;
    }

    if (m_iDataInBuffer == 0)
      break;
    
    if (m_iDataInBuffer > uicBufSize)
    {
      memcpy(pBuf,m_szStartOfBuffer,int(uicBufSize));
      m_szStartOfBuffer += uicBufSize;
      pBuf += int(uicBufSize);
      m_iFilePosition += uicBufSize;
      m_iDataInBuffer -= int(uicBufSize);
      uicBufSize = 0;
    }
    else
    {
      memcpy(pBuf,m_szStartOfBuffer,size_t(m_iDataInBuffer));
      m_iFilePosition += m_iDataInBuffer;
      m_szStartOfBuffer += m_iDataInBuffer;
      uicBufSize -= m_iDataInBuffer;
      pBuf += m_iDataInBuffer;
      m_iDataInBuffer = 0;
    }
  }
  
  SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
  
  return static_cast<unsigned int>(uiBufSize-uicBufSize);
}

//*********************************************************************************************
unsigned int CRarFile::Write(void *lpBuf, int64_t uiBufSize)
{
  return 0;
}

//*********************************************************************************************
void CRarFile::Close()
{
  if (!m_bOpen)
    return;

  if (m_bUseFile)
  {
    m_File.Close();
    g_RarManager.ClearCachedFile(m_strRarPath,m_strPathInRar);
    m_bOpen = false;
  }
  else
  {
    CleanUp();
    if (m_pExtractThread)
    {
      delete m_pExtractThread;
      m_pExtractThread = NULL;
    }
    m_bOpen = false;
  }
}

//*********************************************************************************************
int64_t CRarFile::Seek(int64_t iFilePosition, int iWhence)
{ 
  if (!m_bOpen)
    return -1;

  if (!m_bSeekable)
    return -1;

  if (m_bUseFile)
    return m_File.Seek(iFilePosition,iWhence);
  
  if( WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,SEEKTIMOUT) == WAIT_TIMEOUT )
  {
    CLog::Log(LOGERROR, "%s - Timeout waiting for buffer to empty", __FUNCTION__);
    return -1;
  }

  SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
 
  switch (iWhence)
  {
    case SEEK_CUR:
      if (iFilePosition == 0)
        return m_iFilePosition; // happens sometimes

      iFilePosition += m_iFilePosition;
      break;
    case SEEK_END:
      if (iFilePosition == 0) // do not seek to end
      { 
        m_iFilePosition = this->GetLength();
        m_iDataInBuffer = 0;
        m_iBufferStart = this->GetLength();
        
        return this->GetLength();
      }

      iFilePosition += GetLength();
    case SEEK_SET:
      break;
    default:
      return -1;
  }
  
  if (iFilePosition > this->GetLength()) 
    return -1;
  
  if (iFilePosition == m_iFilePosition) // happens a lot
    return m_iFilePosition; 
  
  if ((iFilePosition >= m_iBufferStart) && (iFilePosition < m_iBufferStart+MAXWINMEMSIZE) 
                                        && (m_iDataInBuffer > 0)) // we are within current buffer
  {
    m_iDataInBuffer = MAXWINMEMSIZE-(iFilePosition-m_iBufferStart);
    m_szStartOfBuffer = m_szBuffer+MAXWINMEMSIZE-m_iDataInBuffer;
    m_iFilePosition = iFilePosition;
    
    return m_iFilePosition;
  }
  
  if (iFilePosition < m_iBufferStart )
  {
    CleanUp();
    if (!OpenInArchive())
      return -1;
    
    if( WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,SEEKTIMOUT) == WAIT_TIMEOUT )
    {
      CLog::Log(LOGERROR, "%s - Timeout waiting for buffer to empty", __FUNCTION__);
      return -1;
    }
    SetEvent(m_pExtract->GetDataIO().hBufferEmpty);
    m_pExtract->GetDataIO().m_iSeekTo = iFilePosition;
  }
  else
    m_pExtract->GetDataIO().m_iSeekTo = iFilePosition;
  
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,MAXWINMEMSIZE);
  SetEvent(m_pExtract->GetDataIO().hSeek);
  SetEvent(m_pExtract->GetDataIO().hBufferFilled);
  if( WaitForSingleObject(m_pExtract->GetDataIO().hSeekDone,SEEKTIMOUT) == WAIT_TIMEOUT )
  {
    CLog::Log(LOGERROR, "%s - Timeout waiting for seek to finish", __FUNCTION__);
    return -1;
  }

  if (m_pExtract->GetDataIO().NextVolumeMissing)
  {
    m_iFilePosition = m_iFileSize;
    return -1;
  }

  if( WaitForSingleObject(m_pExtract->GetDataIO().hBufferEmpty,SEEKTIMOUT) == WAIT_TIMEOUT )
  {
    CLog::Log(LOGERROR, "%s - Timeout waiting for buffer to empty", __FUNCTION__);
    return -1;
  }
  m_iDataInBuffer = m_pExtract->GetDataIO().m_iSeekTo; // keep data
  m_iBufferStart = m_pExtract->GetDataIO().m_iStartOfBuffer;

  if (m_iDataInBuffer < 0 || m_iDataInBuffer > MAXWINMEMSIZE)
  {
    // invalid data returned by UnrarXLib, prevent a crash
    CLog::Log(LOGERROR, "CRarFile::Seek - Data buffer in inconsistent state");
    m_iDataInBuffer = 0;
    return -1;
  }

  m_szStartOfBuffer = m_szBuffer+MAXWINMEMSIZE-m_iDataInBuffer;
  m_iFilePosition = iFilePosition;
  
  return m_iFilePosition;
}

//*********************************************************************************************
int64_t CRarFile::GetLength()
{
  if (!m_bOpen)
    return 0;

  if (m_bUseFile)
    return m_File.GetLength();

  return m_iFileSize;
}

//*********************************************************************************************
int64_t CRarFile::GetPosition()
{
  if (!m_bOpen)
    return -1;

  if (m_bUseFile)
    return m_File.GetPosition();

  return m_iFilePosition;
}

int CRarFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  return -1;
}

void CRarFile::Flush()
{
  if (m_bUseFile)
    m_File.Flush();
}
void CRarFile::InitFromUrl(const CURL& url)
{
  m_strCacheDir = g_advancedSettings.m_cachePath;//url.GetDomain();
  URIUtils::AddSlashAtEnd(m_strCacheDir);
  m_strRarPath = url.GetHostName();
  m_strPassword = url.GetUserName();
  m_strPathInRar = url.GetFileName();  

  vector<CStdString> options;
  CUtil::Tokenize(url.GetOptions().Mid(1), options, "&");
  
  m_bFileOptions = 0;

  for( vector<CStdString>::iterator it = options.begin();it != options.end(); it++)
  {
    int iEqual = (*it).Find('=');
    if( iEqual >= 0 )
    {
      CStdString strOption = (*it).Left(iEqual);
      CStdString strValue = (*it).Mid(iEqual+1);

      if( strOption.Equals("flags") )
        m_bFileOptions = atoi(strValue.c_str());
      else if( strOption.Equals("cache") )
        m_strCacheDir = strValue;
    }
  }

}

void CRarFile::CleanUp()
{
  if (m_pExtractThread)
  {
    if (WaitForSingleObject(m_pExtractThread->hRunning,1) == WAIT_OBJECT_0)
    {
      SetEvent(m_pExtract->GetDataIO().hQuit);
      while (WaitForSingleObject(m_pExtractThread->hRunning,1) == WAIT_OBJECT_0) 
        Sleep(1);
    
    }
    CloseHandle(m_pExtract->GetDataIO().hBufferFilled);
    CloseHandle(m_pExtract->GetDataIO().hBufferEmpty);
    CloseHandle(m_pExtract->GetDataIO().hSeek);
    CloseHandle(m_pExtract->GetDataIO().hSeekDone);
    CloseHandle(m_pExtract->GetDataIO().hQuit);
  }
  if (m_pExtract)
  {
    delete m_pExtract;
    m_pExtract = NULL;
  }
  if (m_pArc)
  {
    delete m_pArc;
    m_pArc = NULL;
  }
  if (m_pCmd)
  {
    delete m_pCmd;
    m_pCmd = NULL;
  }
  if (m_szBuffer)
  {
    delete[] m_szBuffer;
    m_szBuffer = NULL;
    m_szStartOfBuffer = NULL;
  }
}

bool CRarFile::OpenInArchive()
{
  int iHeaderSize;

  InitCRC();

  m_pCmd = new CommandData;
  if (!m_pCmd)
  {
    CleanUp();
    return false;
  }

  // Set the arguments for the extract command
  strcpy(m_pCmd->Command, "X");

  m_pCmd->AddArcName(const_cast<char*>(m_strRarPath.c_str()),NULL);

  strncpy(m_pCmd->ExtrPath, m_strCacheDir.c_str(), sizeof (m_pCmd->ExtrPath) - 2);
  m_pCmd->ExtrPath[sizeof (m_pCmd->ExtrPath) - 2] = 0;
  AddEndSlash(m_pCmd->ExtrPath);

  // Set password for encrypted archives
  if ((m_strPassword.size() > 0) &&
      (m_strPassword.size() < sizeof (m_pCmd->Password)))
  {
    strcpy(m_pCmd->Password, m_strPassword.c_str());
  }

  m_pCmd->ParseDone();

  // Open the archive
  m_pArc = new Archive(m_pCmd);
  if (!m_pArc)
  {
    CleanUp();
    return false;
  }
  if (!m_pArc->WOpen(m_strRarPath.c_str(),NULL))
  {
    CleanUp();
    return false;
  }
  if (!(m_pArc->IsOpened() && m_pArc->IsArchive(true)))
  {
    CleanUp();
    return false;
  }

  m_pExtract = new CmdExtract;
  if (!m_pExtract)
  {
    CleanUp();
    return false;
  }
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,0);
  m_pExtract->GetDataIO().SetCurrentCommand(*(m_pCmd->Command));
  struct FindData FD;
  if (FindFile::FastFind(m_strRarPath.c_str(),NULL,&FD))
    m_pExtract->GetDataIO().TotalArcSize+=FD.Size;
  m_pExtract->ExtractArchiveInit(m_pCmd,*m_pArc);

  while (true)
  {
    if ((iHeaderSize = m_pArc->ReadHeader()) <= 0)
    {
      CleanUp();
      return false;
    }

    if (m_pArc->GetHeaderType() == FILE_HEAD)
    {
      CStdString strFileName;

      if (m_pArc->NewLhd.FileNameW && wcslen(m_pArc->NewLhd.FileNameW) > 0)
      {
        g_charsetConverter.wToUTF8(m_pArc->NewLhd.FileNameW, strFileName);
      }
      else
      {
        g_charsetConverter.unknownToUTF8(m_pArc->NewLhd.FileName, strFileName);
      }

      /* replace back slashes into forward slashes */
      /* this could get us into troubles, file could two different files, one with / and one with \ */
      strFileName.Replace('\\', '/');

      if (strFileName == m_strPathInRar)
      {
        break;
      }
    }
    
    m_pArc->SeekToNext();
  }
  
  m_szBuffer = new byte[MAXWINMEMSIZE];
  m_szStartOfBuffer = m_szBuffer;
  m_pExtract->GetDataIO().SetUnpackToMemory(m_szBuffer,0);
  m_iDataInBuffer = -1;
  m_iFilePosition = 0;
  m_iBufferStart = 0;
  
  delete m_pExtractThread;
  m_pExtractThread = new CRarFileExtractThread();
  m_pExtractThread->Start(m_pArc,m_pCmd,m_pExtract,iHeaderSize);
  
  return true;
}


