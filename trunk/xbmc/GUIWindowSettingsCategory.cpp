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

#include "system.h"
#include "GUIUserMessages.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "utils/Builtins.h"
#include "KeyboardLayoutConfiguration.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIEditControl.h"
#include "GUIImage.h"
#include "utils/Weather.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "ProgramDatabase.h"
#include "ViewDatabase.h"
#include "XBAudioConfig.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "visualizations/VisualisationFactory.h"
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIWindowPrograms.h"
#include "MediaManager.h"
#include "utils/Network.h"
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/WebServer.h"
#endif
#include "GUIControlGroupList.h"
#include "GUIWindowManager.h"
#include "GUIFontManager.h"
#ifdef _LINUX
#include "LinuxTimezone.h"
#include <dlfcn.h>
#include "cores/AudioRenderers/AudioRendererFactory.h"
#ifndef __APPLE__
#include "cores/AudioRenderers/ALSADirectSound.h"
#endif
#ifdef HAS_HAL
#include "HALManager.h"
#endif
#endif
#ifdef __APPLE__
#include "CoreAudio.h"
#include "XBMCHelper.h"
#endif
#include "GUIDialogAccessPoints.h"
#include "FileSystem/Directory.h"
#include "utils/ScraperParser.h"

#include "FileItem.h"
#include "GUIToggleButtonControl.h"
#include "FileSystem/SpecialProtocol.h"
#include "File.h"

#include "Zeroconf.h"
#include "PowerManager.h"

#ifdef _WIN32
#include "WIN32Util.h"
#include "WINDirectSound.h"
#endif
#include <map>
#include "ScraperSettings.h"
#include "ScriptSettings.h"
#include "GUIDialogPluginSettings.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "MouseStat.h"
#include "LocalizeStrings.h"
#include "LangInfo.h"
#include "StringUtils.h"
#include "WindowingFactory.h"

using namespace std;
using namespace DIRECTORY;

#define CONTROL_GROUP_BUTTONS           0
#define CONTROL_GROUP_SETTINGS          1
#define CONTROL_SETTINGS_LABEL          2
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_START_BUTTONS           -100
#define CONTROL_START_CONTROL           -80

#define PREDEFINED_SCREENSAVERS          5

#define RSSEDITOR_PATH "special://home/scripts/RSS Editor/default.py"

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalCategoryButton = NULL;
  m_pOriginalImage = NULL;
  m_pOriginalEdit = NULL;
  // set the correct ID range...
  m_idRange = 8;
  m_iScreen = 0;
  // set the network settings so that we don't reset them unnecessarily
  m_iNetworkAssignment = -1;
  m_strErrorMessage = "";
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
  m_returningFromSkinLoad = false;
  m_delayedSetting = NULL;
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
  FreeControls();
  delete m_pOriginalEdit;
}

bool CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_settings.Save();
    if (m_iWindowBeforeJump!=WINDOW_INVALID)
    {
      JumpToPreviousSection();
      return true;
    }
    m_lastControlID = 0; // don't save the control as we go to a different window each time
    g_windowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      /*   if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size())
         {
          // change the setting...
          m_iSection = iControl-CONTROL_START_BUTTONS;
          CheckNetworkSettings();
          CreateSettings();
          return true;
         }*/
      for (unsigned int i = 0; i < m_vecSettings.size(); i++)
      {
        if (m_vecSettings[i]->GetID() == (int)iControl)
          OnClick(m_vecSettings[i]);
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < (int)(CONTROL_START_BUTTONS + m_vecSections.size()) &&
          focusedControl - CONTROL_START_BUTTONS != m_iSection)
      {
        // changing section, check for updates and cancel any delayed changes
        m_delayedSetting = NULL;
        CheckForUpdates();

        if (m_vecSections[focusedControl-CONTROL_START_BUTTONS]->m_strCategory == "masterlock")
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
          { // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iSection, 0);
            return false;
          }
        }
        m_iSection = focusedControl - CONTROL_START_BUTTONS;
        CheckNetworkSettings();

        CreateSettings();
      }
      return true;
    }
  case GUI_MSG_LOAD_SKIN:
    {
      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        g_guiSettings.SetString("locale.language", m_strNewLanguage);
        g_settings.Save();

        CStdString strLangInfoPath;
        strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", m_strNewLanguage.c_str());
        g_langInfo.Load(strLangInfoPath);

        if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode())
        {
          CLog::Log(LOGINFO, "Language needs a ttf font, loading first ttf font available");
          CStdString strFontSet;
          if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
          {
            m_strNewSkinFontSet=strFontSet;
          }
          else
            CLog::Log(LOGERROR, "No ttf font found but needed: %s", strFontSet.c_str());
        }

        g_charsetConverter.reset();

        CStdString strLanguagePath;
        strLanguagePath.Format("special://xbmc/language/%s/strings.xml", m_strNewLanguage.c_str());
        g_localizeStrings.Load(strLanguagePath);

        // also tell our weather to reload, as this must be localized
        g_weatherManager.Refresh();
      }

      // Do we need to reload the skin font set
      if (!m_strNewSkinFontSet.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.font", m_strNewSkinFontSet);
        g_settings.Save();
      }

      // Reload another skin
      if (!m_strNewSkin.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skin", m_strNewSkin);
        g_settings.Save();
      }

      // Reload a skin theme
      if (!m_strNewSkinTheme.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skintheme", m_strNewSkinTheme);
        // also set the default color theme
        CStdString colorTheme(m_strNewSkinTheme);
        CUtil::ReplaceExtension(colorTheme, ".xml", colorTheme);
        if (colorTheme.Equals("Textures.xml"))
          colorTheme = "defaults.xml";
        g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);
        g_settings.Save();
      }

      // Reload a skin color
      if (!m_strNewSkinColors.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skincolors", m_strNewSkinColors);
        g_settings.Save();
      }

      if (IsActive())
        m_returningFromSkinLoad = true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_delayedSetting = NULL;
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_returningFromSkinLoad = false;
      m_iScreen = (int)message.GetParam2() - (int)CGUIWindow::GetID();
      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_UPDATE_ITEM:
    if (m_delayedSetting)
    {
      OnSettingChanged(m_delayedSetting);
      m_delayedSetting = NULL;
      return true;
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
      {
        // Cancel delayed setting - it's only used for res changing anyway
        m_delayedSetting = NULL;
        if (IsActive() && g_guiSettings.GetResolution() != g_graphicsContext.GetVideoResolution())
        {
          g_guiSettings.SetResolution(g_graphicsContext.GetVideoResolution());
          CreateSettings();
        }
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_delayedSetting = NULL;
      // Hardware based stuff
      // TODO: This should be done in a completely separate screen
      // to give warning to the user that it writes to the EEPROM.
      if ((g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL))
      {
        g_audioConfig.SetAC3Enabled(g_guiSettings.GetBool("audiooutput.ac3passthrough"));
        g_audioConfig.SetDTSEnabled(g_guiSettings.GetBool("audiooutput.dtspassthrough"));
        if (g_audioConfig.NeedsSave())
        { // should we perhaps show a dialog here?
          g_audioConfig.Save();
        }
      }

      CheckForUpdates();
      CheckNetworkSettings();
      CGUIWindow::OnMessage(message);
      FreeControls();
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsCategory::SetupControls()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalCategoryButton || !m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton)
    return ;
  m_pOriginalEdit = (CGUIEditControl *)GetControl(CONTROL_DEFAULT_EDIT);
  if (!m_pOriginalEdit || m_pOriginalEdit->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
  }
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group)
    return;
  // get a list of different sections
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
  if (!pSettingsGroup) return ;
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
  // get the categories we need
  pSettingsGroup->GetCategories(m_vecSections);
  // run through and create our buttons...
  int j=0;
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    if (m_vecSections[i]->m_labelID == 12360 && g_settings.m_iLastLoadedProfileIndex != 0)
      continue;
    CGUIButtonControl *pButton = NULL;
    if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
      pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
    else
      pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_labelID));
    pButton->SetID(CONTROL_START_BUTTONS + j);
    pButton->SetVisible(true);
    pButton->AllocResources();
    group->AddControl(pButton);
    j++;
  }
  if (m_iSection < 0 || m_iSection >= (int)m_vecSections.size())
    m_iSection = 0;
  CreateSettings();
  // set focus correctly
  m_defaultControl = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (!group)
    return;
  vecSettings settings;
  g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    AddSetting(pSetting, group->GetWidth(), iControlID);
    CStdString strSetting = pSetting->GetSetting();
    if (strSetting.Equals("musicplayer.visualisation"))
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting.Equals("musiclibrary.scraper"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("musiclibrary.scraper"), "music");
    }
    else if (strSetting.Equals("scrapers.moviedefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.moviedefault"), "movies");
    }
    else if (strSetting.Equals("scrapers.tvshowdefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.tvshowdefault"), "tvshows");
    }
    else if (strSetting.Equals("scrapers.musicvideodefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.musicvideodefault"), "musicvideos");
    }
    else if (strSetting.Equals("audiooutput.mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videooutput.aspect"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21375), VIDEO_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(21376), VIDEO_LETTERBOX);
      pControl->AddLabel(g_localizeStrings.Get(21377), VIDEO_WIDESCREEN);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiocds.encoder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiocds.quality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("services.webserverusername"))
    {
#ifdef HAS_WEB_SERVER
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetUserName());
        g_settings.Save();
      }
