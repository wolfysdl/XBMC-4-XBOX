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
#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "FileSystem/Directory.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;

CGUIMultiImage::CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd)
    : CGUIControl(parentID, controlID, posX, posY, width, height),
      m_image(0, 0, posX, posY, width, height, texture)
{
  m_currentImage = 0;
  m_timePerImage = timePerImage + fadeTime;
  m_timeToPauseAtEnd = timeToPauseAtEnd;
  m_image.SetCrossFade(fadeTime);
  m_randomized = randomized;
  m_loop = loop;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_directoryLoaded = false;
}

CGUIMultiImage::CGUIMultiImage(const CGUIMultiImage &from)
: CGUIControl(from), m_image(from.m_image)
{
  m_texturePath = from.m_texturePath;
  m_timePerImage = from.m_timePerImage;
  m_timeToPauseAtEnd = from.m_timeToPauseAtEnd;
  m_randomized = from.m_randomized;
  m_loop = from.m_loop;
  m_bDynamicResourceAlloc=false;
  m_directoryLoaded = false;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
  m_currentImage = 0;
  ControlType = GUICONTROL_MULTI_IMAGE;
}

CGUIMultiImage::~CGUIMultiImage(void)
{
}

void CGUIMultiImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check if we're hidden, and deallocate if so
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && m_bAllocated)
      FreeResources();
    return;
  }

  // we are either delayed or visible, so we can allocate our resources

  // check for conditional information before we
  // alloc as this can free our resources
  if (!m_texturePath.IsConstant())
  {
    CStdString texturePath;
    if (item)
      texturePath = m_texturePath.GetItemLabel(item, true);
    else
      texturePath = m_texturePath.GetLabel(m_parentID);
    if (texturePath != m_currentPath && !texturePath.IsEmpty())
    {
      m_currentPath = texturePath;
      LoadDirectory();
      m_image.SetFileName(m_files.size() ? m_files[0] : "");
    }
  }
  // and allocate our resources
  if (!m_bAllocated)
    AllocResources();
}

void CGUIMultiImage::Render()
{
  // Set a viewport so that we don't render outside the defined area
  if (!m_files.empty() && g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
  {
    unsigned int nextImage = m_currentImage + 1;
    if (nextImage >= m_files.size())
      nextImage = m_loop ? 0 : m_currentImage;  // stay on the last image if <loop>no</loop>

    if (nextImage != m_currentImage)
    {
      // check if we should be loading a new image yet
      unsigned int timeToShow = m_timePerImage;
      if (0 == nextImage) // last image should be paused for a bit longer if that's what the skinner wishes.
        timeToShow += m_timeToPauseAtEnd;
      if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > timeToShow)
      {
        // grab a new image
        m_currentImage = nextImage;
        m_image.SetFileName(m_files[m_currentImage]);
        m_imageTimer.StartZero();
      }
    }
    m_image.SetColorDiffuse(m_diffuseColor);
    m_image.Render();
    g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

bool CGUIMultiImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIMultiImage::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (!m_texturePath.IsConstant())
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIMultiImage::AllocResources()
{
  FreeResources();
  CGUIControl::AllocResources();

  if (!m_directoryLoaded)
    LoadDirectory();

  // Load in the current image, and reset our timer
  m_currentImage = 0;
  m_imageTimer.StartZero();

  // and re-randomize if our control has been reallocated
  if (m_randomized)
    random_shuffle(m_files.begin(), m_files.end());

  m_image.SetFileName(m_files.size() ? m_files[0] : "");
}

void CGUIMultiImage::FreeResources(bool immediately)
{
  m_image.FreeResources(immediately);
  m_currentImage = 0;
  CGUIControl::FreeResources(immediately);
}

void CGUIMultiImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

bool CGUIMultiImage::CanFocus() const
{
  return false;
}

void CGUIMultiImage::SetAspectRatio(const CAspectRatio &ratio)
{
  m_image.SetAspectRatio(ratio);
}

void CGUIMultiImage::LoadDirectory()
{
  // Load any images from our texture bundle first
  m_files.clear();

  // don't load any images if our path is empty
  if (m_currentPath.IsEmpty()) return;

  // check to see if we have a single image or a folder of images
  CFileItem item(m_currentPath, false);
  if (item.IsPicture())
  {
    m_files.push_back(m_currentPath);
  }
  else
  { // folder of images
    g_TextureManager.GetBundledTexturesFromPath(m_currentPath, m_files);

    // Load in our images from the directory specified
    // m_currentPath is relative (as are all skin paths)
    CStdString realPath = g_TextureManager.GetTexturePath(m_currentPath, true);
    if (realPath.IsEmpty())
      return;

    URIUtils::AddSlashAtEnd(realPath);
    CFileItemList items;
    CDirectory::GetDirectory(realPath, items);
    for (int i=0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (pItem->IsPicture())
        m_files.push_back(pItem->GetPath());
    }
  }

  // Randomize or sort our images if necessary
  if (m_randomized)
    random_shuffle(m_files.begin(), m_files.end());
  else
    sort(m_files.begin(), m_files.end());

  // flag as loaded - no point in constantly reloading them
  m_directoryLoaded = true;
  m_imageTimer.StartZero();
}

void CGUIMultiImage::SetInfo(const CGUIInfoLabel &info)
{
  m_texturePath = info;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
}
