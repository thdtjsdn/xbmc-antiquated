#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "Addon.h"
#include "GUIWindow.h"
#include "GUIViewControl.h"

class CFileItem;
class CFileItemList;

class CGUIWindowAddonBrowser :
      public CGUIWindow
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  bool IsConfirmed() { return m_confirmed; };

  static bool ManageAddons(const ADDON::TYPE &type,
                           const CONTENT_TYPE &content = CONTENT_NONE,
                           const bool viewActive = true);

  void SetType(const ADDON::TYPE &type);
  void SetContent(const CONTENT_TYPE &content);

protected:
  virtual void OnInitWindow();
  int GetSelectedItem();
  void OnClick(int iItem);
  void OnSort();
  void ClearListItems();
  void Update();
  bool OnContextMenu(int iItem);
  void OnGetAddons(const ADDON::TYPE &type);
  
  ADDON::TYPE m_type;
  CONTENT_TYPE m_content;
  CFileItemList* m_vecItems;

  bool m_confirmed;
  bool m_changed;
  inline void SetActiveOnly(bool activeOnly) { m_getAddons = !activeOnly; };
  bool m_getAddons;
};