#endif
    }
    else if (strSetting.Equals("services.webserverpassword"))
    {
#ifdef HAS_WEB_SERVER
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
#endif
    }
    else if (strSetting.Equals("services.webserverport"))
    {
#ifdef HAS_WEB_SERVER
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
#endif
    }
    else if (strSetting.Equals("services.esport"))
    {
#ifdef HAS_EVENT_SERVER
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
#endif
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(716), NETWORK_DHCP);
      pControl->AddLabel(g_localizeStrings.Get(717), NETWORK_STATIC);
      pControl->AddLabel(g_localizeStrings.Get(787), NETWORK_DISABLED);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("network.httpproxyport"))
    {
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
    }
    else if (strSetting.Equals("subtitles.style"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), FONT_STYLE_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), FONT_STYLE_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), FONT_STYLE_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), FONT_STYLE_BOLD_ITALICS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.color"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("karaoke.fontcolors"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = KARAOKE_COLOR_START; i <= KARAOKE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(22040 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.height") || strSetting.Equals("karaoke.fontheight") )
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting.Equals("subtitles.font") || strSetting.Equals("karaoke.font") )
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting.Equals("subtitles.charset") || strSetting.Equals("locale.charset") || strSetting.Equals("karaoke.charset"))
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.font"))
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skin"))
    {
      FillInSkins(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.soundskin"))
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting.Equals("locale.language"))
    {
      FillInLanguages(pSetting);
    }
#ifdef _LINUX
    else if (strSetting.Equals("locale.timezonecountry"))
    {
      CStdString myTimezoneCountry = g_guiSettings.GetString("locale.timezonecountry");
      int myTimezeoneCountryIndex = 0;

      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      vector<CStdString> countries = g_timezone.GetCounties();
      for (unsigned int i=0; i < countries.size(); i++)
      {
        if (countries[i] == myTimezoneCountry)
           myTimezeoneCountryIndex = i;
        pControl->AddLabel(countries[i], i);
      }
      pControl->SetValue(myTimezeoneCountryIndex);
    }
    else if (strSetting.Equals("locale.timezone"))
    {
      CStdString myTimezoneCountry = g_guiSettings.GetString("locale.timezonecountry");
      CStdString myTimezone = g_guiSettings.GetString("locale.timezone");
      int myTimezoneIndex = 0;

      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->Clear();
      vector<CStdString> timezones = g_timezone.GetTimezonesByCountry(myTimezoneCountry);
      for (unsigned int i=0; i < timezones.size(); i++)
      {
        if (timezones[i] == myTimezone)
           myTimezoneIndex = i;
        pControl->AddLabel(timezones[i], i);
      }
      pControl->SetValue(myTimezoneIndex);
    }
#endif
    else if (strSetting.Equals("videoscreen.screenmode"))
    {
      FillInResolutions(pSetting, false);
    }
    else if (strSetting.Equals("videoscreen.vsync"))
    {
      FillInVSyncs(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skintheme"))
    {
      FillInSkinThemes(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skincolors"))
    {
      FillInSkinColors(pSetting);
    }
    else if (strSetting.Equals("screensaver.mode"))
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting.Equals("videoplayer.displayresolution") || strSetting.Equals("pictures.displayresolution"))
    {
      FillInResolutions(pSetting, true);
    }
    else if (strSetting.Equals("videoplayer.highqualityupscaling"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13113), SOFTWARE_UPSCALING_DISABLED);
      pControl->AddLabel(g_localizeStrings.Get(13114), SOFTWARE_UPSCALING_SD_CONTENT);
      pControl->AddLabel(g_localizeStrings.Get(13115), SOFTWARE_UPSCALING_ALWAYS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.upscalingalgorithm"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13117), VS_SCALINGMETHOD_BICUBIC_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13118), VS_SCALINGMETHOD_LANCZOS_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13119), VS_SCALINGMETHOD_SINC_SOFTWARE);
      pControl->AddLabel(g_localizeStrings.Get(13120), VS_SCALINGMETHOD_VDPAU_HARDWARE);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videolibrary.flattentvshows"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(20420), 0); // Never
      pControl->AddLabel(g_localizeStrings.Get(20421), 1); // One Season
      pControl->AddLabel(g_localizeStrings.Get(20422), 2); // Always
      pControl->SetValue(pSettingInt->GetData());
    }
#ifdef __APPLE__
    else if (strSetting.Equals("input.appleremotemode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13610), APPLE_REMOTE_DISABLED);
      pControl->AddLabel(g_localizeStrings.Get(13611), APPLE_REMOTE_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(13612), APPLE_REMOTE_UNIVERSAL);
      pControl->AddLabel(g_localizeStrings.Get(13613), APPLE_REMOTE_MULTIREMOTE);
      pControl->SetValue(pSettingInt->GetData());
    }
#endif
    else if (strSetting.Equals("powermanagement.shutdownstate"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      if (!g_application.IsStandAlone())
      {
        pControl->AddLabel(g_localizeStrings.Get(13009), POWERSTATE_QUIT);
        pControl->AddLabel(g_localizeStrings.Get(13014), POWERSTATE_MINIMIZE);
      }

      if (g_powerManager.CanPowerdown())
        pControl->AddLabel(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN);

      if (g_powerManager.CanHibernate())
        pControl->AddLabel(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE);

      if (g_powerManager.CanSuspend())
        pControl->AddLabel(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND);

      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.rendermethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
#ifdef HAS_XBOX_D3D
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(21397), RENDER_HQ_RGB_SHADERV2);
#else
      pControl->AddLabel(g_localizeStrings.Get(13416), RENDER_METHOD_AUTO);
      pControl->AddLabel(g_localizeStrings.Get(13417), RENDER_METHOD_ARB);
      pControl->AddLabel(g_localizeStrings.Get(13418), RENDER_METHOD_GLSL);
      pControl->AddLabel(g_localizeStrings.Get(13419), RENDER_METHOD_SOFTWARE);
#ifdef HAVE_LIBVDPAU
      pControl->AddLabel(g_localizeStrings.Get(13421), RENDER_METHOD_VDPAU);
#endif
#endif
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.replaygaintype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("network.enc"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(780), ENC_NONE);
      pControl->AddLabel(g_localizeStrings.Get(781), ENC_WEP);
      pControl->AddLabel(g_localizeStrings.Get(782), ENC_WPA);
      pControl->AddLabel(g_localizeStrings.Get(783), ENC_WPA2);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lookandfeel.startupwindow"))
    {
      FillInStartupWindow(pSetting);
    }
    else if (strSetting.Equals("locale.country"))
    {
      FillInRegions(pSetting);
    }
    else if (strSetting.Equals("network.interface"))
    {
       FillInNetworkInterfaces(pSetting);
    }
    else if (strSetting.Equals("audiooutput.audiodevice"))
    {
      FillInAudioDevices(pSetting);
    }
    else if (strSetting.Equals("audiooutput.passthroughdevice"))
    {
      FillInAudioDevices(pSetting,true);
    }
    else if (strSetting.Equals("videoplayer.resumeautomatically"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), RESUME_NO);
      pControl->AddLabel(g_localizeStrings.Get(107), RESUME_YES);
      pControl->AddLabel(g_localizeStrings.Get(12020), RESUME_ASK);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.synctype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13501), SYNC_DISCON);
      pControl->AddLabel(g_localizeStrings.Get(13502), SYNC_SKIPDUP);
      pControl->AddLabel(g_localizeStrings.Get(13503), SYNC_RESAMPLE);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.resamplequality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13506), RESAMPLE_LOW);
      pControl->AddLabel(g_localizeStrings.Get(13507), RESAMPLE_MID);
      pControl->AddLabel(g_localizeStrings.Get(13508), RESAMPLE_HIGH);
      pControl->AddLabel(g_localizeStrings.Get(13509), RESAMPLE_REALLYHIGH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("weather.plugin"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInWeatherPlugins(pControl, g_guiSettings.GetString("weather.plugin"));
    }
  }

  if (m_vecSections[m_iSection]->m_strCategory == "network")
  {
     NetworkInterfaceChanged();
  }

  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];  
    pSettingControl->Update();
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("videoplayer.upscalingalgorithm"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        int value = g_guiSettings.GetInt("videoplayer.highqualityupscaling");

        if (value == SOFTWARE_UPSCALING_DISABLED)
          pControl->SetEnabled(false);
        else
          pControl->SetEnabled(true);
      }
    }
#ifdef HAVE_LIBVDPAU
    else if (strSetting.Equals("videoplayer.vdpauUpscalingLevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        pControl->SetEnabled(true);
      }
    }
#endif
#if defined(__APPLE__) || defined(_WIN32)
    else if (strSetting.Equals("videoscreen.blankdisplays"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        int value = g_guiSettings.GetResolution();
        if (g_settings.m_ResInfo[value].bFullScreen)
          pControl->SetEnabled(true);
        else
          pControl->SetEnabled(false);
      }
    }
