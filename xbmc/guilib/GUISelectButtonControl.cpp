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

#include "include.h"
#include "GUISelectButtonControl.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "utils/TimeUtils.h"

CGUISelectButtonControl::CGUISelectButtonControl(int parentID, int controlID,
    float posX, float posY,
    float width, float height,
    const CTextureInfo& buttonFocus,
    const CTextureInfo& button,
    const CLabelInfo& labelInfo,
    const CTextureInfo& selectBackground,
    const CTextureInfo& selectArrowLeft,
    const CTextureInfo& selectArrowLeftFocus,
    const CTextureInfo& selectArrowRight,
    const CTextureInfo& selectArrowRightFocus
                                                )
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, buttonFocus, button, labelInfo)
    , m_imgBackground(posX, posY, width, height, selectBackground)
    , m_imgLeft(posX, posY, 16, 16, selectArrowLeft)
    , m_imgLeftFocus(posX, posY, 16, 16, selectArrowLeftFocus)
    , m_imgRight(posX, posY, 16, 16, selectArrowRight)
    , m_imgRightFocus(posX, posY, 16, 16, selectArrowRightFocus)
{
  m_bShowSelect = false;
  m_iCurrentItem = -1;
  m_iDefaultItem = -1;
  m_iStartFrame = 0;
  m_bLeftSelected = false;
  m_bRightSelected = false;
  m_bMovedLeft = false;
  m_bMovedRight = false;
  m_ticks = 0;
  m_label.SetAlign(m_label.GetLabelInfo().align | XBFONT_CENTER_X);
  ControlType = GUICONTROL_SELECTBUTTON;
}

CGUISelectButtonControl::~CGUISelectButtonControl(void)
{}

void CGUISelectButtonControl::Render()
{
  if (m_bInvalidated)
  {
    m_imgBackground.SetWidth(m_width);
    m_imgBackground.SetHeight(m_height);
  }
  // Are we in selection mode
  if (m_bShowSelect)
  {
    // render background, left and right arrow
    m_imgBackground.Render();

    CGUILabel::COLOR color = CGUILabel::COLOR_TEXT;

    // User has moved left...
    if (m_bMovedLeft)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedLeft = false;
      }
      // If we are moving left
      // render item text as disabled
      color = CGUILabel::COLOR_DISABLED;
    }

    // Render arrow
    if (m_bLeftSelected || m_bMovedLeft)
      m_imgLeftFocus.Render();
    else
      m_imgLeft.Render();

    // User has moved right...
    if (m_bMovedRight)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedRight = false;
      }
      // If we are moving right
      // render item text as disabled
      color = CGUILabel::COLOR_DISABLED;
    }

    // Render arrow
    if (m_bRightSelected || m_bMovedRight)
      m_imgRightFocus.Render();
    else
      m_imgRight.Render();

    // Render text if a current item is available
    if (m_iCurrentItem >= 0 && (unsigned)m_iCurrentItem < m_vecItems.size())
    {
      m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
      m_label.SetText(m_vecItems[m_iCurrentItem]);
      m_label.SetColor(color);
      m_label.Render();
    }

    // Select current item, if user doesn't
    // move left or right for 1.5 sec.
    unsigned int ticksSpan = CTimeUtils::GetFrameTime() - m_ticks;
    if (ticksSpan > 1500)
    {
      // User hasn't moved disable selection mode...
      m_bShowSelect = false;

      // ...and send a thread message.
      // (Sending a message with SendMessage
      // can result in a GPF.)
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      g_windowManager.SendThreadMessage(message);
    }
  } // if (m_bShowSelect)
  else
  {
    // No, render a normal button
    CGUIButtonControl::Render();
  }
  CGUIControl::Render();
}

bool CGUISelectButtonControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      if (m_vecItems.size() <= 0)
      {
        m_iCurrentItem = 0;
        m_iDefaultItem = 0;
      }
      m_vecItems.push_back(message.GetLabel());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_iCurrentItem = -1;
      m_iDefaultItem = -1;
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCurrentItem);
      if (m_iCurrentItem >= 0 && m_iCurrentItem < (int)m_vecItems.size())
        message.SetLabel(m_vecItems[m_iCurrentItem]);
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      m_iDefaultItem = m_iCurrentItem = message.GetParam1();
      return true;
    }
  }

  return CGUIButtonControl::OnMessage(message);
}

bool CGUISelectButtonControl::OnAction(const CAction &action)
{
  if (!m_bShowSelect)
  {
    if (action.GetID() == ACTION_SELECT_ITEM)
    {
      // Enter selection mode
      m_bShowSelect = true;

      // Start timer, if user doesn't select an item
      // or moves left/right. The control will
      // automatically select the current item.
      m_ticks = CTimeUtils::GetFrameTime();
      return true;
    }
    else
      return CGUIButtonControl::OnAction(action);
  }
  else
  {
    if (action.GetID() == ACTION_SELECT_ITEM)
    {
      // User has selected an item, disable selection mode...
      m_bShowSelect = false;

      // ...and send a message.
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      SendWindowMessage(message);
      return true;
    }
    if (action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN )
    {
      // Disable selection mode when moving up or down
      m_bShowSelect = false;
      m_iCurrentItem = m_iDefaultItem;
    }
    // call the base class
    return CGUIButtonControl::OnAction(action);
  }
}

void CGUISelectButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);

  m_imgBackground.FreeResources(immediately);

  m_imgLeft.FreeResources(immediately);
  m_imgLeftFocus.FreeResources(immediately);

  m_imgRight.FreeResources(immediately);
  m_imgRightFocus.FreeResources(immediately);

  m_bShowSelect = false;
}

void CGUISelectButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);

  m_imgBackground.DynamicResourceAlloc(bOnOff);

  m_imgLeft.DynamicResourceAlloc(bOnOff);
  m_imgLeftFocus.DynamicResourceAlloc(bOnOff);

  m_imgRight.DynamicResourceAlloc(bOnOff);
  m_imgRightFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISelectButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();

  m_imgBackground.PreAllocResources();

  m_imgLeft.PreAllocResources();
  m_imgLeftFocus.PreAllocResources();

  m_imgRight.PreAllocResources();
  m_imgRightFocus.PreAllocResources();
}

void CGUISelectButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();

  m_imgBackground.AllocResources();

  m_imgLeft.AllocResources();
  m_imgLeftFocus.AllocResources();

  m_imgRight.AllocResources();
  m_imgRightFocus.AllocResources();

  // Position right arrow
  float posX = (m_posX + m_width - 8) - 16;
  float posY = m_posY + (m_height - 16) / 2;
  m_imgRight.SetPosition(posX, posY);
  m_imgRightFocus.SetPosition(posX, posY);

  // Position left arrow
  posX = m_posX + 8;
  m_imgLeft.SetPosition(posX, posY);
  m_imgLeftFocus.SetPosition(posX, posY);
}

void CGUISelectButtonControl::OnLeft()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedLeft = true;
    m_iStartFrame = 0;

    // Reset timer for automatically selecting
    // the current item.
    m_ticks = CTimeUtils::GetFrameTime();

    // Switch to previous item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem--;
      if (m_iCurrentItem < 0)
        m_iCurrentItem = (int)m_vecItems.size() - 1;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnLeft();
  }
}

void CGUISelectButtonControl::OnRight()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedRight = true;
    m_iStartFrame = 0;

    // Reset timer for automatically selecting
    // the current item.
    m_ticks = CTimeUtils::GetFrameTime();

    // Switch to next item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem++;
      if (m_iCurrentItem >= (int)m_vecItems.size())
        m_iCurrentItem = 0;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnRight();
  }
}

bool CGUISelectButtonControl::OnMouseOver(const CPoint &point)
{
  bool ret = CGUIControl::OnMouseOver(point);
  m_bLeftSelected = false;
  m_bRightSelected = false;
  if (m_imgLeft.HitTest(point))
  { // highlight the left control, but don't start moving until we have clicked
    m_bLeftSelected = true;
  }
  if (m_imgRight.HitTest(point))
  { // highlight the right control, but don't start moving until we have clicked
    m_bRightSelected = true;
  }
  // reset ticks
  m_ticks = CTimeUtils::GetFrameTime();
  return ret;
}

bool CGUISelectButtonControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_bShowSelect && m_imgLeft.HitTest(point))
      OnLeft();
    else if (m_bShowSelect && m_imgRight.HitTest(point))
      OnRight();
    else // normal select
      CGUIButtonControl::OnMouseEvent(point, event);
    return true;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    OnLeft();
    return true;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    OnRight();
    return true;
  }
  return false;
}

void CGUISelectButtonControl::SetPosition(float posX, float posY)
{
  float leftOffX = m_imgLeft.GetXPosition() - m_posX;
  float leftOffY = m_imgLeft.GetYPosition() - m_posY;
  float rightOffX = m_imgRight.GetXPosition() - m_posX;
  float rightOffY = m_imgRight.GetYPosition() - m_posY;
  float backOffX = m_imgBackground.GetXPosition() - m_posX;
  float backOffY = m_imgBackground.GetYPosition() - m_posY;
  CGUIButtonControl::SetPosition(posX, posY);
  m_imgLeft.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgLeftFocus.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgRight.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgRightFocus.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgBackground.SetPosition(posX + backOffX, posY + backOffY);
}

void CGUISelectButtonControl::UpdateColors()
{
  CGUIButtonControl::UpdateColors();
  m_imgLeft.SetDiffuseColor(m_diffuseColor);
  m_imgLeftFocus.SetDiffuseColor(m_diffuseColor);
  m_imgRight.SetDiffuseColor(m_diffuseColor);
  m_imgRightFocus.SetDiffuseColor(m_diffuseColor);
  m_imgBackground.SetDiffuseColor(m_diffuseColor);
}

