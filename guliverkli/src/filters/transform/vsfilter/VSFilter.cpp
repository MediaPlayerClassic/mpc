/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DirectVobSubFilter.h"
#include "DirectVobSubInputPin.h"
#include "DirectVobSubOutputPin.h"
#include "TextInputPin.h"
#include "DirectVobSubPropPage.h"
#include "VSFilter.h"
#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\DSUtil\MediaTypes.h"

#include <initguid.h>
#include <uuids.h>
#include "..\..\..\..\include\moreuuids.h"
#include "DirectVobSubUIDs.h"
#include "..\..\..\..\include\Ogg\OggDS.h"


/////////////////////////////////////////////////////////////////////////////
// CVSFilterApp 

BEGIN_MESSAGE_MAP(CVSFilterApp, CWinApp)
END_MESSAGE_MAP()

CVSFilterApp::CVSFilterApp()
{
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL CVSFilterApp::InitInstance()
{
	if(!CWinApp::InitInstance())
		return FALSE;

	SetRegistryKey(_T("Gabest"));

	DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_ATTACH, 0); // "DllMain" of the dshow baseclasses

	STARTUPINFO si;
	GetStartupInfo(&si);
	m_AppName = CString(si.lpTitle);
	m_AppName.Replace('\\', '/');
	m_AppName = m_AppName.Mid(m_AppName.ReverseFind('/')+1);
	m_AppName.MakeLower();

	return TRUE;
}

int CVSFilterApp::ExitInstance()
{
	DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_DETACH, 0); // "DllMain" of the dshow baseclasses

	return CWinApp::ExitInstance();
}

HINSTANCE CVSFilterApp::LoadAppLangResourceDLL()
{
	CString fn;
	fn.ReleaseBufferSetLength(::GetModuleFileName(m_hInstance, fn.GetBuffer(MAX_PATH), MAX_PATH));
	fn = fn.Mid(fn.ReverseFind('\\')+1);
	fn = fn.Left(fn.ReverseFind('.')+1);
	fn = fn + _T("lang");
	return ::LoadLibrary(fn);
}

CVSFilterApp theApp;

//////////////////////////////////////////////////////////////////////////

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_NULL, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YUY2},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV12},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_I420},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_IYUV},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB32},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB565},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB555},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB24},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesIn2[] =
{
	{&MEDIATYPE_Text, &MEDIASUBTYPE_None},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_None},
};


const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn)/sizeof(sudPinTypesIn[0]), // Number of types
      sudPinTypesIn         // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
      sudPinTypesOut       // Pin information
    },
    { L"Input2",            // Pins string name
      TRUE,                 // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      TRUE,                 // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn2)/sizeof(sudPinTypesIn2[0]), // Number of types
      sudPinTypesIn2       // Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter =
{
    &CLSID_DirectVobSubFilter,    // Filter CLSID
    L"DirectVobSub",        // String name
    MERIT_DO_NOT_USE,       // Filter merit
    sizeof(sudpPins)/sizeof(sudpPins[0]), // Number of pins
    sudpPins                // Pin information
};

/*removeme*/
/*const*/ AMOVIESETUP_FILTER sudFilter2 =
{
    &CLSID_DirectVobSubFilter2,    // Filter CLSID
    L"DirectVobSub (auto-loading version)", // String name
    MERIT_PREFERRED+2,      // Filter merit
    sizeof(sudpPins)/sizeof(sudpPins[0]), // Number of pins
    sudpPins                // Pin information
};

CFactoryTemplate g_Templates[] =
{
    { L"DirectVobSub"
    , &CLSID_DirectVobSubFilter
    , CDirectVobSubFilter::CreateInstance
    , NULL
    , &sudFilter }
  ,
    { L"DirectVobSub (auto-loading version)"
    , &CLSID_DirectVobSubFilter2
    , CDirectVobSubFilter2::CreateInstance
    , NULL
    , &sudFilter2 }
  ,
    { L"DVSMainPPage"
    , &CLSID_DVSMainPPage
    , CDVSMainPPage::CreateInstance }
  ,
    { L"DVSGeneralPPage"
    , &CLSID_DVSGeneralPPage
    , CDVSGeneralPPage::CreateInstance }
  ,
    { L"DVSMiscPPage"
    , &CLSID_DVSMiscPPage
    , CDVSMiscPPage::CreateInstance }
  ,
    { L"DVSTimingPPage"
    , &CLSID_DVSTimingPPage
    , CDVSTimingPPage::CreateInstance }
  ,
    { L"DVSZoomPPage"
    , &CLSID_DVSZoomPPage
    , CDVSZoomPPage::CreateInstance }
  ,
    { L"DVSColorPPage"
    , &CLSID_DVSColorPPage
    , CDVSColorPPage::CreateInstance }
  ,
    { L"DVSPathsPPage"
    , &CLSID_DVSPathsPPage
    , CDVSPathsPPage::CreateInstance }
  ,
    { L"DVSAboutPPage"
    , &CLSID_DVSAboutPPage
    , CDVSAboutPPage::CreateInstance }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//////////////////////////////
/*removeme*/
extern void JajDeGonoszVagyok();

STDAPI DllRegisterServer()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if(theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 0) != 1)
		theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 0);

	if(theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_VMRZOOMENABLED), -1) == -1)
		theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_VMRZOOMENABLED), 0);

	if(theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ENABLEZPICON), -1) == -1)
		theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ENABLEZPICON), 0);

	/*removeme*/
	JajDeGonoszVagyok();

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
//	DVS_WriteProfileInt2(IDS_R_GENERAL, IDS_RG_SEENDIVXWARNING, 0);

	return AMovieDllRegisterServer2(FALSE);
}

void CALLBACK DirectVobSub(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	if(FAILED(::CoInitialize(0))) return;

    CComPtr<IBaseFilter> pFilter;
	CComQIPtr<ISpecifyPropertyPages> pSpecify;

	if(SUCCEEDED(pFilter.CoCreateInstance(CLSID_DirectVobSubFilter)) && (pSpecify = pFilter))
	{
		ShowPPage(pFilter, hwnd);
	}

	::CoUninitialize();
}
