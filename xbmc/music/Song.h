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
/*!
 \file Song.h
\brief
*/
#pragma once

#include "utils/StdString.h"

#include <map>
#include <vector>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

/*!
 \ingroup music
 \brief Class to store and read album information from CMusicDatabase
 \sa CSong, CMusicDatabase
 */

class CGenre
{
public:
  long idGenre;
  CStdString strGenre;
};

/*!
 \ingroup music
 \brief Class to store and read song information from CMusicDatabase
 \sa CAlbum, CMusicDatabase
 */
class CSong
{
public:
  CSong() ;
  CSong(MUSIC_INFO::CMusicInfoTag& tag);
  virtual ~CSong(){};
  void Clear() ;

  bool operator<(const CSong &song) const
  {
    if (strFileName < song.strFileName) return true;
    if (strFileName > song.strFileName) return false;
    if (iTrack < song.iTrack) return true;
    return false;
  }
  long idSong;
  CStdString strFileName;
  CStdString strTitle;
  CStdString strArtist;
  CStdString strAlbum;
  CStdString strAlbumArtist;
  CStdString strGenre;
  CStdString strThumb;
  CStdString strMusicBrainzTrackID;
  CStdString strMusicBrainzArtistID;
  CStdString strMusicBrainzAlbumID;
  CStdString strMusicBrainzAlbumArtistID;
  CStdString strMusicBrainzTRMID;
  CStdString strComment;
  char rating;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimesPlayed;
  CStdString lastPlayed;
  int iStartOffset;
  int iEndOffset;
};

/*!
 \ingroup music
 \brief A map of CSong objects, used for CMusicDatabase
 */
class CSongMap
{
public:
  CSongMap();

  CSong *Find(const CStdString &file);
  void Add(const CStdString &file, const CSong &song);
  void Clear();
  int Size();

private:
  std::map<CStdString, CSong> m_map;
};

/*!
 \ingroup music
 \brief A vector of CSong objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CSong> VECSONGS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CGenre> VECGENRES;
