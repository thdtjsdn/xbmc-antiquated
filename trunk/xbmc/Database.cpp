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

#include "Database.h"
#include "Util.h"
#include "Settings.h"
#include "Crc32.h"
#include "FileSystem/SpecialProtocol.h"
#include "AutoPtrHandle.h"
#include "utils/log.h"

using namespace AUTOPTR;
using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

CDatabase::CDatabase(void)
{
  m_bOpen = false;
  m_iRefCount = 0;
  m_preV2version = 0.0f;
  m_version = 0;
}

CDatabase::~CDatabase(void)
{
  Close();
}

void CDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

uint32_t CDatabase::ComputeCRC(const CStdString &text)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(text);
  return crc;
}

CStdString CDatabase::FormatSQL(CStdString strStmt, ...)
{
  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  strStmt.Replace("%s", "%q");
  //  the %I64 enhancement is not supported by sqlite3_vmprintf
  //  must be %ll instead
  strStmt.Replace("%I64", "%ll");

  va_list args;
  va_start(args, strStmt);
  char *szSql = sqlite3_vmprintf(strStmt.c_str(), args);
  va_end(args);

  CStdString strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  return strResult;
}

bool CDatabase::Open()
{
  if (IsOpen())
  {
    m_iRefCount++;
    return true;
  }

  CStdString strDatabase;
  CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), m_strDatabaseFile, strDatabase);

  m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase(_P(strDatabase).c_str());

  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  if ( m_pDB->connect() != DB_CONNECTION_OK)
  {
    CLog::Log(LOGERROR, "Unable to open %s (old version?)", strDatabase.c_str());
    return false;
  }
  
  // test if db already exists, if not we need to create the tables
  if (!m_pDB->exists())
  {
    CreateTables();
  }

  // Mark our db as open here to make our destructor to properly close the file handle
  m_bOpen = true;

  // Database exists, check the version number
  m_pDS->query("SELECT * FROM sqlite_master WHERE type = 'table' AND name = 'version'\n");
  int version = 0;
  if (m_pDS->num_rows() > 0)
  {
    m_pDS->close();
    m_pDS->query("SELECT idVersion FROM version\n");
    if (m_pDS->num_rows() > 0)
    {
//#ifdef PRE_2_1_DATABASE_COMPATIBILITY
      float fVersion = m_pDS->fv("idVersion").get_asFloat();
      if (fVersion < m_preV2version)
      { // old version - drop db completely
        CLog::Log(LOGERROR, "Unable to open %s (old version?)", m_strDatabaseFile.c_str());
        Close();
        m_pDB->drop();
        return false;
      }
      if (fVersion < 3)
      {
        // has to be old version - drop the version table
        m_pDS->close();
        CLog::Log(LOGINFO, "dropping version table");
        m_pDS->exec("drop table version");
        CLog::Log(LOGINFO, "creating version table");
        version = 3;
        m_pDS->exec("CREATE TABLE version (idVersion integer)\n");
        CStdString strSQL=FormatSQL("INSERT INTO version (idVersion) values(%i)\n", version);
        m_pDS->exec(strSQL.c_str());
      }
      else
//#endif
      version = m_pDS->fv("idVersion").get_asInt();
    }
  }
  CDatabase::UpdateOldVersion(version); // always call this
  if (version < m_version)
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", m_strDatabaseFile.c_str(), version, m_version);
    if (UpdateOldVersion(version) && UpdateVersionNumber())
      CLog::Log(LOGINFO, "Update to version %i successfull", m_version);
    else
    {
      CLog::Log(LOGERROR, "Can't update the database %s from version %i to %i", m_strDatabaseFile.c_str(), version, m_version);
      Close();
      return false;
    }
  }
  else if (version > m_version)
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting!", m_strDatabaseFile.c_str());
    Close();
    return false;
  }

  m_pDS->exec("PRAGMA cache_size=4096\n");
  m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
  m_pDS->exec("PRAGMA count_changes='OFF'\n");

  m_iRefCount++;
  return true;
}

bool CDatabase::IsOpen()
{
  return m_bOpen;
}

void CDatabase::Close()
{
  if (!m_bOpen)
    return ;

  if (m_iRefCount > 1)
  {
    m_iRefCount--;
    return ;
  }

  m_iRefCount--;
  m_bOpen = false;

  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
  m_pDS2.reset();
}

bool CDatabase::Compress(bool bForce /* =true */)
{
  // compress database
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (!bForce)
    {
      m_pDS->query("select iCompressCount from version");
      if (!m_pDS->eof())
      {
        int iCount = m_pDS->fv(0).get_asInt();
        if (iCount > MAX_COMPRESS_COUNT)
          iCount = -1;
        m_pDS->close();
        CStdString strSQL=FormatSQL("update version set iCompressCount=%i\n",++iCount);
        m_pDS->exec(strSQL.c_str());
        if (iCount != 0)
          return true;
      }
    }

    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Compressing the database %s failed", m_strDatabaseFile.c_str());
    return false;
  }
  return true;
}

void CDatabase::Interupt()
{
  m_pDS->interrupt();
}

void CDatabase::BeginTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->start_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:begintransaction failed");
  }
}

bool CDatabase::CommitTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->commit_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:committransaction failed");
    return false;
  }
  return true;
}

void CDatabase::RollbackTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->rollback_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:rollbacktransaction failed");
  }
}

bool CDatabase::InTransaction()
{
  if (NULL != m_pDB.get()) return false;
  return m_pDB->in_transaction();
}

bool CDatabase::CreateTables()
{
    //  Modern file systems have a cluster/block size of 4k.
    //  To gain better performance when performing write
    //  operations to the database, set the page size of the
    //  database file to 4k.
    //  This needs to be done before any table is created.
    CLog::Log(LOGINFO, "Set page size");
    m_pDS->exec("PRAGMA page_size=4096\n");
    //  Also set the memory cache size to 16k
    CLog::Log(LOGINFO, "Set default cache size");
    m_pDS->exec("PRAGMA default_cache_size=4096\n");

    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    CStdString strSQL=FormatSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n", m_version);
    m_pDS->exec(strSQL.c_str());

    return true;
}

bool CDatabase::UpdateOldVersion(int version)
{
  try
  {
    m_pDS->query("select iCompressCount from version");
    return false;
  }
  catch(...)
  {
    // add compresscount field
    m_pDS->exec("alter table version add iCompressCount integer\n");
  }
  return false;
}

bool CDatabase::UpdateVersionNumber()
{
  try
  {
    CStdString strSQL=FormatSQL("UPDATE version SET idVersion=%i\n", m_version);
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    return false;
  }

  return true;
}
