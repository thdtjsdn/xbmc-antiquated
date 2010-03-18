/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iostream>
#include "IFile.h"
#include "StdString.h"
#include "../utils/BitstreamStats.h"

class CURL;

namespace XFILE
{
class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) = 0;
  virtual ~IFileCallback() {};
};

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01

/* use buffered io during reading, ( hint to make all protocols buffered, some might be internal anyway ) */
#define READ_BUFFERED  0x02

/* use cache to access this file */
#define READ_CACHED     0x04

/* open without caching. regardless to file type. */
#define READ_NO_CACHE  0x08

class CFileStreamBuffer;
class ICacheInterface;

class CFile
{
public:
  CFile();
  virtual ~CFile();

  bool Open(const CStdString& strFileName, unsigned int flags = 0);
  bool OpenForWrite(const CStdString& strFileName, bool bOverWrite = false);
  unsigned int Read(void* lpBuf, int64_t uiBufSize);
  bool ReadString(char *szLine, int iLineLength);
  int Write(const void* lpBuf, int64_t uiBufSize);
  void Flush();
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t GetPosition();
  int64_t GetLength();
  void Close();
  int GetChunkSize() {if (m_pFile) return m_pFile->GetChunkSize(); return 0;}
  bool SkipNext(){if (m_pFile) return m_pFile->SkipNext(); return false;}
  BitstreamStats GetBitstreamStats() { return m_bitStreamStats; }

  ICacheInterface* GetCache() {if (m_pFile) return m_pFile->GetCache(); return NULL;}
  int IoControl(int request, void* param) { if (m_pFile) return m_pFile->IoControl(request, param); return -1; }

  IFile *GetImplemenation() { return m_pFile; }
  IFile *Detach();
  void   Attach(IFile *pFile, unsigned int flags = 0);

  static bool Exists(const CStdString& strFileName);
  static int  Stat(const CStdString& strFileName, struct __stat64* buffer);
  int Stat(struct __stat64 *buffer);
  static bool Delete(const CStdString& strFileName);
  static bool Rename(const CStdString& strFileName, const CStdString& strNewFileName);
  static bool Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const CStdString& fileName, bool hidden);

private:
  unsigned int m_flags;
  IFile* m_pFile;
  CFileStreamBuffer* m_pBuffer;
  BitstreamStats m_bitStreamStats;
};

// streambuf for file io, only supports buffered input currently
class CFileStreamBuffer
  : public std::streambuf
{
public:
  ~CFileStreamBuffer();
  CFileStreamBuffer(int backsize = 0);

  void Attach(IFile *file);
  void Detach();

private:
  virtual int_type underflow();
  virtual std::streamsize showmanyc();
  virtual pos_type seekoff(off_type, std::ios_base::seekdir,std::ios_base::openmode = std::ios_base::in | std::ios_base::out);
  virtual pos_type seekpos(pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);

  IFile* m_file;
  char*  m_buffer;
  int    m_backsize;
  int    m_frontsize;
};

// very basic file input stream
class CFileStream
  : public std::istream
{
public:
  CFileStream(int backsize = 0);
  ~CFileStream();

  bool Open(const CStdString& filename);
  bool Open(const CURL& filename);
  void Close();

  int64_t GetLength();
#ifdef _ARMEL
  char*   ReadFile();
#endif
private:
  CFileStreamBuffer m_buffer;
  IFile*            m_file;
};

}
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)