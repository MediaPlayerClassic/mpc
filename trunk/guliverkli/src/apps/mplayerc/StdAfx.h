// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__C76533D6_6242_4BEB_8FD3_C6BE58F07224__INCLUDED_)
#define AFX_STDAFX_H__C76533D6_6242_4BEB_8FD3_C6BE58F07224__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>

#define ResStr(id) CString(MAKEINTRESOURCE(id))

#include <afxdisp.h>
#include <Shlwapi.h>
#include <atlpath.h>
#include <streams.h>
#include <dvdmedia.h>
#include <mpconfig.h>
#include "..\..\..\include\qt\qt.h"
#include <afxole.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C76533D6_6242_4BEB_8FD3_C6BE58F07224__INCLUDED_)