#endif
#ifdef __APPLE__
    else if (strSetting.Equals("input.appleremotemode"))
    {
      bool cancelled;
      int remoteMode = g_guiSettings.GetInt("input.appleremotemode");

      // if it's not disabled, start the event server or else apple remote won't work
      if ( remoteMode != APPLE_REMOTE_DISABLED )
      {
        g_guiSettings.SetBool("services.esenabled", true);
        g_application.StartEventServer();
      }

      // if XBMC helper is running, prompt user before effecting change
      if ( g_xbmcHelper.IsRunning() && g_xbmcHelper.GetMode()!=remoteMode )
      {
        if (!CGUIDialogYesNo::ShowAndGetInput(13144, 13145, 13146, 13147, -1, -1, cancelled, 10000))
        {
          // user declined, restore previous spinner state and appleremote mode
          CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
          g_guiSettings.SetInt("input.appleremotemode", g_xbmcHelper.GetMode());
          pControl->SetValue(g_xbmcHelper.GetMode());
        }
        else
        {
          // reload configuration
          g_xbmcHelper.Configure();
        }
      }
      else
      {
        // set new configuration.
        g_xbmcHelper.Configure();
      }

      if (g_xbmcHelper.ErrorStarting() == true)
      {
        // inform user about error
        CGUIDialogOK::ShowAndGetInput(13620, 13621, 20022, 20022);

        // reset spinner to disabled state
        CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
        pControl->SetValue(APPLE_REMOTE_DISABLED);
      }
    }
    else if (strSetting.Equals("input.appleremotealwayson"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("input.appleremotemode");
         if (value != APPLE_REMOTE_DISABLED)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
     else if (strSetting.Equals("input.appleremotesequencetime"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("input.appleremotemode");
         if (value == APPLE_REMOTE_UNIVERSAL)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
#endif
    else if (strSetting.Equals("filelists.allowfiledeletion"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("filelists.showaddsourcebuttons"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("masterlock.startuplock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE);
    }
    else if (!strSetting.Equals("services.esenabled")
             && strSetting.Left(11).Equals("services.es"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("services.esenabled"));
    }
    else if (strSetting.Equals("audiocds.quality"))
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV);
    }
    else if (strSetting.Equals("audiocds.bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV) &&
                                           (g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_CBR));
    }
    else if (strSetting.Equals("audiooutput.ac3passthrough") || strSetting.Equals("audiooutput.dtspassthrough") || strSetting.Equals("audiooutput.passthroughdevice"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL);
    }
    else if (strSetting.Equals("musicplayer.crossfadealbumtracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("musicplayer.crossfade") > 0);
    }
    else if (strSetting.Equals("services.webserverusername"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
    }
    else if (strSetting.Equals("services.webserverpassword"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
    }
    else if (strSetting.Equals("services.webserverport"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
    }
    else if (strSetting.Equals("network.ipaddress") || strSetting.Equals("network.subnet") || strSetting.Equals("network.gateway") || strSetting.Equals("network.dns"))
    {
#ifdef _LINUX
      bool enabled = (geteuid() == 0);
#else
      bool enabled = false;
#endif
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1)
         enabled = (pControl1->GetValue() == NETWORK_STATIC);

       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
#ifdef HAS_LINUX_NETWORK
      if (pControl1)
         pControl1->SetEnabled(geteuid() == 0);
#endif
    }
    else if (strSetting.Equals("network.essid") || strSetting.Equals("network.enc") || strSetting.Equals("network.key"))
    {
      // Get network information
      CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
      CStdString ifaceName = ifaceControl->GetLabel();
      CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
      bool bIsWireless = iface->IsWireless();

#ifdef HAS_LINUX_NETWORK
      bool enabled = bIsWireless && (geteuid() == 0);
#else
      bool enabled = bIsWireless;
#endif
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1)
         enabled &= (pControl1->GetValue() != NETWORK_DISABLED);

      if (strSetting.Equals("network.key"))
      {
         pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
         if (pControl1) enabled &= (pControl1->GetValue() != ENC_NONE);
      }

       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("network.httpproxyserver")   || strSetting.Equals("network.httpproxyport") ||
             strSetting.Equals("network.httpproxyusername") || strSetting.Equals("network.httpproxypassword"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
    }
#ifdef HAS_LINUX_NETWORK
    else if (strSetting.Equals("network.key"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
      if (pControl && pControl1)
         pControl->SetEnabled(!pControl1->IsDisabled() && pControl1->GetValue() > 0);
    }
    else if (strSetting.Equals("network.save"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(geteuid() == 0);
    }
#endif
    else if (strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpassword"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.lastfmsubmit") | g_guiSettings.GetBool("scrobbler.lastfmsubmitradio"));
    }
    else if (strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpassword"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.librefmsubmit"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.verticaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.horizontaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.horizontaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocking") || strSetting.Equals("postprocessing.horizontaldeblocking") || strSetting.Equals("postprocessing.autobrightnesscontrastlevels") || strSetting.Equals("postprocessing.dering"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.auto"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable"));
    }
    else if (strSetting.Equals("subtitles.color") || strSetting.Equals("subtitles.style") || strSetting.Equals("subtitles.charset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("locale.charset"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("screensaver.dimlevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "Dim");
    }
    else if (strSetting.Equals("screensaver.slideshowpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow");
    }
    else if (strSetting.Equals("screensaver.slideshowshuffle"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow" ||
                           g_guiSettings.GetString("screensaver.mode") == "Fanart Slideshow");
    }
    else if (strSetting.Equals("screensaver.preview")           ||
             strSetting.Equals("screensaver.usedimonpause")     ||
             strSetting.Equals("screensaver.usemusicvisinstead"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") != "None");
      if (strSetting.Equals("screensaver.usedimonpause") && g_guiSettings.GetString("screensaver.mode").Equals("Dim"))
        pControl->SetEnabled(false);
    }
    else if (strSetting.Left(16).Equals("weather.areacode"))
    {
      CSettingString *pSetting = (CSettingString *)GetSetting(strSetting)->GetSetting();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetLabel2(CWeather::GetAreaCity(pSetting->GetData()));
    }
    else if (strSetting.Equals("weather.plugin"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      if (pControl->GetCurrentLabel().Equals(g_localizeStrings.Get(13611)))
        g_guiSettings.SetString("weather.plugin", "");
      else
        g_guiSettings.SetString("weather.plugin", pControl->GetCurrentLabel());
    }
    else if (strSetting.Equals("musicfiles.trackformat"))
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("musicfiles.trackformat"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
      }
    }
    else if (strSetting.Equals("musicfiles.trackformatright"))
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("musicfiles.trackformatright"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
      }
    }
#ifdef HAS_TIME_SERVER
    else if (strSetting.Equals("locale.timeserveraddress"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("locale.timeserver"));
    }
#endif
    else if (strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("debug.screenshotpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
    else if (strSetting.Equals("lookandfeel.rssedit"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(XFILE::CFile::Exists(RSSEDITOR_PATH) && g_guiSettings.GetBool("lookandfeel.enablerssfeeds"));
    }
    else if (strSetting.Equals("musiclibrary.scrapersettings"))
    {
      CScraperParser parser;
      bool enabled=false;
      if (parser.Load("special://xbmc/system/scrapers/music/"+g_guiSettings.GetString("musiclibrary.scraper")))
        enabled = parser.HasFunction("GetSettings");

      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("videoplayer.synctype"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.usedisplayasclock"));
    }
    else if (strSetting.Equals("videoplayer.maxspeedadjust"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) && 
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }
    }      
    else if (strSetting.Equals("videoplayer.resamplequality"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) && 
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }
    }
    else if (strSetting.Equals("weather.pluginsettings"))
    {
      // Create our base path
      CStdString basepath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetString("weather.plugin").IsEmpty() && CScriptSettings::SettingsExist(basepath));
    }
#if defined(_LINUX) && !defined(__APPLE__)
    else if (strSetting.Equals("audiooutput.custompassthrough"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL)
      {
        if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.passthroughdevice").Equals("custom"));
      }
      else
      {
        if (pControl) pControl->SetEnabled(false);
      }
    }
    else if (strSetting.Equals("audiooutput.customdevice"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.audiodevice").Equals("custom"));
    }
#endif
  }
}

void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  // date and time used to be here
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Left(16).Equals("weather.areacode"))
  {
    CStdString strSearch;
    if (CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(14024), false))
    {
      strSearch.Replace(" ", "+");
      CStdString strResult = ((CSettingString *)pSettingControl->GetSetting())->GetData();
      if (g_weatherManager.GetSearchResults(strSearch, strResult))
        ((CSettingString *)pSettingControl->GetSetting())->SetData(strResult);
      g_weatherManager.Refresh();
    }
  }
  else if (strSetting.Equals("weather.plugin"))
  {
    g_weatherManager.Refresh();
  }
  else if (strSetting.Equals("weather.pluginsettings"))
  {
    // Create our base path
    CStdString basepath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
    CGUIDialogPluginSettings::ShowAndGetInput(basepath);
    // TODO: maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
    g_weatherManager.Refresh();
  }
  else if (strSetting.Equals("lookandfeel.rssedit"))
    CBuiltins::Execute("RunScript("RSSEDITOR_PATH")");
  else if (strSetting.Equals("musiclibrary.scrapersettings"))
  {
    CMusicDatabase database;
    database.Open();
    SScraperInfo info;
    database.GetScraperForPath("musicdb://",info);

    if (info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + info.strPath))
        CGUIDialogPluginSettings::ShowAndGetInput(info);

    database.SetScraperForPath("musicdb://",info);
    database.Close();
  }

  // if OnClick() returns false, the setting hasn't changed or doesn't
  // require immediate update
  if (!pSettingControl->OnClick())
  {
    UpdateSettings();
    if (!pSettingControl->IsDelayed())
      return;
  }

  if (pSettingControl->IsDelayed())
  { // delayed setting
    m_delayedSetting = pSettingControl;
    m_delayedTimer.StartZero();
  }
  else
    OnSettingChanged(pSettingControl);
}

void CGUIWindowSettingsCategory::CheckForUpdates()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    if (pSettingControl->NeedsUpdate())
    {
      OnSettingChanged(pSettingControl);
      pSettingControl->Reset();
    }
  }
}

void CGUIWindowSettingsCategory::OnSettingChanged(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();

  // ok, now check the various special things we need to do
  if (strSetting.Equals("musicplayer.visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData( CVisualisation::GetCombinedName( pControl->GetCurrentLabel() ) );
  }
  else if (strSetting.Equals("debug.showloginfo"))
  {
    if (g_guiSettings.GetBool("debug.showloginfo"))
    {
      int level = std::max(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
      g_advancedSettings.m_logLevel = level;
      CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting. Level %d.", level);
    }
    else
    {
      int level = std::min(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
      CLog::Log(LOGNOTICE, "Disabled debug logging due to GUI setting. Level %d.", level);
      g_advancedSettings.m_logLevel = level;
    }
  }
  /*else if (strSetting.Equals("musicfiles.repeat"))
  {
    g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  }*/
  else if (strSetting.Equals("musiclibrary.cleanup"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteMusicDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("musiclibrary.scraper"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "music");
  }
  else if (strSetting.Equals("scrapers.moviedefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "movies");
  }
  else if (strSetting.Equals("scrapers.tvshowdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "tvshows");
  }
  else if (strSetting.Equals("scrapers.musicvideodefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "musicvideos");
  }
  else if (strSetting.Equals("videolibrary.cleanup"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.CleanDatabase();
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("videolibrary.export"))
    CBuiltins::Execute("exportlibrary(video)");  
  else if (strSetting.Equals("musiclibrary.export"))
    CBuiltins::Execute("exportlibrary(music)");  
  else if (strSetting.Equals("karaoke.export") )
  {
    vector<CStdString> choices;
    choices.push_back(g_localizeStrings.Get(22034));
    choices.push_back(g_localizeStrings.Get(22035));

    int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if ( retVal > 0 )
    {
      CStdString path(g_settings.GetDatabaseFolder());
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares);
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), path, true))
      {
        CMusicDatabase musicdatabase;
        musicdatabase.Open();

        if ( retVal == 1 )
        {
          CUtil::AddFileToFolder(path, "karaoke.html", path);
          musicdatabase.ExportKaraokeInfo( path, true );
        }
        else
        {
          CUtil::AddFileToFolder(path, "karaoke.csv", path);
          musicdatabase.ExportKaraokeInfo( path, false );
        }
        musicdatabase.Close();
      }
    }
  }
  else if (strSetting.Equals("videolibrary.import"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "videodb.xml", g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("musiclibrary.import"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("karaoke.importcsv"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "karaoke.csv", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportKaraokeInfo(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("scrobbler.lastfmsubmit") || strSetting.Equals("scrobbler.lastfmsubmitradio") || strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpassword"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.lastfmpassword");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.lastfmusername");
    if ((g_guiSettings.GetBool("scrobbler.lastfmsubmit") || 
         g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLastfmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLastfmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("scrobbler.librefmsubmit") || strSetting.Equals("scrobbler.librefmsubmitradio") || strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpassword"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.librefmpassword");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.librefmusername");
    if ((g_guiSettings.GetBool("scrobbler.librefmsubmit") || 
         g_guiSettings.GetBool("scrobbler.librefmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLibrefmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLibrefmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("audiooutput.audiodevice"))
  {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
#if defined(_LINUX) && !defined(__APPLE__)
      g_guiSettings.SetString("audiooutput.audiodevice", m_AnalogAudioSinkMap[pControl->GetCurrentLabel()]);
#else
      g_guiSettings.SetString("audiooutput.audiodevice", pControl->GetCurrentLabel());
#endif
  }
#if defined(_LINUX)
  else if (strSetting.Equals("audiooutput.passthroughdevice"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
#if defined(_LINUX) && !defined(__APPLE__)
      g_guiSettings.SetString("audiooutput.passthroughdevice", m_DigitalAudioSinkMap[pControl->GetCurrentLabel()]);
#else
      g_guiSettings.SetString("audiooutput.passthroughdevice", pControl->GetCurrentLabel());
#endif
  }
#endif
#ifdef HAS_LCD
  else if (strSetting.Equals("videoscreen.haslcd"))
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
#endif
  else if ( strSetting.Equals("services.webserver") || strSetting.Equals("services.webserverport") || 
            strSetting.Equals("services.webserverusername") || strSetting.Equals("services.webserverpassword"))
  {
    if (strSetting.Equals("services.webserverport"))
      ValidatePortNumber(pSettingControl, "8080", "80");
#ifdef HAS_WEB_SERVER
    g_application.StopWebServer(true);
    if (g_guiSettings.GetBool("services.webserver"))
    {
      g_application.StartWebServer();
      if (g_application.m_pWebServer) {
        if (strSetting.Equals("services.webserverusername"))
          g_application.m_pWebServer->SetUserName(g_guiSettings.GetString("services.webserverusername").c_str());
        else
          g_application.m_pWebServer->SetPassword(g_guiSettings.GetString("services.webserverpassword").c_str());
      }
    }
#endif
  } 
  else if (strSetting.Equals("services.zeroconf"))
  {
#ifdef HAS_ZEROCONF
    //ifdef zeroconf here because it's only found in guisettings if defined
    CZeroconf::GetInstance()->Stop();
    if(g_guiSettings.GetBool("services.zeroconf"))
      CZeroconf::GetInstance()->Start();
#endif
  }
  else if (strSetting.Equals("network.ipaddress"))
  {
    if (g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC)
    {
      CStdString strDefault = g_guiSettings.GetString("network.ipaddress").Left(g_guiSettings.GetString("network.ipaddress").ReverseFind('.'))+".1";
      if (g_guiSettings.GetString("network.gateway").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.gateway",strDefault);
      if (g_guiSettings.GetString("network.dns").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.dns",strDefault);

    }
  }

  else if (strSetting.Equals("network.httpproxyport"))
  {
    ValidatePortNumber(pSettingControl, "8080", "8080", false);
  }
  else if (strSetting.Equals("videoplayer.calibrate") || strSetting.Equals("videoscreen.guicalibration"))
  { // activate the video calibration screen
    g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (strSetting.Equals("videoscreen.testpattern"))
  { // activate the test pattern
    g_windowManager.ActivateWindow(WINDOW_TEST_PATTERN);
  }
  else if (strSetting.Equals("subtitles.height"))
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting.Equals("subtitles.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("subtitles.height"));
  }
  else if (strSetting.Equals("subtitles.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("karaoke.fontheight"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
  }
  else if (strSetting.Equals("karaoke.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("karaoke.fontheight"));
  }
  else if (strSetting.Equals("karaoke.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
      newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("locale.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("lookandfeel.font"))
  { // new font choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = pControl->GetCurrentLabel();
    if (strSkinFontSet != ".svn" && strSkinFontSet != g_guiSettings.GetString("lookandfeel.font"))
    {
      m_strNewSkinFontSet = strSkinFontSet;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewSkinFontSet.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skin"))
  { // new skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = g_settings.GetSkinFolder(strSkin);
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().textColor);
      if (strSkin != ".svn" && strSkin != g_guiSettings.GetString("lookandfeel.skin"))
      {
        m_strNewSkin = strSkin;
        g_application.DelayLoadSkin();
      }
      else
      { // Do not reload the skin we are already using
        m_strNewSkin.Empty();
        g_application.CancelDelayLoadSkin();
      }
    }
    else
    {
      m_strErrorMessage.Format("Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().disabledColor);
    }
  }
  else if (strSetting.Equals("lookandfeel.soundskin"))
  { // new sound skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue()==0)
      g_guiSettings.SetString("lookandfeel.soundskin", "OFF");
    else if (pControl->GetValue()==1)
      g_guiSettings.SetString("lookandfeel.soundskin", "SKINDEFAULT");
    else
      g_guiSettings.SetString("lookandfeel.soundskin", pControl->GetCurrentLabel());

    g_audioManager.Enable(true);
    g_audioManager.Load();
  }
  else if (strSetting.Equals("input.enablemouse"))
  {
    g_Mouse.SetEnabled(g_guiSettings.GetBool("input.enablemouse"));
  }
  else if (strSetting.Equals("videoscreen.screenmode"))
  { // new resolution choosen... - update if necessary
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    RESOLUTION nextRes = (RESOLUTION)msg.GetParam1();
    RESOLUTION lastRes = g_graphicsContext.GetVideoResolution();
    g_guiSettings.SetResolution(nextRes);
    g_graphicsContext.SetVideoResolution(nextRes);
    bool cancelled = false;
    if (!CGUIDialogYesNo::ShowAndGetInput(13110, 13111, 20022, 20022, -1, -1, cancelled, 10000))
    {
      g_guiSettings.SetResolution(lastRes);
      g_graphicsContext.SetVideoResolution(lastRes);
    }
  }
  else if (strSetting.Equals("videoscreen.vsync"))
  {
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
// DXMERGE: This may be useful
//    g_videoConfig.SetVSyncMode((VSYNC)msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.fakefullscreen"))
  {
    if (g_graphicsContext.IsFullScreenRoot())
      g_graphicsContext.SetVideoResolution(g_graphicsContext.GetVideoResolution(), true);
  }
  else if (strSetting.Equals("locale.language"))
  { // new language chosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strLanguage = pControl->GetCurrentLabel();
    if (strLanguage != ".svn" && strLanguage != pSettingString->GetData())
    {
      m_strNewLanguage = strLanguage;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewLanguage.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skintheme"))
  { //a new Theme was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinTheme;

    if (pControl->GetValue() == 0) // Use default theme
      strSkinTheme = "SKINDEFAULT";
    else
      strSkinTheme = pControl->GetCurrentLabel() + ".xpr";

    if (strSkinTheme != pSettingString->GetData())
    {
      m_strNewSkinTheme = strSkinTheme;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin theme we are using
      m_strNewSkinTheme.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skincolors"))
  { //a new color was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinColor;

    if (pControl->GetValue() == 0) // Use default colors
      strSkinColor = "SKINDEFAULT";
    else
      strSkinColor = pControl->GetCurrentLabel() + ".xml";

    if (strSkinColor != pSettingString->GetData())
    {
      m_strNewSkinColors = strSkinColor;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin colors we are using
      m_strNewSkinColors.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("videoplayer.displayresolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.flickerfilter") || strSetting.Equals("videoscreen.soften"))
  { // reset display
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);
  }
  else if (strSetting.Equals("screensaver.mode"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strScreenSaver;
    if (iValue == 0)
      strScreenSaver = "None";
    else if (iValue == 1)
      strScreenSaver = "Dim";
    else if (iValue == 2)
      strScreenSaver = "Black";
    else if (iValue == 3)
      strScreenSaver = "SlideShow"; // PictureSlideShow
    else if (iValue == 4)
      strScreenSaver = "Fanart Slideshow"; //Fanart Slideshow
    else
      strScreenSaver = pControl->GetCurrentLabel() + ".xbs";
    pSettingString->SetData(strScreenSaver);
  }
  else if (strSetting.Equals("screensaver.preview"))
  {
    g_application.ActivateScreenSaver(true);
  }
  else if (strSetting.Equals("screensaver.slideshowpath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares = g_settings.m_pictureSources;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("debug.screenshotpath") || strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("subtitles.custompath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = g_guiSettings.GetString(strSetting,false);
    VECSOURCES shares;

    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetLocalDrives(shares);

    UpdateSettings();
    bool bWriteOnly = true;

    if (strSetting.Equals("subtitles.custompath"))
    {
      bWriteOnly = false;
      shares = g_settings.m_videoSources;
      g_mediaManager.GetLocalDrives(shares);
    }
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path, bWriteOnly))
    {
      pSettingString->SetData(path);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("locale.country"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("locale.country", strRegion);
    g_weatherManager.Refresh(); // need to reset our weather, as temperatures need re-translating.
  }
#ifdef HAS_TIME_SERVER
  else if (strSetting.Equals("locale.timeserver") || strSetting.Equals("locale.timeserveraddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("locale.timeserver"))
      g_application.StartTimeServer();
  }
#endif
  else if (strSetting.Equals("smb.winsserver") || strSetting.Equals("smb.workgroup") )
  {
    if (g_guiSettings.GetString("smb.winsserver") == "0.0.0.0")
      g_guiSettings.SetString("smb.winsserver", "");

    /* okey we really don't need to restarat, only deinit samba, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogOK *dlg = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!dlg) return ;
    dlg->SetHeading( g_localizeStrings.Get(14038) );
    dlg->SetLine( 0, g_localizeStrings.Get(14039) );
    dlg->SetLine( 1, g_localizeStrings.Get(14040));
    dlg->SetLine( 2, "");
    dlg->DoModal();

    if (dlg->IsConfirmed())
    {
      g_settings.Save();
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  else if (strSetting.Equals("services.upnpserver"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnpserver"))
      g_application.StartUPnPServer();
    else
      g_application.StopUPnPServer();
#endif
  }
  else if (strSetting.Equals("services.upnprenderer"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnprenderer"))
      g_application.StartUPnPRenderer();
    else
      g_application.StopUPnPRenderer();
#endif
  }
  else if (strSetting.Equals("services.esenabled"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
      g_application.StartEventServer();
    else
    {
      if (!g_application.StopEventServer(true, true))
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("services.esport"))
  {
#ifdef HAS_EVENT_SERVER
    ValidatePortNumber(pSettingControl, "9777", "9777");
    //restart eventserver without asking user
    if (g_application.StopEventServer(true, false))
      g_application.StartEventServer();
#ifdef __APPLE__
    //reconfigure XBMCHelper for port changes
    g_xbmcHelper.Configure();
#endif
#endif
  }
  else if (strSetting.Equals("services.esallinterfaces"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (g_application.StopEventServer(true, true))
        g_application.StartEventServer();
      else
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("services.esinitialdelay") ||
           strSetting.Equals("services.escontinuousdelay"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      g_application.RefreshEventServer();
    }
#endif
  }
  else if (strSetting.Equals("masterlock.lockcode"))
  {
    // Now Prompt User to enter the old and then the new MasterCode!
    if(g_passwordManager.SetMasterLockMode())
    {
      // We asked for the master password and saved the new one!
      // Nothing todo here
    }
  }
  else if (strSetting.Equals("network.interface"))
  {
     NetworkInterfaceChanged();
  }
#ifdef HAS_LINUX_NETWORK
  else if (strSetting.Equals("network.save"))
  {
     NetworkAssignment iAssignment;
     CStdString sIPAddress;
     CStdString sNetworkMask;
     CStdString sDefaultGateway;
     CStdString sWirelessNetwork;
     CStdString sWirelessKey;
     CStdString sDns;
     EncMode iWirelessEnc;
     CStdString ifaceName;

     CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
     ifaceName = ifaceControl->GetLabel();
     CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);

     // Update controls with information
     CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
     if (pControl1) iAssignment = (NetworkAssignment) pControl1->GetValue();
     CGUIButtonControl* pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.ipaddress")->GetID());
     if (pControl2) sIPAddress = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.subnet")->GetID());
     if (pControl2) sNetworkMask = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.gateway")->GetID());
     if (pControl2) sDefaultGateway = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.dns")->GetID());
     if (pControl2) sDns = pControl2->GetLabel2();
     pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
     if (pControl1) iWirelessEnc = (EncMode) pControl1->GetValue();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());
     if (pControl2) sWirelessNetwork = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.key")->GetID());
     if (pControl2) sWirelessKey = pControl2->GetLabel2();

     CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
     pDlgProgress->SetLine(0, "");
     pDlgProgress->SetLine(1, g_localizeStrings.Get(784));
     pDlgProgress->SetLine(2, "");
     pDlgProgress->StartModal();
     pDlgProgress->Progress();

     std::vector<CStdString> nameServers;
     nameServers.push_back(sDns);
     g_application.getNetwork().SetNameServers(nameServers);
     iface->SetSettings(iAssignment, sIPAddress, sNetworkMask, sDefaultGateway, sWirelessNetwork, sWirelessKey, iWirelessEnc);

     pDlgProgress->Close();

     if (iAssignment == NETWORK_DISABLED)
        CGUIDialogOK::ShowAndGetInput(0, 788, 0, 0);
     else if (iface->IsConnected())
        CGUIDialogOK::ShowAndGetInput(0, 785, 0, 0);
     else
        CGUIDialogOK::ShowAndGetInput(0, 786, 0, 0);
  }
  else if (strSetting.Equals("network.essid"))
  {
    CGUIDialogAccessPoints *dialog = (CGUIDialogAccessPoints *)g_windowManager.GetWindow(WINDOW_DIALOG_ACCESS_POINTS);
    if (dialog)
    {
       CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
       dialog->SetInterfaceName(pControl->GetLabel());
       dialog->DoModal();

       if (dialog->WasItemSelected())
       {
          CGUIButtonControl* pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());
          if (pControl2) pControl2->SetLabel2(dialog->GetSelectedAccessPointEssId());
          pControl = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
          if (pControl) pControl->SetValue(dialog->GetSelectedAccessPointEncMode());
       }
    }
  }
#endif
#ifdef _LINUX
  else if (strSetting.Equals("locale.timezonecountry"))
  {
    CGUISpinControlEx *pControlCountry = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString country = pControlCountry->GetCurrentLabel();

    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
    pControl->Clear();
    vector<CStdString> timezones = g_timezone.GetTimezonesByCountry(country);
    for (unsigned int i=0; i < timezones.size(); i++)
    {
      pControl->AddLabel(timezones[i], i);
    }

    g_timezone.SetTimezone(pControl->GetLabel());
    g_guiSettings.SetString("locale.timezonecountry",pControlCountry->GetLabel().c_str());

    CGUISpinControlEx *tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
    g_guiSettings.SetString("locale.timezone", tzControl->GetLabel().c_str());
  }
  else  if (strSetting.Equals("locale.timezone"))
  {
     CGUISpinControlEx *tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
     g_timezone.SetTimezone(tzControl->GetLabel());
     g_guiSettings.SetString("locale.timezone", tzControl->GetLabel().c_str());

     tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezonecountry")->GetID());
     g_guiSettings.SetString("locale.timezonecountry", tzControl->GetLabel().c_str());
  }
#endif
  else if (strSetting.Equals("lookandfeel.skinzoom"))
  {
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
  }
  else if (strSetting.Equals("videolibrary.flattentvshows") ||
           strSetting.Equals("videolibrary.removeduplicates"))
  {
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }

  UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_vecSections.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for(int i = 0; (size_t)i < m_vecSettings.size(); i++)
  {
    delete m_vecSettings[i];
  }
  m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  if (!pSetting->IsVisible()) return;  // not displayed in current session
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetWidth(width);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == EDIT_CONTROL_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_HIDDEN_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_NUMBER_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_IP_INPUT)
  {
    pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (!pControl) return ;
    ((CGUIEditControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIEditControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CEditSettingControl((CGUIEditControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
  }
  if (!pControl) return;
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
    m_vecSettings.push_back(pSettingControl);
  }
}

void CGUIWindowSettingsCategory::Render()
{
  // update realtime changeable stuff
  UpdateRealTimeSettings();

  if (m_delayedSetting && m_delayedTimer.GetElapsedMilliseconds() > 3000)
  { // we send a thread message so that it's processed the following frame (some settings won't
    // like being changed during Render())
    CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), GetID());
    g_windowManager.SendThreadMessage(message, GetID());
    m_delayedTimer.Stop();
  }

  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      bAlphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      bAlphaFaded = true;
    }
  }
  CGUIWindow::Render();
  if (bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
  // render the error message if necessary
  if (m_strErrorMessage.size())
  {
    CGUIFont *pFont = g_fontManager.GetFont("font13");
    float fPosY = g_graphicsContext.GetHeight() * 0.8f;
    float fPosX = g_graphicsContext.GetWidth() * 0.5f;
    CGUITextLayout::DrawText(pFont, fPosX, fPosY, 0xffffffff, 0, m_strErrorMessage, XBFONT_CENTER_X);
  }
}

void CGUIWindowSettingsCategory::CheckNetworkSettings()
{
  if (!g_application.IsStandAlone())
    return;

  // check if our network needs restarting (requires a reset, so check well!)
  if (m_iNetworkAssignment == -1)
  {
    // nothing to do here, folks - move along.
    return ;
  }
  // we need a reset if:
  // 1.  The Network Assignment has changed OR
  // 2.  The Network Assignment is STATIC and one of the network fields have changed
  if (m_iNetworkAssignment != g_guiSettings.GetInt("network.assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("network.ipaddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("network.subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("network.gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("network.dns"))))
  {
/*    // our network settings have changed - we should prompt the user to reset XBMC
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039, 14040, 0))
    {
      // reset settings
      g_application.getApplicationMessenger().RestartApp();
      // Todo: aquire new network settings without restart app!
    }
    else*/

    // update our settings variables
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");

    // replace settings
    /*   g_guiSettings.SetInt("network.assignment", m_iNetworkAssignment);
       g_guiSettings.SetString("network.ipaddress", m_strNetworkIPAddress);
       g_guiSettings.SetString("network.subnet", m_strNetworkSubnet);
       g_guiSettings.SetString("network.gateway", m_strNetworkGateway);
       g_guiSettings.SetString("network.dns", m_strNetworkDNS);*/
  }
}

void CGUIWindowSettingsCategory::FillInSubtitleHeights(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  if (CUtil::IsUsingTTFSubtitles())
  { // easy - just fill as per usual
    CStdString strLabel;
    for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
    {
      if (pSettingInt->m_iFormat > -1)
      {
        CStdString strFormat = g_localizeStrings.Get(pSettingInt->m_iFormat);
        strLabel.Format(strFormat, i);
      }
      else
        strLabel.Format(pSettingInt->m_strFormat, i);
      pControl->AddLabel(strLabel, i);
    }
    pControl->SetValue(pSettingInt->GetData());
  }
}

void CGUIWindowSettingsCategory::FillInSubtitleFonts(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentFont = 0;
  int iFont = 0;

  // find TTF fonts
  {
    CFileItemList items;
    if (CDirectory::GetDirectory("special://xbmc/media/Fonts/", items))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];

        if (!pItem->m_bIsFolder)
        {

          if ( !CUtil::GetExtension(pItem->GetLabel()).Equals(".ttf") ) continue;
          if (pItem->GetLabel().Equals(pSettingString->GetData(), false))
            iCurrentFont = iFont;

          pControl->AddLabel(pItem->GetLabel(), iFont++);
        }

      }
    }
  }
  pControl->SetValue(iCurrentFont);
}

void CGUIWindowSettingsCategory::FillInSkinFonts(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iSkinFontSet = 0;

  m_strNewSkinFontSet.Empty();

  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("Font.xml", &res);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return ;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return ;
  }

  const TiXmlNode *pChild = pRootElement->FirstChild();
  strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          pControl->AddLabel(idAttr, iSkinFontSet);
          if (strcmpi(idAttr, g_guiSettings.GetString("lookandfeel.font").c_str()) == 0)
            pControl->SetValue(iSkinFontSet);
          iSkinFontSet++;
        }
      }
      pChild = pChild->NextSibling();
    }

  }
  else
  {
    // Since no fontset is defined, there is no selection of a fontset, so disable the component
    pControl->AddLabel(g_localizeStrings.Get(13278), 1);
    pControl->SetValue(1);
    pControl->SetEnabled(false);
  }
}

void CGUIWindowSettingsCategory::FillInSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/skin/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/skin/", items);

  int iCurrentSkin = 0;
  int iSkin = 0;
  vector<CStdString> vecSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      //   if (g_SkinInfo.Check(pItem->m_strPath))
      //   {
      vecSkins.push_back(pItem->GetLabel());
      //   }
    }
  }

  sort(vecSkins.begin(), vecSkins.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecSkins.size(); ++i)
  {
    CStdString strSkin = vecSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.skin").c_str()) == 0)
    {
      iCurrentSkin = iSkin;
    }
    pControl->AddLabel(strSkin, iSkin++);
  }
  pControl->SetValue(iCurrentSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInSoundSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/sounds/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/sounds/", items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  int i;
  for (i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecSoundSkins.push_back(pItem->GetLabel());
    }
  }

  pControl->AddLabel(g_localizeStrings.Get(474), iSoundSkin++); // Off
  pControl->AddLabel(g_localizeStrings.Get(15109), iSoundSkin++); // Skin Default

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="SKINDEFAULT")
    iCurrentSoundSkin=1;

  sort(vecSoundSkins.begin(), vecSoundSkins.end(), sortstringbyname());
  for (i = 0; i < (int) vecSoundSkins.size(); ++i)
  {
    CStdString strSkin = vecSoundSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.soundskin").c_str()) == 0)
    {
      iCurrentSoundSkin = iSoundSkin;
    }
    pControl->AddLabel(strSkin, iSoundSkin++);
  }
  pControl->SetValue(iCurrentSoundSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInCharSets(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentCharset = 0;
  vector<CStdString> vecCharsets = g_charsetConverter.getCharsetLabels();

  CStdString strCurrentCharsetLabel="DEFAULT";
  if (pSettingString->GetData()!="DEFAULT")
    strCurrentCharsetLabel = g_charsetConverter.getCharsetLabelByName(pSettingString->GetData());

  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  vecCharsets.insert(vecCharsets.begin(), g_localizeStrings.Get(13278)); // "Default"

  bool bIsAuto=(pSettingString->GetData()=="DEFAULT");

  for (int i = 0; i < (int) vecCharsets.size(); ++i)
  {
    CStdString strCharsetLabel = vecCharsets[i];

    if (!bIsAuto && strCharsetLabel == strCurrentCharsetLabel)
      iCurrentCharset = i;

    pControl->AddLabel(strCharsetLabel, i);
  }

  pControl->SetValue(iCurrentCharset);
}

void CGUIWindowSettingsCategory::FillInVisualisations(CSetting *pSetting, int iControlID)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  if (!pSetting) return;
  int iWinID = g_windowManager.GetActiveWindow();
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, iWinID, iControlID);
    g_windowManager.SendMessage(msg);
  }
  vector<CStdString> vecVis;
  //find visz....
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/visualisations/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/visualisations/", items);

  CVisualisationFactory visFactory;
  CStdString strExtension;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      const char *visPath = (const char*)pItem->m_strPath;

      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".vis")  // normal visualisation
      {
        if(!CVisualisation::IsValidVisualisation(pItem->m_strPath))
          continue;
        CStdString strLabel = pItem->GetLabel();
        vecVis.push_back( CVisualisation::GetFriendlyName( strLabel ) );
      }
      else if ( strExtension == ".mvis" )  // multi visualisation with sub modules
      {
        CVisualisation* vis = visFactory.LoadVisualisation( visPath );
        if ( vis )
        {
          map<string, string> subModules;
          map<string, string>::iterator iter;
          string moduleName, path;
          CStdString visName = pItem->GetLabel();
          visName = visName.Mid(0, visName.size() - 5);

          // get list of sub modules from the visualisation
          vis->GetSubModules( subModules );

          for ( iter=subModules.begin() ; iter!=subModules.end() ; iter++ )
          {
            // each pair of the map is of the format 'module name' => 'module path'
            moduleName = iter->first;
            vecVis.push_back( CVisualisation::GetFriendlyName( visName.c_str(), moduleName.c_str() ).c_str() );
            CLog::Log(LOGDEBUG, "Module %s for visualisation %s", moduleName.c_str(), visPath);
          }
          delete vis;
        }
      }
    }
  }

  CStdString strDefaultVis = pSettingString->GetData();
  if (!strDefaultVis.Equals("None"))
    strDefaultVis = CVisualisation::GetFriendlyName( strDefaultVis );

  sort(vecVis.begin(), vecVis.end(), sortstringbyname());

  // add the "disabled" setting first
  int iVis = 0;
  int iCurrentVis = 0;
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
    msg.SetLabel(231);
    g_windowManager.SendMessage(msg);
  }
  for (int i = 0; i < (int) vecVis.size(); ++i)
  {
    CStdString strVis = vecVis[i];

    if (strcmpi(strVis.c_str(), strDefaultVis.c_str()) == 0)
      iCurrentVis = iVis;

    {
      CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
      msg.SetLabel(strVis);
      g_windowManager.SendMessage(msg);
    }
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, iWinID, iControlID, iCurrentVis);
    g_windowManager.SendMessage(msg);
  }
}

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting, bool playbackSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
  control->SetDelayed();
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(control->GetID());
  pControl->Clear();

  pControl->AddLabel(g_settings.m_ResInfo[RES_WINDOW].strMode, RES_WINDOW);
  pControl->AddLabel(g_settings.m_ResInfo[RES_DESKTOP].strMode, RES_DESKTOP);
  size_t maxRes = g_settings.m_ResInfo.size();
  if (g_Windowing.GetNumScreens())
    maxRes = std::min(maxRes, (size_t)RES_DESKTOP + g_Windowing.GetNumScreens());
  for (size_t i = RES_CUSTOM ; i < maxRes; i++)
  {
    pControl->AddLabel(g_settings.m_ResInfo[i].strMode, i);
  }
  pControl->SetValue(CGUISettings::GetResFromString(pSettingString->GetData()));
}

void CGUIWindowSettingsCategory::FillInVSyncs(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
#if !defined(__APPLE__) && !defined(_WIN32)
  pControl->AddLabel(g_localizeStrings.Get(13101) , VSYNC_DRIVER);
#endif
  pControl->AddLabel(g_localizeStrings.Get(13106) , VSYNC_DISABLED);
  pControl->AddLabel(g_localizeStrings.Get(13107) , VSYNC_VIDEO);
  pControl->AddLabel(g_localizeStrings.Get(13108) , VSYNC_ALWAYS);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInLanguages(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  m_strNewLanguage.Empty();
  //find languages...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/language/", items);

  int iCurrentLang = 0;
  int iLanguage = 0;
  vector<CStdString> vecLanguage;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecLanguage.size(); ++i)
  {
    CStdString strLanguage = vecLanguage[i];
    if (strcmpi(strLanguage.c_str(), pSettingString->GetData().c_str()) == 0)
      iCurrentLang = iLanguage;
    pControl->AddLabel(strLanguage, iLanguage++);
  }

  pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInScreenSavers(CSetting *pSetting)
{ // Screensaver mode
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(351), 0); // Off
  pControl->AddLabel(g_localizeStrings.Get(352), 1); // Dim
  pControl->AddLabel(g_localizeStrings.Get(353), 2); // Black
  pControl->AddLabel(g_localizeStrings.Get(108), 3); // PictureSlideShow
  pControl->AddLabel(g_localizeStrings.Get(20425), 4); // Fanart Slideshow

  //find screensavers ....
  CFileItemList items;
  CDirectory::GetDirectory( "special://xbmc/screensavers/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/screensavers/", items);

  int iCurrentScr = -1;
  vector<CStdString> vecScr;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xbs")
      {
#ifdef _LINUX
        void *handle = dlopen(_P(pItem->m_strPath).c_str(), RTLD_LAZY);
        if (!handle)
        {
          CLog::Log(LOGERROR, "FillInScreensavers: Unable to load %s, reason: %s", pItem->m_strPath.c_str(), dlerror());
          continue;
        }
        dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        vecScr.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultScr = pSettingString->GetData();
  CStdString strExtension;
  CUtil::GetExtension(strDefaultScr, strExtension);
  if (strExtension == ".xbs")
    strDefaultScr.Delete(strDefaultScr.size() - 4, 4);

  sort(vecScr.begin(), vecScr.end(), sortstringbyname());
  for (int i = 0; i < (int) vecScr.size(); ++i)
  {
    CStdString strScr = vecScr[i];

    if (strcmpi(strScr.c_str(), strDefaultScr.c_str()) == 0)
      iCurrentScr = i + PREDEFINED_SCREENSAVERS;

    pControl->AddLabel(strScr, i + PREDEFINED_SCREENSAVERS);
  }

  // if we can't find the screensaver previously configured
  // then fallback to turning the screensaver off.
  if (iCurrentScr < 0)
  {
    if (strDefaultScr == "Dim")
      iCurrentScr = 1;
    else if (strDefaultScr == "Black")
      iCurrentScr = 2;
    else if (strDefaultScr == "SlideShow") // PictureSlideShow
      iCurrentScr = 3;
    else if (strDefaultScr == "Fanart Slideshow") // Fanart slideshow
      iCurrentScr = 4;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}

void CGUIWindowSettingsCategory::FillInRegions(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iCurrentRegion=0;
  CStdStringArray regions;
  g_langInfo.GetRegionNames(regions);

  CStdString strCurrentRegion=g_langInfo.GetCurrentRegion();

  sort(regions.begin(), regions.end(), sortstringbyname());

  for (int i = 0; i < (int) regions.size(); ++i)
  {
    const CStdString& strRegion = regions[i];

    if (strRegion == strCurrentRegion)
      iCurrentRegion = i;

    pControl->AddLabel(strRegion, i);
  }

  pControl->SetValue(iCurrentRegion);
}

CBaseSettingControl *CGUIWindowSettingsCategory::GetSetting(const CStdString &strSetting)
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    if (m_vecSettings[i]->GetSetting()->GetSetting() == strSetting)
      return m_vecSettings[i];
  }
  return NULL;
}

void CGUIWindowSettingsCategory::JumpToSection(int windowId, const CStdString &section)
{
  // grab our section
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(windowId - WINDOW_SETTINGS_MYPICTURES);
  if (!pSettingsGroup) return;
  // get the categories we need
  vecSettingsCategory categories;
  pSettingsGroup->GetCategories(categories);
  // iterate through them and check for the required section
  int iSection = -1;
  for (unsigned int i = 0; i < categories.size(); i++)
    if (categories[i]->m_strCategory.Equals(section))
      iSection = i;
  if (iSection == -1) return;

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSectionBeforeJump=m_iSection;
  m_iControlBeforeJump=m_lastControlID;
  m_iWindowBeforeJump=GetID();

  m_iSection=iSection;
  m_lastControlID=CONTROL_START_CONTROL;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, windowId);
  OnMessage(msg1);
  for (unsigned int i=0; i<m_vecSections.size(); ++i)
    CONTROL_DISABLE(CONTROL_START_BUTTONS+i);
}

void CGUIWindowSettingsCategory::JumpToPreviousSection()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSection=m_iSectionBeforeJump;
  m_lastControlID=m_iControlBeforeJump;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowBeforeJump);
  OnMessage(msg1);

  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
}

void CGUIWindowSettingsCategory::FillInSkinThemes(CSetting *pSetting)
{
  // There is a default theme (just Textures.xpr)
  // any other *.xpr files are additional themes on top of this one.
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skintheme");

  m_strNewSkinTheme.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standart Textures.xpr will be used!

  // find all *.xpr in this path
  CStdString strDefaultTheme = pSettingString->GetData();

  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // Remove the .xpr extension from the Themes
  CStdString strExtension;
  CUtil::GetExtension(strSettingString, strExtension);
  if (strExtension == ".xpr") strSettingString.Delete(strSettingString.size() - 4, 4);
  // Sort the Themes for GUI and list them
  int iCurrentTheme = 0;
  for (int i = 0; i < (int) vecTheme.size(); ++i)
  {
    CStdString strTheme = vecTheme[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strTheme.CompareNoCase(strSettingString) == 0 )
      iCurrentTheme = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strTheme, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentTheme);
}

void CGUIWindowSettingsCategory::FillInSkinColors(CSetting *pSetting)
{
  // There is a default theme (just defaults.xml)
  // any other *.xml files are additional color themes on top of this one.
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skincolors");

  m_strNewSkinColors.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standard defaults.xml will be used!

  // Search for colors in the Current skin!
  vector<CStdString> vecColors;

  CStdString strPath;
  CUtil::AddFileToFolder(g_SkinInfo.GetBaseDir(),"colors",strPath);

  CFileItemList items;
  CDirectory::GetDirectory(PTH_IC(strPath), items, ".xml");
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder && pItem->GetLabel().CompareNoCase("defaults.xml") != 0)
    { // not the default one
      CStdString strLabel = pItem->GetLabel();
      vecColors.push_back(strLabel.Mid(0, strLabel.size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());

  // Remove the .xml extension from the Themes
  if (CUtil::GetExtension(strSettingString) == ".xml")
    CUtil::RemoveExtension(strSettingString);

  int iCurrentColor = 0;
  for (int i = 0; i < (int) vecColors.size(); ++i)
  {
    CStdString strColor = vecColors[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strColor.CompareNoCase(strSettingString) == 0 )
      iCurrentColor = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strColor, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentColor);
}

void CGUIWindowSettingsCategory::FillInStartupWindow(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  const vector<CSkinInfo::CStartupWindow> &startupWindows = g_SkinInfo.GetStartupWindows();

  // TODO: How should we localize this?
  // In the long run there is no way to do it really without the skin having some
  // translation information built in to it, which isn't really feasible.

  // Alternatively we could lookup the strings in the english strings file to get
  // their id and then get the string from that

  // easier would be to have the skinner use the "name" as the label number.

  // eg <window id="0">513</window>

  bool currentSettingFound(false);
  for (vector<CSkinInfo::CStartupWindow>::const_iterator it = startupWindows.begin(); it != startupWindows.end(); it++)
  {
    CStdString windowName((*it).m_name);
    if (StringUtils::IsNaturalNumber(windowName))
      windowName = g_localizeStrings.Get(atoi(windowName.c_str()));
    int windowID((*it).m_id);
    pControl->AddLabel(windowName, windowID);
    if (pSettingInt->GetData() == windowID)
      currentSettingFound = true;
  }

  // ok, now check whether our current option is one of these
  // and set it's value
  if (!currentSettingFound)
  { // nope - set it to the "default" option - the first one
    pSettingInt->SetData(startupWindows[0].m_id);
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::OnInitWindow()
{
  if (g_application.IsStandAlone())
  {
#ifndef __APPLE__
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");
#endif
  }
  m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::FillInViewModes(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->AddLabel("Auto", DEFAULT_VIEW_AUTO);
  bool found(false);
  int foundType = 0;
  CGUIWindow *window = g_windowManager.GetWindow(windowID);
  if (window)
  {
    window->Initialize();
    for (int i = 50; i < 60; i++)
    {
      CGUIBaseContainer *control = (CGUIBaseContainer *)window->GetControl(i);
      if (control)
      {
        int type = (control->GetType() << 16) | i;
        pControl->AddLabel(control->GetLabel(), type);
        if (type == pSettingInt->GetData())
          found = true;
        else if ((type >> 16) == (pSettingInt->GetData() >> 16))
          foundType = type;
      }
    }
    window->ClearAll();
  }
  if (!found)
    pSettingInt->SetData(foundType ? foundType : (DEFAULT_VIEW_AUTO));
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInSortMethods(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CFileItemList items("C:");
  CGUIViewState *state = CGUIViewState::GetViewState(windowID, items);
  if (state)
  {
    bool found(false);
    vector< pair<int,int> > sortMethods;
    state->GetSortMethods(sortMethods);
    for (unsigned int i = 0; i < sortMethods.size(); i++)
    {
      pControl->AddLabel(g_localizeStrings.Get(sortMethods[i].second), sortMethods[i].first);
      if (sortMethods[i].first == pSettingInt->GetData())
        found = true;
    }
    if (!found && sortMethods.size())
      pSettingInt->SetData(sortMethods[0].first);
  }
  pControl->SetValue(pSettingInt->GetData());
  delete state;
}

void CGUIWindowSettingsCategory::FillInScrapers(CGUISpinControlEx *pControl, const CStdString& strSelected, const CStdString& strContent)
{
  CFileItemList items;
  if (strContent.Equals("music"))
    CDirectory::GetDirectory("special://xbmc/system/scrapers/music",items,".xml",false);
  else
    CDirectory::GetDirectory("special://xbmc/system/scrapers/video",items,".xml",false);
  int j=0;
  int k=0;
  pControl->Clear();
  for ( int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;

    CScraperParser parser;
    if (parser.Load(items[i]->m_strPath))
    {
      if (parser.GetContent() != strContent && !strContent.Equals("music"))
        continue;

      if (parser.GetName().Equals(strSelected) || CUtil::GetFileName(items[i]->m_strPath).Equals(strSelected))
      {
        if (strContent.Equals("music")) // native strContent would be albums or artists but we're using the same scraper for both
        {
          if (g_guiSettings.GetString("musiclibrary.scraper") != strSelected)
          {
            g_guiSettings.SetString("musiclibrary.scraper", CUtil::GetFileName(items[i]->m_strPath));

            SScraperInfo info;
            CMusicDatabase database;

            info.strPath = g_guiSettings.GetString("musiclibrary.scraper");
            info.strContent = "albums";
            info.strTitle = parser.GetName();

            database.Open();
            database.SetScraperForPath("musicdb://",info);
            database.Close();
          }
        }
        else if (strContent.Equals("movies"))
          g_guiSettings.SetString("scrapers.moviedefault", CUtil::GetFileName(items[i]->m_strPath));
        else if (strContent.Equals("tvshows"))
          g_guiSettings.SetString("scrapers.tvshowdefault", CUtil::GetFileName(items[i]->m_strPath));
        else if (strContent.Equals("musicvideos"))
          g_guiSettings.SetString("scrapers.musicvideodefault", CUtil::GetFileName(items[i]->m_strPath));
        k = j;
      }
      pControl->AddLabel(parser.GetName(),j++);
    }
  }
  pControl->SetValue(k);
}

void CGUIWindowSettingsCategory::FillInNetworkInterfaces(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  // query list of interfaces
  vector<CStdString> vecInterfaces;
  std::vector<CNetworkInterface*>& ifaces = g_application.getNetwork().GetInterfaceList();
  std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
  while (iter != ifaces.end())
  {
    CNetworkInterface* iface = *iter;
    vecInterfaces.push_back(iface->GetName());
    ++iter;
  }
  sort(vecInterfaces.begin(), vecInterfaces.end(), sortstringbyname());

  int iInterface = 0;
  for (unsigned int i = 0; i < vecInterfaces.size(); ++i)
  {
    pControl->AddLabel(vecInterfaces[i], iInterface++);
  }
}

void CGUIWindowSettingsCategory::FillInAudioDevices(CSetting* pSetting, bool Passthrough)
{
#ifdef __APPLE__
  if (Passthrough)
    return;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  
  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);
  
  if (CCoreAudioHardware::GetDefaultOutputDevice())
    pControl->AddLabel("Default Output Device", 0); // This will cause FindAudioDevice to fall back to the system default as configured in 'System Preferences'
  int activeDevice = 0;
  
  CStdString deviceName;
  for (int i = pControl->GetMaximum(); !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    pControl->AddLabel(device.GetName(deviceName), i);
    
    if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(deviceName))
      activeDevice = i; // Tag this one
    
    deviceList.pop_front();
  }
  pControl->SetValue(activeDevice);
#elif defined(_LINUX)
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  CStdString currentDevice = Passthrough ? g_guiSettings.GetString("audiooutput.passthroughdevice") : g_guiSettings.GetString("audiooutput.audiodevice");

  if (Passthrough)
  {
    m_DigitalAudioSinkMap.clear();
    m_DigitalAudioSinkMap["Error - no devices found"] = "null:";
    m_DigitalAudioSinkMap["custom"] = "custom";
  }
  else
  {
    m_AnalogAudioSinkMap.clear();
    m_AnalogAudioSinkMap["Error - no devices found"] = "null:";
    m_AnalogAudioSinkMap["custom"] = "custom";
  }
  

  int numberSinks = 0;

  int selectedValue = -1;
  AudioSinkList sinkList;
  CAudioRendererFactory::EnumerateAudioSinks(sinkList, Passthrough);
  if (sinkList.size()==0)
  {
    pControl->AddLabel("Error - no devices found", 0);
    numberSinks = 1;
    selectedValue = 0;
  }
  else
  {
    AudioSinkList::const_iterator iter = sinkList.begin();
    for (int i=0; iter != sinkList.end(); iter++)
    {
      CStdString label = (*iter).first;
      CStdString sink  = (*iter).second;
      pControl->AddLabel(label.c_str(), i);

      if (currentDevice.Equals(sink))
        selectedValue = i;

      if (Passthrough)
        m_DigitalAudioSinkMap[label] = sink;
      else
        m_AnalogAudioSinkMap[label] = sink;

      i++;
    }

    numberSinks = sinkList.size();
  }

  if (currentDevice.Equals("custom"))
    selectedValue = numberSinks;

  pControl->AddLabel("custom", numberSinks++);

  if (selectedValue < 0)
  {
    CLog::Log(LOGWARNING, "Failed to find previously selected audio sink");
    pControl->AddLabel(currentDevice, numberSinks);
    pControl->SetValue(numberSinks);
  }
  else
    pControl->SetValue(selectedValue);
#elif defined(_WIN32)
  if (Passthrough)
    return;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  CWDSound p_dsound;
  std::vector<DSDeviceInfo > deviceList = p_dsound.GetSoundDevices();
  std::vector<DSDeviceInfo >::const_iterator iter = deviceList.begin();
  for (int i=0; iter != deviceList.end(); i++)
  {
    DSDeviceInfo dev = *iter;
    pControl->AddLabel(dev.strDescription, i);

    if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(dev.strDescription))
        pControl->SetValue(i);

    ++iter;
  }
#endif
}

void CGUIWindowSettingsCategory::FillInWeatherPlugins(CGUISpinControlEx *pControl, const CStdString& strSelected)
{
  int j=0;
  int k=0;
  pControl->Clear();
  // add our disable option
  pControl->AddLabel(g_localizeStrings.Get(13611), j++);

  CFileItemList items;
  if (CDirectory::GetDirectory("special://home/plugins/weather/", items, "/", false))
  {
    for (int i=0; i<items.Size(); ++i)
    {    
      // create the full path to the plugin
      CStdString plugin;
      CStdString pluginPath = items[i]->m_strPath;
      // remove slash at end so we can use the plugins folder as plugin name
      CUtil::RemoveSlashAtEnd(pluginPath);
      // add default.py to our plugin path to create the full path
      CUtil::AddFileToFolder(pluginPath, "default.py", plugin);
      if (XFILE::CFile::Exists(plugin))
      {
        // is this the users choice
        if (CUtil::GetFileName(pluginPath).Equals(strSelected))
          k = j;
        // we want to use the plugins folder as name
        pControl->AddLabel(CUtil::GetFileName(pluginPath), j++);
      }
    }
  }
  pControl->SetValue(k);
}

void CGUIWindowSettingsCategory::NetworkInterfaceChanged(void)
{
  return;

   NetworkAssignment iAssignment;
   CStdString sIPAddress;
   CStdString sNetworkMask;
   CStdString sDefaultGateway;
   CStdString sWirelessNetwork;
   CStdString sWirelessKey;
   EncMode iWirelessEnc;
   bool bIsWireless;
   CStdString ifaceName;

   // Get network information
   CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
   ifaceName = ifaceControl->GetLabel();
   CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
   iface->GetSettings(iAssignment, sIPAddress, sNetworkMask, sDefaultGateway, sWirelessNetwork, sWirelessKey, iWirelessEnc);
   bIsWireless = iface->IsWireless();

   CStdString dns;
   std::vector<CStdString> dnss = g_application.getNetwork().GetNameServers();
   if (dnss.size() >= 1)
      dns = dnss[0];

   // Update controls with information
   CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
   if (pControl1) pControl1->SetValue(iAssignment);
   GetSetting("network.dns")->GetSetting()->FromString(dns);
   if (iAssignment == NETWORK_STATIC || iAssignment == NETWORK_DISABLED)
   {
     GetSetting("network.ipaddress")->GetSetting()->FromString(sIPAddress);
     GetSetting("network.subnet")->GetSetting()->FromString(sNetworkMask);
     GetSetting("network.gateway")->GetSetting()->FromString(sDefaultGateway);
   }
   else
   {
     GetSetting("network.ipaddress")->GetSetting()->FromString(iface->GetCurrentIPAddress());
     GetSetting("network.subnet")->GetSetting()->FromString(iface->GetCurrentNetmask());
     GetSetting("network.gateway")->GetSetting()->FromString(iface->GetCurrentDefaultGateway());
   }

   pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
   if (pControl1) pControl1->SetValue(iWirelessEnc);

   if (bIsWireless)
   {
      GetSetting("network.essid")->GetSetting()->FromString(sWirelessNetwork);
      GetSetting("network.key")->GetSetting()->FromString(sWirelessKey);
   }
   else
   {
      GetSetting("network.essid")->GetSetting()->FromString("");
      GetSetting("network.key")->GetSetting()->FromString("");
   }
}

void CGUIWindowSettingsCategory::ValidatePortNumber(CBaseSettingControl* pSettingControl, const CStdString& userPort, const CStdString& privPort, bool listening/*=true*/)
{
  CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
  // check that it's a valid port
  int port = atoi(pSetting->GetData().c_str());
#ifdef _LINUX
  if (listening && geteuid() != 0 && (port < 1024 || port > 65535))
  {
    CGUIDialogOK::ShowAndGetInput(257, 850, 852, -1);
    pSetting->SetData(userPort.c_str());
  }
  else
#endif
  if (port <= 0 || port > 65535)
  {
    CGUIDialogOK::ShowAndGetInput(257, 850, 851, -1);
    pSetting->SetData(privPort.c_str());
  }
}