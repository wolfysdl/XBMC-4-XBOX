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

#include "Weather.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/CurlFile.h"
#include "XMLUtils.h"
#include "utils/POUtils.h"
#include "Temperature.h"
#include "xbox/network.h"
#include "Util.h"
#include "Application.h"
#include "settings/GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "XBDateTime.h"
#include "LangInfo.h"
#include "LocalizeStrings.h"
#include "FileSystem/Directory.h"
#include "StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;

#define CONTROL_BTNREFRESH  2
#define CONTROL_SELECTLOCATION 3
#define CONTROL_LABELLOCATION 10
#define CONTROL_LABELUPDATED 11
#define CONTROL_IMAGELOGO  101

#define CONTROL_IMAGENOWICON 21
#define CONTROL_LABELNOWCOND 22
#define CONTROL_LABELNOWTEMP 23
#define CONTROL_LABELNOWFEEL 24
#define CONTROL_LABELNOWUVID 25
#define CONTROL_LABELNOWWIND 26
#define CONTROL_LABELNOWDEWP 27
#define CONTROL_LABELNOWHUMI 28

#define CONTROL_STATICTEMP  223
#define CONTROL_STATICFEEL  224
#define CONTROL_STATICUVID  225
#define CONTROL_STATICWIND  226
#define CONTROL_STATICDEWP  227
#define CONTROL_STATICHUMI  228

#define CONTROL_LABELD0DAY  31
#define CONTROL_LABELD0HI  32
#define CONTROL_LABELD0LOW  33
#define CONTROL_LABELD0GEN  34
#define CONTROL_IMAGED0IMG  35

#define MAX_LOCATION   3
#define LOCALIZED_TOKEN_FIRSTID   370
#define LOCALIZED_TOKEN_LASTID   395
#define LOCALIZED_TOKEN_FIRSTID2 1396
#define LOCALIZED_TOKEN_LASTID2   1450 
/*
FIXME'S
>strings are not centered
*/

// USE THESE FOR ZIP
//#define WEATHER_BASE_PATH "Z:\\weather\\"
//#define WEATHER_USE_ZIP 1
//#define WEATHER_USE_RAR 0
//#define WEATHER_SOURCE_FILE "special://xbmc/media/weather.zip"

// OR THESE FOR RAR
#define WEATHER_BASE_PATH "special://temp/weather/"
#define WEATHER_USE_ZIP 0
#define WEATHER_USE_RAR 1
#define WEATHER_SOURCE_FILE "special://xbmc/media/weather.rar"

CWeather g_weatherManager;

bool CWeather::DoWork()
{
  if (!g_application.getNetwork().IsAvailable())
    return false;

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");
  XFILE::CCurlFile httpUtil;
  CStdString strURL;

  CStdString strSetting;
  strSetting.Format("weather.areacode%i", GetArea() + 1);
  CStdString areaCode = GetAreaCode(g_guiSettings.GetString(strSetting));
  strURL.Format("http://wxdata.weather.com/wxdata/weather/local/%s?cc=*&unit=m&dayf=7", areaCode.c_str());
  CStdString xml;
  if (httpUtil.Get(strURL, xml))
  {
    CLog::Log(LOGINFO, "WEATHER: Weather download successful");
    if (!m_bImagesOkay)
    {
      CDirectory::Create(WEATHER_BASE_PATH);
      if (WEATHER_USE_ZIP)
        g_ZipManager.ExtractArchive(WEATHER_SOURCE_FILE, WEATHER_BASE_PATH);
      else if (WEATHER_USE_RAR)
        g_RarManager.ExtractArchive(WEATHER_SOURCE_FILE, WEATHER_BASE_PATH);
      m_bImagesOkay = true;
    }
    LoadWeather(xml);
    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    g_windowManager.SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

CWeather::CWeather(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  m_bImagesOkay = false;

  Reset();
}

CWeather::~CWeather(void)
{
}

void CWeather::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &value, const CStdString& strDefaultValue)
{
  value = "";
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild && pChild->FirstChild())
  {
    value = pChild->FirstChild()->Value();
    if (value == "-")
      value = "";
  }
  if (value.IsEmpty())
    value = strDefaultValue;
}

