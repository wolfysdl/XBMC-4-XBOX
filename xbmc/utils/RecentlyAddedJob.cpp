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
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "FileItem.h"
#include "RecentlyAddedJob.h"
#include "GUIWindow.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "utils/Variant.h"

#define NUM_ITEMS 10

CRecentlyAddedJob::CRecentlyAddedJob(int flag)
{
  m_flag = flag;
} 

bool CRecentlyAddedJob::UpdateVideo()
{
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);

  if ( home == NULL )
    return false;

  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateVideos() - Running RecentlyAdded home screen update");
  
  int            i = 0;
  CFileItemList  items;
  CVideoDatabase videodatabase;
  
  videodatabase.Open();

  if (videodatabase.GetRecentlyAddedMoviesNav("videodb://1/", items, NUM_ITEMS))
  {  
    for (; i < items.Size(); ++i)
    {
      CFileItemPtr item = items.Get(i);
      CStdString   value;
      CStdString   strRating;
      value.Format("%i", i + 1);
      strRating.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
      
      home->SetProperty("LatestMovie." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMovie." + value + ".Rating"      , strRating);
      home->SetProperty("LatestMovie." + value + ".Year"        , item->GetVideoInfoTag()->m_iYear);
      home->SetProperty("LatestMovie." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMovie." + value + ".RunningTime" , item->GetVideoInfoTag()->m_strRuntime);
      home->SetProperty("LatestMovie." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMovie." + value + ".Trailer"     , item->GetVideoInfoTag()->m_strTrailer);

      if (!item->HasThumbnail())
        m_thumbLoader.LoadItem(item.get());

      home->SetProperty("LatestMovie." + value + ".Thumb"       , item->GetThumbnailImage());
      home->SetProperty("LatestMovie." + value + ".Fanart"      , item->GetCachedFanart());
    }
  } 
  for (; i < NUM_ITEMS; ++i)
  {
    CStdString value;
    value.Format("%i", i + 1);
    home->SetProperty("LatestMovie." + value + ".Title"       , "");
    home->SetProperty("LatestMovie." + value + ".Thumb"       , "");
    home->SetProperty("LatestMovie." + value + ".Rating"      , "");
    home->SetProperty("LatestMovie." + value + ".Year"        , "");
    home->SetProperty("LatestMovie." + value + ".Plot"        , "");
    home->SetProperty("LatestMovie." + value + ".RunningTime" , "");
    home->SetProperty("LatestMovie." + value + ".Path"        , "");
    home->SetProperty("LatestMovie." + value + ".Trailer"     , "");
    home->SetProperty("LatestMovie." + value + ".Fanart"      , "");
  }
 
  i = 0;
  CFileItemList  TVShowItems; 
 
  if (videodatabase.GetRecentlyAddedEpisodesNav("videodb://1/", TVShowItems, NUM_ITEMS))
  {  
    for (; i < TVShowItems.Size(); ++i)
    {    
      CFileItemPtr item          = TVShowItems.Get(i);
      int          EpisodeSeason = item->GetVideoInfoTag()->m_iSeason;
      int          EpisodeNumber = item->GetVideoInfoTag()->m_iEpisode;
      CStdString   EpisodeNo;
      CStdString   value;
      CStdString   strRating;
      EpisodeNo.Format("s%02de%02d", EpisodeSeason, EpisodeNumber);
      value.Format("%i", i + 1);
      strRating.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
      
      home->SetProperty("LatestEpisode." + value + ".ShowTitle"     , item->GetVideoInfoTag()->m_strShowTitle);
      home->SetProperty("LatestEpisode." + value + ".EpisodeTitle"  , item->GetVideoInfoTag()->m_strTitle);
      home->SetProperty("LatestEpisode." + value + ".Rating"        , strRating);      
      home->SetProperty("LatestEpisode." + value + ".Plot"          , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestEpisode." + value + ".EpisodeNo"     , EpisodeNo);
      home->SetProperty("LatestEpisode." + value + ".EpisodeSeason" , EpisodeSeason);
      home->SetProperty("LatestEpisode." + value + ".EpisodeNumber" , EpisodeNumber);
      home->SetProperty("LatestEpisode." + value + ".Path"          , item->GetVideoInfoTag()->m_strFileNameAndPath);

      if (!item->HasThumbnail())
        m_thumbLoader.LoadItem(item.get());

      home->SetProperty("LatestEpisode." + value + ".Thumb"         , item->GetThumbnailImage());
      home->SetProperty("LatestEpisode." + value + ".Fanart"        , item->GetCachedFanart());
    }
  } 
  for (; i < NUM_ITEMS; ++i)
  {
    CStdString value;
    value.Format("%i", i + 1);
    home->SetProperty("LatestEpisode." + value + ".ShowTitle"     , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeTitle"  , "");
    home->SetProperty("LatestEpisode." + value + ".Rating"        , "");      
    home->SetProperty("LatestEpisode." + value + ".Plot"          , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeNo"     , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeSeason" , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeNumber" , "");
    home->SetProperty("LatestEpisode." + value + ".Path"          , "");
    home->SetProperty("LatestEpisode." + value + ".Thumb"         , "");
    home->SetProperty("LatestEpisode." + value + ".Fanart"        , "");
  }  

  i = 0;
  CFileItemList MusicVideoItems;

  if (videodatabase.GetRecentlyAddedMusicVideosNav("videodb://1/", MusicVideoItems, NUM_ITEMS))
  {
    for (; i < MusicVideoItems.Size(); ++i)
    {
      CFileItemPtr item = MusicVideoItems.Get(i);
      CStdString   value;
      value.Format("%i", i + 1);

      home->SetProperty("LatestMusicVideo." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMusicVideo." + value + ".Year"        , item->GetVideoInfoTag()->m_iYear);
      home->SetProperty("LatestMusicVideo." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMusicVideo." + value + ".RunningTime" , item->GetVideoInfoTag()->m_strRuntime);
      home->SetProperty("LatestMusicVideo." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMusicVideo." + value + ".Artist"      , item->GetVideoInfoTag()->m_strArtist);

      if (!item->HasThumbnail())
        m_thumbLoader.LoadItem(item.get());

      home->SetProperty("LatestMusicVideo." + value + ".Thumb"       , item->GetThumbnailImage());
      home->SetProperty("LatestMusicVideo." + value + ".Fanart"      , item->GetCachedFanart());
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    CStdString value;
    value.Format("%i", i + 1);
    home->SetProperty("LatestMusicVideo." + value + ".Title"       , "");
    home->SetProperty("LatestMusicVideo." + value + ".Thumb"       , "");
    home->SetProperty("LatestMusicVideo." + value + ".Year"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".Plot"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".RunningTime" , "");
    home->SetProperty("LatestMusicVideo." + value + ".Path"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".Artist"      , "");
    home->SetProperty("LatestMusicVideo." + value + ".Fanart"      , "");
  }

  videodatabase.Close();
  return true;
}

bool CRecentlyAddedJob::UpdateMusic()
{
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);
  
  if ( home == NULL )
    return false;
  
  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateMusic() - Running RecentlyAdded home screen update");
  
  int            i = 0;
  CFileItemList  musicItems;
  CMusicDatabase musicdatabase;
  
  musicdatabase.Open();
  
  if (musicdatabase.GetRecentlyAddedAlbumSongs("musicdb://", musicItems, NUM_ITEMS))
  { 
    for (; i < musicItems.Size(); ++i)
    {  
      CFileItemPtr item = musicItems.Get(i);
      CStdString   value;
      value.Format("%i", i + 1);
      
      CStdString   strThumb;
      CStdString   strRating;
      CStdString   strAlbum  = item->GetMusicInfoTag()->GetAlbum();
      CStdString   strArtist = item->GetMusicInfoTag()->GetArtist();
      
      long idAlbum = musicdatabase.GetAlbumByName(strAlbum,strArtist);
      if (idAlbum != -1)
        musicdatabase.GetAlbumThumb(idAlbum,strThumb);
      
      strRating.Format("%c", item->GetMusicInfoTag()->GetRating());
      
      home->SetProperty("LatestSong." + value + ".Title"   , item->GetMusicInfoTag()->GetTitle());
      home->SetProperty("LatestSong." + value + ".Year"    , item->GetMusicInfoTag()->GetYear());
      home->SetProperty("LatestSong." + value + ".Artist"  , strArtist);      
      home->SetProperty("LatestSong." + value + ".Album"   , strAlbum);
      home->SetProperty("LatestSong." + value + ".Rating"  , strRating);
      home->SetProperty("LatestSong." + value + ".Path"    , item->GetMusicInfoTag()->GetURL());
      home->SetProperty("LatestSong." + value + ".Thumb"   , strThumb);
      home->SetProperty("LatestSong." + value + ".Fanart"  , item->GetCachedFanart());
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    CStdString value;
    value.Format("%i", i + 1);
    home->SetProperty("LatestSong." + value + ".Title"   , "");
    home->SetProperty("LatestSong." + value + ".Year"    , "");
    home->SetProperty("LatestSong." + value + ".Artist"  , "");      
    home->SetProperty("LatestSong." + value + ".Album"   , "");
    home->SetProperty("LatestSong." + value + ".Rating"  , "");
    home->SetProperty("LatestSong." + value + ".Path"    , "");
    home->SetProperty("LatestSong." + value + ".Thumb"   , "");
    home->SetProperty("LatestSong." + value + ".Fanart"  , "");
  }
  
  i = 0;
  VECALBUMS albums;
  
  if (musicdatabase.GetRecentlyAddedAlbums(albums, NUM_ITEMS))
  { 
    for (; i < (int)albums.size(); ++i)
    {
      CStdString value;
      CStdString strPath;
      CStdString strThumb;
      CStdString strDBpath;
      CStdString strSQLAlbum;
      CAlbum&    album=albums[i];     
      
      value.Format("%i", i + 1);
      musicdatabase.GetAlbumThumb(album.idAlbum,strThumb);
      strDBpath.Format("musicdb://3/%i/", album.idAlbum);
      strSQLAlbum.Format("idAlbum=%i", album.idAlbum);
      
      CStdString strArtist = musicdatabase.GetSingleValue("albumview", "strArtist", strSQLAlbum);
      
      home->SetProperty("LatestAlbum." + value + ".Title"   , musicdatabase.GetAlbumById(album.idAlbum));
      home->SetProperty("LatestAlbum." + value + ".Year"    , atoi(musicdatabase.GetSingleValue("album", "iYear", strSQLAlbum)));
      home->SetProperty("LatestAlbum." + value + ".Artist"  , strArtist);      
      home->SetProperty("LatestAlbum." + value + ".Rating"  , musicdatabase.GetSingleValue("albumview", "iRating", strSQLAlbum));
      home->SetProperty("LatestAlbum." + value + ".Path"    , strDBpath);
      home->SetProperty("LatestAlbum." + value + ".Thumb"   , strThumb);
      home->SetProperty("LatestAlbum." + value + ".Fanart"  , CFileItem::GetCachedThumb(strArtist,g_settings.GetMusicFanartFolder()));
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    CStdString value;
    value.Format("%i", i + 1);
    home->SetProperty("LatestAlbum." + value + ".Title"   , "");
    home->SetProperty("LatestAlbum." + value + ".Year"    , "");
    home->SetProperty("LatestAlbum." + value + ".Artist"  , "");      
    home->SetProperty("LatestAlbum." + value + ".Rating"  , "");
    home->SetProperty("LatestAlbum." + value + ".Path"    , "");
    home->SetProperty("LatestAlbum." + value + ".Thumb"   , "");
    home->SetProperty("LatestAlbum." + value + ".Fanart"  , "");            
  }
  
  musicdatabase.Close();
  return true;
}

bool CRecentlyAddedJob::UpdateTotal()
{
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);
  
  if ( home == NULL )
    return false;
  
  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateTotal() - Running RecentlyAdded home screen update");
  
  CVideoDatabase videodatabase;  
  CMusicDatabase musicdatabase;
  
  musicdatabase.Open();
  int MusSongTotals   = atoi(musicdatabase.GetSingleValue("songview"       , "count(1)"));
  int MusAlbumTotals  = atoi(musicdatabase.GetSingleValue("songview"       , "count(distinct strAlbum)"));
  int MusArtistTotals = atoi(musicdatabase.GetSingleValue("songview"       , "count(distinct strArtist)"));
  musicdatabase.Close();
 
  videodatabase.Open();
  int tvShowCount     = atoi(videodatabase.GetSingleValue("tvshowview"     , "count(1)"));
  int movieTotals     = atoi(videodatabase.GetSingleValue("movieview"      , "count(1)"));
  int movieWatched    = atoi(videodatabase.GetSingleValue("movieview"      , "count(playCount)"));
  int MusVidTotals    = atoi(videodatabase.GetSingleValue("musicvideoview" , "count(1)"));
  int MusVidWatched   = atoi(videodatabase.GetSingleValue("musicvideoview" , "count(playCount)"));
  int EpWatched       = atoi(videodatabase.GetSingleValue("tvshowview"     , "sum(watchedcount)"));
  int EpCount         = atoi(videodatabase.GetSingleValue("tvshowview"     , "sum(totalcount)"));
  int TvShowsWatched  = atoi(videodatabase.GetSingleValue("tvshowview"     , "sum(watchedcount = totalcount)"));
  videodatabase.Close();
  
  home->SetProperty("TVShows.Count"         , tvShowCount);
  home->SetProperty("TVShows.Watched"       , TvShowsWatched);
  home->SetProperty("TVShows.UnWatched"     , tvShowCount - TvShowsWatched);
  home->SetProperty("Episodes.Count"        , EpCount);
  home->SetProperty("Episodes.Watched"      , EpWatched);
  home->SetProperty("Episodes.UnWatched"    , EpCount-EpWatched);  
  home->SetProperty("Movies.Count"          , movieTotals);
  home->SetProperty("Movies.Watched"        , movieWatched);
  home->SetProperty("Movies.UnWatched"      , movieTotals - movieWatched);
  home->SetProperty("MusicVideos.Count"     , MusVidTotals);
  home->SetProperty("MusicVideos.Watched"   , MusVidWatched);
  home->SetProperty("MusicVideos.UnWatched" , MusVidTotals - MusVidWatched);
  home->SetProperty("Music.SongsCount"      , MusSongTotals);
  home->SetProperty("Music.AlbumsCount"     , MusAlbumTotals);
  home->SetProperty("Music.ArtistsCount"    , MusArtistTotals);
  
  return true;
}


bool CRecentlyAddedJob::DoWork()
{
  bool ret = true;
  if (m_flag & Audio)
    ret &= UpdateMusic();
  
  if (m_flag & Video)
    ret &= UpdateVideo();
  
  if (m_flag & Totals)
    ret &= UpdateTotal();
    
  return ret; 
}
