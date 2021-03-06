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

#include "dialogs/GUIDialogKaiToast.h"
#include "GUISliderControl.h"
#include "GUIImage.h"
#include "GUIAudioManager.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"

#define POPUP_ICON                400
#define POPUP_CAPTION_TEXT        401
#define POPUP_NOTIFICATION_BUTTON 402
#define POPUP_ICON_INFO           403
#define POPUP_ICON_WARNING        404
#define POPUP_ICON_ERROR          405

CGUIDialogKaiToast::CGUIDialogKaiToast(void)
: CGUIDialog(WINDOW_DIALOG_KAI_TOAST, "DialogKaiToast.xml")
{
  m_defaultIcon = "";
  m_loadOnDemand = false;
}

CGUIDialogKaiToast::~CGUIDialogKaiToast(void)
{
}

bool CGUIDialogKaiToast::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      ResetTimer();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogKaiToast::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  CGUIImage *image = (CGUIImage *)GetControl(POPUP_ICON);
  if (image)
    m_defaultIcon = image->GetFileName();
}

void CGUIDialogKaiToast::QueueNotification(eMessageType eType, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  CGUIImage *image    = NULL;

  if (eType == Info)
    image = (CGUIImage *)GetControl(POPUP_ICON_INFO);
  else if (eType == Warning)
    image = (CGUIImage *)GetControl(POPUP_ICON_WARNING);
  else if (eType == Error)
    image = (CGUIImage *)GetControl(POPUP_ICON_ERROR);

  CStdString strImage;
  if (image)
    strImage = image->GetFileName();

  QueueNotification(strImage, aCaption, aDescription, displayTime, withSound, messageTime);
}

void CGUIDialogKaiToast::QueueNotification(const CStdString& aCaption, const CStdString& aDescription)
{
  QueueNotification("", aCaption, aDescription);
}

void CGUIDialogKaiToast::QueueNotification(const CStdString& aImageFile, const CStdString& aCaption, const CStdString& aDescription, unsigned int displayTime /*= TOAST_DISPLAY_TIME*/, bool withSound /*= true*/, unsigned int messageTime /*= TOAST_MESSAGE_TIME*/)
{
  CSingleLock lock(m_critical);

  Notification toast;
  toast.imagefile = aImageFile;
  toast.caption = aCaption;
  toast.description = aDescription;
  toast.displayTime = displayTime > TOAST_MESSAGE_TIME + 500 ? displayTime : TOAST_MESSAGE_TIME + 500;
  toast.messageTime = messageTime;
  toast.withSound = withSound;

  m_notifications.push(toast);
}

bool CGUIDialogKaiToast::DoWork()
{
  CSingleLock lock(m_critical);

  if (m_notifications.size() > 0 &&
      CTimeUtils::GetFrameTime() - m_timer > TOAST_MESSAGE_TIME)
  {
    Notification toast = m_notifications.front();
    m_notifications.pop();
    lock.Leave();

    m_toastDisplayTime = toast.displayTime;
    m_toastMessageTime = toast.messageTime;

    CSingleLock lock2(g_graphicsContext);

    if(!Initialize())
      return false;

    SET_CONTROL_LABEL(POPUP_CAPTION_TEXT, toast.caption);

    SET_CONTROL_LABEL(POPUP_NOTIFICATION_BUTTON, toast.description);

    CGUIImage *image = (CGUIImage *)GetControl(POPUP_ICON);
    if (image)
    {
      if (!toast.imagefile.IsEmpty())
        image->SetFileName(toast.imagefile);
      else
        image->SetFileName(m_defaultIcon);
    }

    //  Play the window specific init sound for each notification queued
    SetSound(toast.withSound);

    ResetTimer();
    return true;
  }

  return false;
}


void CGUIDialogKaiToast::ResetTimer()
{
  m_timer = CTimeUtils::GetFrameTime();
}

void CGUIDialogKaiToast::FrameMove()
{
  //  Fading does not count as display time
  if (IsAnimating(ANIM_TYPE_WINDOW_OPEN))
    ResetTimer();

  // now check if we should exit
  if (CTimeUtils::GetFrameTime() - m_timer > m_toastDisplayTime)
    Close();
  
  CGUIDialog::FrameMove();
}