void CWeather::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue)
{
  if (!XMLUtils::GetInt(pRootElement, strTagName.c_str(), iValue))
    iValue = 0;
}

void CWeather::LocalizeOverviewToken(CStdString &token)
{
  // NOTE: This routine is case-sensitive.  Reason is std::less<CStdString> uses a case-sensitive
  //       < operator.  Thus, some tokens may have to be duplicated in strings.xml (see drizzle vs Drizzle).
  CStdString strLocStr = "";
  if (!token.IsEmpty())
  {
    ilocalizedTokens i;
    i = m_localizedTokens.find(token);
    if (i != m_localizedTokens.end())
    {
      strLocStr = g_localizeStrings.Get(i->second);
    }
  }
  if (strLocStr == "")
    strLocStr = token; //if not found, let fallback
  token = strLocStr;
}

void CWeather::LocalizeOverview(CStdString &str)
{
  CStdStringArray words;
  StringUtils::SplitString(str, " ", words);
  str.clear();
  for (unsigned int i = 0; i < words.size(); i++)
  {
    LocalizeOverviewToken(words[i]);
    str += words[i] + " ";
  }
  str.TrimRight(" ");
}

// input param must be kmh
int CWeather::ConvertSpeed(int curSpeed)
{
  switch (g_langInfo.GetSpeedUnit())
  {
  case CLangInfo::SPEED_UNIT_KMH:
    break;
  case CLangInfo::SPEED_UNIT_MPS:
    curSpeed=(int)(curSpeed * (1000.0 / 3600.0) + 0.5);
    break;
  case CLangInfo::SPEED_UNIT_MPH:
    curSpeed=(int)(curSpeed / (8.0 / 5.0));
    break;
  case CLangInfo::SPEED_UNIT_MPMIN:
    curSpeed=(int)(curSpeed * (1000.0 / 3600.0) + 0.5*60);
    break;
  case CLangInfo::SPEED_UNIT_FTH:
    curSpeed=(int)(curSpeed * 3280.8398888889f);
    break;
  case CLangInfo::SPEED_UNIT_FTMIN:
    curSpeed=(int)(curSpeed * 54.6805555556f);
    break;
  case CLangInfo::SPEED_UNIT_FTS:
    curSpeed=(int)(curSpeed * 0.911344f);
    break;
  case CLangInfo::SPEED_UNIT_KTS:
    curSpeed=(int)(curSpeed * 0.5399568f);
    break;
  case CLangInfo::SPEED_UNIT_INCHPS:
    curSpeed=(int)(curSpeed * 10.9361388889f);
    break;
  case CLangInfo::SPEED_UNIT_YARDPS:
    curSpeed=(int)(curSpeed * 0.3037814722f);
    break;
  case CLangInfo::SPEED_UNIT_FPF:
    curSpeed=(int)(curSpeed * 1670.25f);
    break;
  case CLangInfo::SPEED_UNIT_BEAUFORT:
    {
      float knot=(float)curSpeed * 0.5399568f; // to kts first
      if(knot<=1.0) curSpeed=0;
      if(knot>1.0 && knot<3.5) curSpeed=1;
      if(knot>=3.5 && knot<6.5) curSpeed=2;
      if(knot>=6.5 && knot<10.5) curSpeed=3;
      if(knot>=10.5 && knot<16.5) curSpeed=4;
      if(knot>=16.5 && knot<21.5) curSpeed=5;
      if(knot>=21.5 && knot<27.5) curSpeed=6;
      if(knot>=27.5 && knot<33.5) curSpeed=7;
      if(knot>=33.5 && knot<40.5) curSpeed=8;
      if(knot>=40.5 && knot<47.5) curSpeed=9;
      if(knot>=47.5 && knot<55.5) curSpeed=10;
      if(knot>=55.5 && knot<63.5) curSpeed=11;
      if(knot>=63.5 && knot<74.5) curSpeed=12;
      if(knot>=74.5 && knot<80.5) curSpeed=13;
      if(knot>=80.5 && knot<89.5) curSpeed=14;
      if(knot>=89.5) curSpeed=15;
    }
    break;
  default:
    assert(false);
  }

  return curSpeed;
}

