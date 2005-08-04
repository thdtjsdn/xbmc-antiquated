#pragma once
#include "idirectory.h"

namespace DIRECTORY
{

  /*!
  \ingroup windows 
  \brief Get access to shares and it's directories.
  */
  class CVirtualDirectory : public IDirectory
  {
  public:
    CVirtualDirectory(void);
    virtual ~CVirtualDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    void SetShares(VECSHARES& vecShares);
    inline unsigned int GetNumberOfShares() { return m_vecShares.size(); }
    void AddShare(const CShare& share);
    bool RemoveShare(const CStdString& strPath);
    bool IsShare(const CStdString& strPath) const;
    CStdString GetDVDDriveUrl();

    inline const CShare& operator [](const int index) const
    {
      return m_vecShares[index];
    }

  protected:
    void CacheThumbs(CFileItemList &items);
    VECSHARES m_vecShares;
  };
};
