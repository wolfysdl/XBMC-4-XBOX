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

#include "settings/GUIDialogSettings.h"
#include "MediaSource.h"
#include "Profile.h"

class CGUIDialogProfileSettings : public CGUIDialogSettings
{
public:
  CGUIDialogProfileSettings(void);
  virtual ~CGUIDialogProfileSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForProfile(unsigned int iProfile, bool bDetails=true);
protected:
  virtual void OnCancel();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void OnSettingChanged(unsigned int setting);
  virtual void OnSettingChanged(SettingInfo &setting);

  bool m_bNeedSave;
  CStdString m_strName;
  CStdString m_strThumb;
  CStdString m_strDirectory;
  int m_iSourcesMode;
  int m_iDbMode;
  bool m_bIsDefault;
  bool m_bIsNewUser;
  bool m_bShowDetails;

  CProfile::CLock m_locks;
  CStdString m_strDefaultImage;
};