bool CWeather::LoadWeather(const CStdString &weatherXML)
{
  int iTmpInt;
  CStdString iTmpStr;
  SYSTEMTIME time;

  GetLocalTime(&time); //used when deciding what weather to grab for today

  // Load in our tokens if necessary
  if (!m_localizedTokens.size())
    LoadLocalizedToken();

  // load the xml file
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(weatherXML))
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (!pRootElement)
  {
    CLog::Log(LOGERROR, "WEATHER: Unable to get data - invalid XML");
    return false;
  }

  //if root element is 'error' display the error message
  if (strcmp(pRootElement->Value(), "error") == 0)
  {
    CStdString error;
    GetString(pRootElement, "err", error, "Unknown Error"); //grab the error string
    CLog::Log(LOGERROR, "WEATHER: Unable to get data: %s", error.c_str());
    return false;
  }

  // location
  TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
  if (pElement)
  {
    GetString(pElement, "dnam", m_location[m_iCurWeather], "");
  }

  //current weather
  pElement = pRootElement->FirstChildElement("cc");
  if (pElement)
  {
    // Use the local date/time the file is parsed...
    CDateTime time=CDateTime::GetCurrentDateTime();
    m_lastUpdateTime = time.GetAsLocalizedDateTime(false, false);

    // ...and not the date/time from weather.com
    //GetString(pElement, "lsup", m_szLastUpdateTime, "");

    GetString(pElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
    if (iTmpStr == "")
      m_currentIcon.Format("%s128x128/na.png", WEATHER_BASE_PATH);
    else
      m_currentIcon.Format("%s128x128/%s.png", WEATHER_BASE_PATH, iTmpStr.c_str());

    GetString(pElement, "t", m_currentConditions, "");   //current condition
    LocalizeOverview(m_currentConditions);

    GetInteger(pElement, "tmp", iTmpInt);    //current temp
    CTemperature temp=CTemperature::CreateFromCelsius(iTmpInt);
    m_currentTemperature.Format("%2.0f", temp.ToLocale());
    GetInteger(pElement, "flik", iTmpInt);    //current 'Feels Like'
    CTemperature tempFlik=CTemperature::CreateFromCelsius(iTmpInt);
    m_currentFeelsLike.Format("%2.0f", tempFlik.ToLocale());

    TiXmlElement *pNestElement = pElement->FirstChildElement("wind"); //current wind
    if (pNestElement)
    {
      GetInteger(pNestElement, "s", iTmpInt);   //current wind strength
      iTmpInt = ConvertSpeed(iTmpInt);    //convert speed if needed
      GetString(pNestElement, "t", iTmpStr, "N");  //current wind direction

      //From <dir eg NW> at <speed> km/h   g_localizeStrings.Get(407)
      //This is a bit untidy, but i'm fed up with localization and string formats :)
      CStdString szWindFrom = g_localizeStrings.Get(407);
      CStdString szWindAt = g_localizeStrings.Get(408);
      CStdString szCalm = g_localizeStrings.Get(1410);

      if (iTmpStr ==  "CALM")
        m_currentWind = szCalm;
      else
        m_currentWind.Format("%s %s %s %i %s",
              szWindFrom.c_str(), iTmpStr,
              szWindAt.c_str(), iTmpInt, g_langInfo.GetSpeedUnitString().c_str());
    }

    GetInteger(pElement, "hmid", iTmpInt);    //current humidity
    m_currentHumidity.Format("%i%%", iTmpInt);

    pNestElement = pElement->FirstChildElement("uv"); //current UV index
    if (pNestElement)
    {
      GetInteger(pNestElement, "i", iTmpInt);
      GetString(pNestElement, "t", iTmpStr, "");
      LocalizeOverviewToken(iTmpStr);
      m_currentUVIndex.Format("%i %s", iTmpInt, iTmpStr);
    }

    GetInteger(pElement, "dewp", iTmpInt);    //current dew point
    CTemperature dewPoint=CTemperature::CreateFromCelsius(iTmpInt);
    m_currentDewPoint.Format("%2.0f", dewPoint.ToLocale());
  }
  //future forcast
  pElement = pRootElement->FirstChildElement("dayf");
  if (pElement)
  {
    TiXmlElement *pOneDayElement = pElement->FirstChildElement("day");;
    if (pOneDayElement)
    {
      for (int i = 0; i < NUM_DAYS; i++)
      {
        const char *attr = pOneDayElement->Attribute("t");
        if (attr)
        {
          m_dfForcast[i].m_day = attr;
          LocalizeDay(m_dfForcast[i].m_day);
        }

        GetString(pOneDayElement, "hi", iTmpStr, ""); //string cause i've seen it return N/A
        if (iTmpStr == "N/A")
          m_dfForcast[i].m_high = "";
        else
        {
          CTemperature temp=CTemperature::CreateFromCelsius(atoi(iTmpStr));
          m_dfForcast[i].m_high.Format("%2.0f", temp.ToLocale());
        }

        GetString(pOneDayElement, "low", iTmpStr, "");
        if (iTmpStr == "N/A")
          m_dfForcast[i].m_high = "";
        else
        {
          CTemperature temp=CTemperature::CreateFromCelsius(atoi(iTmpStr));
          m_dfForcast[i].m_low.Format("%2.0f", temp.ToLocale());
        }

        TiXmlElement *pDayTimeElement = pOneDayElement->FirstChildElement("part"); //grab the first day/night part (should be day)
        if (pDayTimeElement)
        {
          GetString(pDayTimeElement, "icon", iTmpStr, ""); //string cause i've seen it return N/A
          if (iTmpStr == "N/A")
            m_dfForcast[i].m_icon.Format("%s128x128/na.png", WEATHER_BASE_PATH);
          else
            m_dfForcast[i].m_icon.Format("%s128x128/%s.png", WEATHER_BASE_PATH, iTmpStr);

          GetString(pDayTimeElement, "t", m_dfForcast[i].m_overview, "");
          LocalizeOverview(m_dfForcast[i].m_overview);
        }

        pOneDayElement = pOneDayElement->NextSiblingElement("day");
        if (!pOneDayElement)
          break; // No more days, break out
      }
    }
  }
  return true;
}

