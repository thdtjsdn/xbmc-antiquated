/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
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

#include "GUIWindowTestPattern.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "Key.h"

CGUIWindowTestPattern::CGUIWindowTestPattern(void)
    : CGUIWindow(WINDOW_TEST_PATTERN, "")
{
}

CGUIWindowTestPattern::~CGUIWindowTestPattern(void)
{}


bool CGUIWindowTestPattern::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PREVIOUS_MENU:
    {
      g_windowManager.PreviousWindow();
      return true;
    }
    break;

  case ACTION_MOVE_UP:
  case ACTION_MOVE_LEFT:
     m_pattern = m_pattern > 0 ? m_pattern - 1 : TEST_PATTERNS_COUNT - 1;
     break;

  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_RIGHT:
     m_pattern = (m_pattern + 1) % TEST_PATTERNS_COUNT;
     break;

  }
  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

bool CGUIWindowTestPattern::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    m_pattern = 0;
    m_bounceDirectionX = 1;
    m_bounceDirectionY = 1;
    m_bounceX = 0;
    m_bounceY = 0;
    m_blinkFrame = 0;
    break;

  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowTestPattern::Render()
{
  BeginRender();

  int top = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.top;
  int bottom = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.bottom;
  int left = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.left;
  int right = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.right;

  switch (m_pattern)
  {
    case 0:
      DrawContrastBrightnessPattern(top, left, bottom, right);
      break;

    case 1:
      DrawVerticalLines(top, left, bottom, right);
      break;

    case 2:
      DrawHorizontalLines(top, left, bottom, right);
      break;

    case 3:
      DrawCheckers(top, left, bottom, right);
      break;

    case 4:
      DrawBouncingRectangle(top, left, bottom, right);
      break;
  }

  EndRender();

  CGUIWindow::Render();
}

