/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "Application.h"
#include "Addon.h"
#include "Settings.h"
#include "settings/AddonSettings.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"
#include "Util.h"

using namespace std;
using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * CAddon - AddOn Info and Helper Class
 *
 */

CAddon::CAddon()
{
  Reset();
}

void CAddon::Reset()
{
  m_guid        = "";
  m_guid_parent = "";
  m_addonType   = ADDON_UNKNOWN;
  m_strPath     = "";
  m_disabled    = false;
  m_stars       = -1;
  m_strVersion  = "";
  m_strName     = "";
  m_summary     = "";
  m_strDesc     = "";
  m_disclaimer  = "";
  m_strLibName  = "";
  m_childs      = 0;
}

bool CAddon::operator==(const CAddon &rhs) const
{
  return (m_guid == rhs.m_guid);
}

void CAddon::LoadAddonStrings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon;
  if (url.GetProtocol() == "plugin")
  {
    pathToAddon = "special://home/plugins/";
    CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
    CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);
  }
  else
    pathToAddon = url.Get();

  // Path where the language strings reside
  CStdString pathToLanguageFile = pathToAddon;
  CStdString pathToFallbackLanguageFile = pathToAddon;
  CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load language strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);
}

void CAddon::ClearAddonStrings()
{
  // Unload temporary language strings
  g_localizeStringsTemp.Clear();
}

bool CAddon::CreateChildAddon(const CAddon &parent, CAddon &child)
{
  if (parent.m_addonType != ADDON_PVRDLL)
  {
    CLog::Log(LOGERROR, "Can't create a child add-on for '%s' and type '%i', is not allowed for this type!", parent.m_strName.c_str(), parent.m_addonType);
    return false;
  }

  child = parent;
  child.m_guid_parent = parent.m_guid;
  child.m_guid = CUtil::CreateUUID();

  VECADDONS *addons = g_addonmanager.GetAddonsFromType(parent.m_addonType);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).m_guid == parent.m_guid)
    {
      (*it).m_childs++;
      child.m_strName.Format("%s #%i", child.m_strName.c_str(), (*it).m_childs);
      break;
    }
  }

  return true;
}

} /* namespace ADDON */
