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

#include "stdafx.h"
#include "SPCCodec.h"
#include "cores/DllLoader/DllLoader.h"
#include "../DllLoader/SoLoader.h"
#include "MusicInfoTagLoaderSPC.h"
#include "MusicInfoTag.h"
#include "FileSystem/File.h"
#include "DynamicDll.h"

using namespace MUSIC_INFO;
using namespace XFILE;

SPCCodec::SPCCodec()
{
  m_CodecName = "SPC";
  m_szBuffer = NULL;
  m_pApuRAM = NULL;
  m_iDataPos = 0; 
  m_loader = NULL;
  m_dll.EmuAPU = NULL;
  m_dll.LoadSPCFile = NULL;
  m_dll.SeekAPU = NULL;
#ifdef _LINUX
  m_dll.ResetAPU = NULL;
  m_dll.InitAPU = NULL;
#endif
}

SPCCodec::~SPCCodec()
{
  DeInit();
}

bool SPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_loader = new SoLoader(DLL_PATH_SPC_CODEC);
  if (!m_loader)
    return false;
  if (!m_loader->Load())
  {
    delete m_loader;
    m_loader = NULL;
    return false;
  }
  
  m_loader->ResolveExport("LoadSPCFile",(void**)&m_dll.LoadSPCFile);
  m_loader->ResolveExport("EmuAPU",(void**)&m_dll.EmuAPU);
  m_loader->ResolveExport("SeekAPU",(void**)&m_dll.SeekAPU);
#ifdef _LINUX
  m_loader->ResolveExport("InitAPU",(void**)&m_dll.InitAPU);
  m_loader->ResolveExport("ResetAPU",(void**)&m_dll.ResetAPU);
#endif

  CFile file;
  if (!file.Open(strFile))
  {
    CLog::Log(LOGERROR,"SPCCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_szBuffer = new char[0x10200];
  if (!file.Read(m_szBuffer,0x10200))
  {
    delete[] m_szBuffer;
    m_szBuffer = NULL;
    file.Close();
    CLog::Log(LOGERROR,"SPCCodec: error reading file %s!",strFile.c_str());
    return false;
  }
  file.Close();

  m_pApuRAM = new u8[65536];
#ifdef _LINUX
  m_dll.InitAPU();
  m_dll.ResetAPU();
#endif

  m_dll.LoadSPCFile(m_szBuffer);
 
  m_SampleRate = 32000;  
  m_Channels = 2;
  m_BitsPerSample = 16;
  CMusicInfoTagLoaderSPC tagLoader;
  CMusicInfoTag tag;
  tagLoader.Load(strFile,tag);
  if (tag.Loaded())
    m_TotalTime = tag.GetDuration()*1000;
  else
    m_TotalTime = 4*60*1000; // default
  m_iDataPos = 0;
 
  return true;
}

void SPCCodec::DeInit()
{ 
  if (m_loader)
    delete m_loader;
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
  
  if (m_pApuRAM)
    delete[] m_pApuRAM;
  m_pApuRAM = NULL;
}

__int64 SPCCodec::Seek(__int64 iSeekTime)
{
  if (m_iDataPos > iSeekTime/1000*m_SampleRate*4)
  {
    m_dll.LoadSPCFile(m_szBuffer);
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
  }
  else
  {
    __int64 iDataPos2 = m_iDataPos;
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
    iSeekTime -= (iDataPos2*1000)/(m_SampleRate*4);
  }
  
  m_dll.SeekAPU((u32)iSeekTime*64,0);
  return (m_iDataPos*1000)/(m_SampleRate*4);
}

int SPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*4)
    return READ_EOF;
  
  *actualsize = (int)((BYTE*)m_dll.EmuAPU(pBuffer,0,size/4)-pBuffer);
  m_iDataPos += *actualsize;

  if (*actualsize)
    return READ_SUCCESS;    
  else
    return READ_ERROR;
}

bool SPCCodec::CanInit()
{
  return true;
}

