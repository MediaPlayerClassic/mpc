#pragma once

#include <afxtempl.h>

enum exttype {EXTSRT = 0, EXTSUB, EXTSMI, EXTPSB, EXTSSA, EXTASS, EXTIDX, EXTUSF, EXTXSS};
extern TCHAR* exttypestr[9];
typedef struct {CString fn; /*exttype ext;*/} SubFile;
typedef CArray <SubFile, SubFile&> SubFiles;
extern void GetSubFileNames(CString fn, CStringArray& paths, SubFiles& ret);