//convert weather.com day strings into localized string id's
void CWeather::LocalizeDay(CStdString &day)
{
  if (day == "Monday")   //monday is localized string 11
    day = g_localizeStrings.Get(11);
  else if (day == "Tuesday")
    day = g_localizeStrings.Get(12);
  else if (day == "Wednesday")
    day = g_localizeStrings.Get(13);
  else if (day == "Thursday")
    day = g_localizeStrings.Get(14);
  else if (day == "Friday")
    day = g_localizeStrings.Get(15);
  else if (day == "Saturday")
    day = g_localizeStrings.Get(16);
  else if (day == "Sunday")
    day = g_localizeStrings.Get(17);
  else
    day = "";
}


void CWeather::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens

  // Try the strings PO file first
  CPODocument PODoc;
  if (PODoc.LoadFile("special://xbmc/language/English/strings.po"))
  {
    int counter = 0;

    while (PODoc.GetNextEntry())
    {
      if (PODoc.GetEntryType() != ID_FOUND)
        continue;

      uint32_t id = PODoc.GetEntryID();
      PODoc.ParseEntry(ISSOURCELANG);

      if (id > LOCALIZED_TOKEN_LASTID2) break;
      if ((LOCALIZED_TOKEN_FIRSTID  <= id && id <= LOCALIZED_TOKEN_LASTID)  ||
          (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2))
      {
        if (!PODoc.GetMsgid().empty())
        {
          m_localizedTokens.insert(make_pair(PODoc.GetMsgid(), id));
          counter++;
        }
      }
    }

    CLog::Log(LOGDEBUG, "POParser: loaded %i weather tokens", counter);
    return;
  }

  CLog::Log(LOGDEBUG,
            "Weather: no PO string file available, to load English tokens, "
            "fallback to strings.xml file");

  // We load the tokens from the strings.xml file
  CStdString strLanguagePath = "special://xbmc/language/English/strings.xml";
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strLanguagePath) || !xmlDoc.RootElement())
  {
    CLog::Log(LOGERROR, "Weather: unable to load %s: %s at line %d", strLanguagePath.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return;
  }

  CStdString strEncoding;
  XMLUtils::GetEncoding(&xmlDoc, strEncoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->Value() != CStdString("strings"))
    return;

  const TiXmlElement *pChild = pRootElement->FirstChildElement();
  while (pChild)
  {
    CStdString strValue = pChild->Value();
    if (strValue == "string")
    { // Load new style language file with id as attribute
      const char* attrId = pChild->Attribute("id");
      if (attrId && !pChild->NoChildren())
      {
        int id = atoi(attrId);
        if ((LOCALIZED_TOKEN_FIRSTID <= id && id <= LOCALIZED_TOKEN_LASTID) ||
            (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2))
        {
          CStdString utf8Label;
          if (strEncoding.IsEmpty()) // Is language file utf8?
            utf8Label=pChild->FirstChild()->Value();
          else
            g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8Label);

          if (!utf8Label.IsEmpty())
            m_localizedTokens.insert(make_pair(utf8Label, id));
        }
      }
    }
    pChild = pChild->NextSiblingElement();
  }
}

