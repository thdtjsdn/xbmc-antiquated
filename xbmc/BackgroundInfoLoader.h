#pragma once

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

#include "utils/Thread.h"
#include "IProgressCallback.h"
#include "utils/CriticalSection.h"

#include <vector>

class CFileItem;
class CFileItemList;

class IBackgroundLoaderObserver
{
public:
  virtual ~IBackgroundLoaderObserver() {}
  virtual void OnItemLoaded(CFileItem* pItem) = 0;
};

class CBackgroundInfoLoader : public IRunnable
{
public:
  CBackgroundInfoLoader(int nThreads=-1);
  virtual ~CBackgroundInfoLoader();

  void Load(CFileItemList& items);
  bool IsLoading();
  virtual void Run();
  void SetObserver(IBackgroundLoaderObserver* pObserver);
  void SetProgressCallback(IProgressCallback* pCallback);
  virtual bool LoadItem(CFileItem* pItem) { return false; };

  void StopThread(); // will actually stop all worker threads.

  void SetNumOfWorkers(int nThreads); // -1 means auto compute num of required threads

protected:
  virtual void OnLoaderStart() {};
  virtual void OnLoaderFinish() {};

  CFileItemList *m_pVecItems;
  std::vector<CFileItem *> m_vecItems; // FileItemList would delete the items and we only want to keep a reference.
  CCriticalSection m_lock;

  bool m_bStartCalled;
  bool m_bRunning;
  bool m_bStop;
  int  m_nRequestedThreads;
  int  m_nActiveThreads;

  IBackgroundLoaderObserver* m_pObserver;
  IProgressCallback* m_pProgressCallback;

  std::vector<CThread *> m_workers;
};

