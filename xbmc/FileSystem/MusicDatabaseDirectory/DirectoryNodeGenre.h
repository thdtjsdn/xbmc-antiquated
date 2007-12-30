#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CDirectoryNodeGenre : public CDirectoryNode
    {
    public:
      CDirectoryNodeGenre(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  }
}


