
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowHome.h"
#include "utils/JobManager.h"
#include "utils/RecentlyAddedJob.h"
#include "utils/log.h"

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
{
  m_updateRA = (Audio | Video | Totals);
  }

CGUIWindowHome::~CGUIWindowHome(void)
{
}

void CGUIWindowHome::OnInitWindow()
{  
  AddRecentlyAddedJobs( m_updateRA );

  CGUIWindow::OnInitWindow();
}

void CGUIWindowHome::AddRecentlyAddedJobs(int flag)
{
  if (flag != 0)
    CJobManager::GetInstance().AddJob(new CRecentlyAddedJob(flag), NULL);
  m_updateRA = 0;
}

