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

#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMusicVideos::CDirectoryNodeRecentlyAddedMusicVideos(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeRecentlyAddedMusicVideos::GetChildType() const
{
  return NODE_TYPE_TITLE_MUSICVIDEOS;
}

bool CDirectoryNodeRecentlyAddedMusicVideos::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetRecentlyAddedMusicVideosNav(strBaseDir, items);

  videodatabase.Close();

  return bSuccess;
}

