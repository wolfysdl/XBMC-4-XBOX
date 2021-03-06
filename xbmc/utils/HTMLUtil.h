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

namespace HTML
{
class CHTMLUtil
{
public:
  CHTMLUtil(void);
  virtual ~CHTMLUtil(void);
  int FindTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos = 0) const;
  int FindClosingTag(const CStdString& strHTML, const CStdString& strTag, CStdString& strtagFound, int iPos) const;
  void getValueOfTag(const CStdString& strTagAndValue, CStdString& strValue);
  void getAttributeOfTag(const CStdString& strTagAndValue, const CStdString& strTag, CStdString& strValue);
  static void RemoveTags(CStdString& strHTML);
  static void ConvertHTMLToW(const CStdStringW& strHTML, CStdStringW& strStripped);
};
}