bool CWeather::GetSearchResults(const CStdString &strSearch, CStdString &strResult)
{
  // Check to see if the user entered a weather.com code
  if (strSearch.size() == 8)
  {
    strResult = "";
    int i = 0;
    for (i = 0; i < 4; ++i)
    {
      strResult += toupper(strSearch[i]);
      if (!isalpha(strSearch[i]))
        break;
    }
    if (i == 4)
    {
      for ( ; i < 8; ++i)
      {
        strResult += strSearch[i];
        if (!isdigit(strSearch[i]))
          break;
      }
      if (i == 8)
      {
        return true; // match
      }
    }
    // no match, wipe string
    strResult = "";
  }

  CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogProgress *pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  //do the download
  CStdString strURL;
  CStdString strXML;
  XFILE::CCurlFile httpUtil;

  if (pDlgProgress)
  {
    pDlgProgress->SetHeading(410);       //"Accessing Weather.com"
    pDlgProgress->SetLine(0, 194);       //"Searching"
    pDlgProgress->SetLine(1, strSearch);
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal();
    pDlgProgress->Progress();
  }

  strURL.Format("http://wxdata.weather.com/wxdata/search/search?where=%s", strSearch);

  if (!httpUtil.Get(strURL, strXML))
  {
    if (pDlgProgress)
      pDlgProgress->Close();
    return false;
  }

  //some select dialog init stuff
  if (!pDlgSelect)
  {
    if (pDlgProgress)
      pDlgProgress->Close();
    return false;
  }

  pDlgSelect->SetHeading(396); //"Select Location"
  pDlgSelect->Reset();

  ///////////////////////////////
  // load the xml file
  ///////////////////////////////
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strXML.c_str());
  if (xmlDoc.Error())
    return false;

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (pRootElement)
  {
    CStdString strItemTmp;
    TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
    while (pElement)
    {
      if (!pElement->NoChildren())
      {
        strItemTmp.Format("%s - %s", pElement->Attribute("id"), pElement->FirstChild()->Value());
        pDlgSelect->Add(strItemTmp);
      }
      pElement = pElement->NextSiblingElement("loc");
    }
  }

  if (pDlgProgress)
    pDlgProgress->Close();

  pDlgSelect->EnableButton(TRUE);
  pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
  pDlgSelect->DoModal();

  if (pDlgSelect->GetSelectedLabel() < 0)
  {
    if (pDlgSelect->IsButtonPressed())
    {
      pDlgSelect->Close(); //close the select dialog and return to weather settings
      return true;
    }
  }

  //copy the selected code into the settings
  if (pDlgSelect->GetSelectedLabel() >= 0)
    strResult = pDlgSelect->GetSelectedLabelText();

  if (pDlgProgress)
    pDlgProgress->Close();

  return true;
}

