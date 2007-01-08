/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogBoxBase.h"

CGUIDialogBoxBase::CGUIDialogBoxBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIDialog(dwID, xmlFile)
{
  m_bConfirmed = false;
}

CGUIDialogBoxBase::~CGUIDialogBoxBase(void)
{
}

bool CGUIDialogBoxBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bConfirmed = false;
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxBase::IsConfirmed() const
{
  return m_bConfirmed;
}

void CGUIDialogBoxBase::SetHeading(const string& strLine)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetHeading(int iString)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetLine(int iLine, const string& strLine)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetLine(int iLine, int iString)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), iLine + 2);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetChoice(int iButton, int iString) // iButton == 0 for no, 1 for yes
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 10+iButton);
  if (iString)
    msg.SetLabel(iString);
  else
    msg.SetLabel("");
  OnMessage(msg);
}

void CGUIDialogBoxBase::SetChoice(int iButton, const string& strString) // iButton == 0 for no, 1 for yes
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 10+iButton);
  msg.SetLabel(strString);
  OnMessage(msg);
}

void CGUIDialogBoxBase::OnInitWindow()
{
  // set focus to default
  m_lastControlID = m_dwDefaultFocusControlID;
  CGUIDialog::OnInitWindow();
}
