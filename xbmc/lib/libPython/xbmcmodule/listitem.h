#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#endif
#include "GUIListItem.h"
#pragma once

#define ListItem_Check(op) PyObject_TypeCheck(op, &ListItem_Type)
#define ListItem_CheckExact(op) ((op)->ob_type == &ListItem_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  extern PyTypeObject ListItem_Type;

  typedef struct {
    PyObject_HEAD
    CGUIListItem* item;
  } ListItem;

  extern ListItem* ListItem_FromString(string strLabel);

  void initListItem_Type();
}

#ifdef __cplusplus
}
#endif
