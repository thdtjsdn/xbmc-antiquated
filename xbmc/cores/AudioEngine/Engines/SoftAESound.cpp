/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined USE_EXTERNAL_FFMPEG)
  #include <libavutil/avutil.h>
#else
  #include "cores/dvdplayer/Codecs/ffmpeg/libavutil/avutil.h"
#endif

#include "AESound.h"

#include <samplerate.h>
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "FileSystem/FileFactory.h"
#include "FileSystem/IFile.h"

#include "AEFactory.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEUtil.h"

#include "SoftAE.h"
#include "SoftAESound.h"

/* typecast the global AE to CSoftAE */
#define AE (*((CSoftAE*)&AE))

typedef struct
{
  char     chunk_id[4];
  uint32_t chunksize;
} WAVE_CHUNK;

CSoftAESound::CSoftAESound(const CStdString &filename) :
  IAESound(filename),
  m_refcount    (1    ),
  m_volume      (1.0f )
{
  m_filename = filename;
}

CSoftAESound::~CSoftAESound()
{
  DeInitialize();
}

void CSoftAESound::DeInitialize()
{
  CSingleLock lock(m_critSection);
  m_wavLoader.DeInitialize();
}

bool CSoftAESound::Initialize()
{
  DeInitialize();
  CSingleLock lock(m_critSection);

  if (!m_wavLoader.Initialize(m_filename, AE.GetSampleRate()))
    return false;

  m_wavLoader.Remap(AE.GetChannelLayout());
  return true;
}

unsigned int CSoftAESound::GetSampleCount()
{
  if (!m_wavLoader.IsValid())
    return 0;
  return m_wavLoader.GetSampleCount();
}

float* CSoftAESound::GetSamples()
{
  if (!m_wavLoader.IsValid())
    return NULL;
  return m_wavLoader.GetSamples();
}

bool CSoftAESound::IsPlaying()
{
  if (!m_wavLoader.IsValid())
    return false;
  return AE.IsPlaying(this);
}

void CSoftAESound::Play()
{
  if (!m_wavLoader.IsValid())
    return;
  AE.PlaySound(this);
}

void CSoftAESound::Stop()
{
  if (!m_wavLoader.IsValid())
    return;
  AE.StopSound(this);
}