CStdString CWeather::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
  {
    CStdString busy;
    busy.Format("%s128x128/na.png", WEATHER_BASE_PATH);
    return busy;
  }
  return CInfoLoader::BusyInfo(info);
}

CStdString CWeather::TranslateInfo(int info) const
{
  if (info == WEATHER_LABEL_CURRENT_COND) return m_currentConditions;
  else if (info == WEATHER_IMAGE_CURRENT_ICON) return m_currentIcon;
  else if (info == WEATHER_LABEL_CURRENT_TEMP) return m_currentTemperature;
  else if (info == WEATHER_LABEL_CURRENT_FEEL) return m_currentFeelsLike;
  else if (info == WEATHER_LABEL_CURRENT_UVID) return m_currentUVIndex;
  else if (info == WEATHER_LABEL_CURRENT_WIND) return m_currentWind;
  else if (info == WEATHER_LABEL_CURRENT_DEWP) return m_currentDewPoint;
  else if (info == WEATHER_LABEL_CURRENT_HUMI) return m_currentHumidity;
  else if (info == WEATHER_LABEL_LOCATION) return m_location[m_iCurWeather];
  return "";
}

CStdString CWeather::GetAreaCity(const CStdString &codeAndCity) const
{
  CStdString areaCode(codeAndCity);
  int pos = areaCode.Find(" - ");
  if (pos >= 0)
    areaCode = areaCode.Mid(pos + 3);
  return areaCode;
}

CStdString CWeather::GetAreaCode(const CStdString &codeAndCity) const
{
  CStdString areaCode(codeAndCity);
  int pos = areaCode.Find(" - ");
  if (pos >= 0)
    areaCode = areaCode.Left(pos);
  return areaCode;
}

CStdString CWeather::GetLocation(int iLocation)
{
  if (m_location[iLocation].IsEmpty())
  {
    CStdString setting;
    setting.Format("weather.areacode%i", iLocation + 1);
    m_location[iLocation] = GetAreaCity(g_guiSettings.GetString(setting));
  }
  return m_location[iLocation];
}

void CWeather::Reset()
{
  m_lastUpdateTime = "";
  m_currentIcon = "";
  m_currentConditions = "";
  m_currentTemperature = "";
  m_currentFeelsLike = "";
  m_currentWind = "";
  m_currentHumidity = "";
  m_currentUVIndex = "";
  m_currentDewPoint = "";

  for (int i = 0; i < NUM_DAYS; i++)
  {
    m_dfForcast[i].m_icon = "";
    m_dfForcast[i].m_overview = "";
    m_dfForcast[i].m_day = "";
    m_dfForcast[i].m_high = "";
    m_dfForcast[i].m_low = "";
  }
  for (int i = 0; i < MAX_LOCATION; i++)
    m_location[i] = "";
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_lastUpdateTime.IsEmpty();
}

