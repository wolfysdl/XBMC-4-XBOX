/*!
\file GUIMultiImage.h
\brief
*/

#ifndef GUILIB_GUIMULTIIMAGECONTROL_H
#define GUILIB_GUIMULTIIMAGECONTROL_H

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

#include "GUIImage.h"
#include "utils/Stopwatch.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIMultiImage : public CGUIControl
{
public:
  CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd);
  CGUIMultiImage(const CGUIMultiImage &from);
  virtual ~CGUIMultiImage(void);
  virtual CGUIMultiImage *Clone() const { return new CGUIMultiImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;

  void SetInfo(const CGUIInfoLabel &info);
  void SetAspectRatio(const CAspectRatio &ratio);

protected:
  void LoadDirectory();

  CGUIInfoLabel m_texturePath;
  CStdString m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  unsigned int m_timePerImage;
  unsigned int m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;

  bool m_bDynamicResourceAlloc;
  bool m_directoryLoaded;
  std::vector<CStdString> m_files;

  CGUIImage m_image;
};
#endif
