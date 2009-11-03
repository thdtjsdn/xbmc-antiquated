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

#pragma once

#ifdef HAS_DX
#include "StdString.h"

class ID3DResource
{
public:
  virtual void OnDestroyDevice()=0;
  virtual void OnCreateDevice()=0;
  virtual void OnLostDevice() {};
  virtual void OnResetDevice() {};
};

class CD3DTexture : public ID3DResource
{
public:
  CD3DTexture();
  ~CD3DTexture();

  bool Create(UINT width, UINT height, UINT mipLevels, DWORD usage, D3DFORMAT format, D3DPOOL pool);
  void Release();
  bool LockRect(UINT level, D3DLOCKED_RECT *lr, const RECT *rect, DWORD flags);
  void UnlockRect(UINT level);
  bool GetLevelDesc(UINT level, D3DSURFACE_DESC *desc);
  bool GetSurfaceLevel(UINT level, LPDIRECT3DSURFACE9 *surface);

  LPDIRECT3DTEXTURE9 Get() const { return m_texture; };

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
private:
  unsigned int GetMemoryUsage(unsigned int pitch) const;
  // creation parameters
  UINT      m_width;
  UINT      m_height;
  UINT      m_mipLevels;
  DWORD     m_usage;
  D3DFORMAT m_format;
  D3DPOOL   m_pool;

  // created texture
  LPDIRECT3DTEXTURE9 m_texture;

  // saved data
  BYTE*     m_data;
};

class CD3DEffect : public ID3DResource
{
public:
  CD3DEffect();
  virtual ~CD3DEffect();
  bool Create(const CStdString &effectString);
  void Release();
  bool SetTechnique(D3DXHANDLE handle);
  bool SetTexture(D3DXHANDLE handle, CD3DTexture &texture);
  bool Begin(UINT *passes, DWORD flags);
  bool BeginPass(UINT pass);
  void EndPass();
  void End();

  ID3DXEffect *Get() const { return m_effect; };

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
  virtual void OnLostDevice();
  virtual void OnResetDevice();
private:
  bool CreateEffect();
  CStdString   m_effectString;
  ID3DXEffect *m_effect;
};

class CD3DVertexBuffer : public ID3DResource
{
public:
  CD3DVertexBuffer();
  virtual ~CD3DVertexBuffer();
  bool Create(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool);
  void Release();
  bool Lock(UINT offset, UINT size, void **data, DWORD flags);
  void Unlock();

  LPDIRECT3DVERTEXBUFFER9 Get() const { return m_vertex; };

  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
private:
  bool CreateVertexBuffer();
  UINT    m_length;
  DWORD   m_usage;
  DWORD   m_fvf;
  D3DPOOL m_pool;
  LPDIRECT3DVERTEXBUFFER9 m_vertex;

  // saved data
  BYTE*   m_data;
};
#endif
