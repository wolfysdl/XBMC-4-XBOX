/*!
\file GUIToggleButtonControl.h
\brief
*/

#ifndef GUILIB_GUITOGGLEBUTTONCONTROL_H
#define GUILIB_GUITOGGLEBUTTONCONTROL_H

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

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIToggleButtonControl : public CGUIButtonControl
{
public:
  CGUIToggleButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CTextureInfo& altTextureFocus, const CTextureInfo& altTextureNoFocus, const CLabelInfo &labelInfo);
  virtual ~CGUIToggleButtonControl(void);
  virtual CGUIToggleButtonControl *Clone() const { return new CGUIToggleButtonControl(*this); };

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  void SetLabel(const std::string& strLabel);
  void SetAltLabel(const std::string& label);
  virtual CStdString GetLabel() const;
  void SetToggleSelect(int toggleSelect) { m_toggleSelect = toggleSelect; };
  void SetAltClickActions(const CGUIAction &clickActions);

protected:
  virtual void UpdateColors();
  virtual void OnClick();
  virtual void SetInvalid();
  CGUIButtonControl m_selectButton;
  int m_toggleSelect;
};
#endif
