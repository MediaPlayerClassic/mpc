// Copyright (C) 2003-2004 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
	bool bOK = false;

	HKEY hKey;
	LONG ec = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, KEY_ALL_ACCESS, &hKey);
	if(ec == ERROR_SUCCESS)
	{
		if(pszSubkey != 0)
			ec = ::RegDeleteKey(hKey, pszSubkey);

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
	bool bOK = false;

	CString szKey(pszKey);
	if(pszSubkey != 0)
		szKey += CString(_T("\\")) + pszSubkey;

	HKEY hKey;
	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if(ec == ERROR_SUCCESS)
	{
		if(pszValue != 0)
		{
			ec = ::RegSetValueEx(hKey, pszValueName, 0, REG_SZ,
				reinterpret_cast<BYTE*>(const_cast<LPTSTR>(pszValue)),
				(_tcslen(pszValue) + 1) * sizeof(TCHAR));
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue)
{
	return SetRegKeyValue(pszKey, pszSubkey, 0, pszValue);
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext = NULL, ...)
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(subtype2);
/*
	SetRegKeyValue(_T("Media Type\\") + null, subtype, _T("0"), chkbytes);
	SetRegKeyValue(_T("Media Type\\") + null, subtype, _T("Source Filter"), CStringFromGUID(clsid));
*/
/*
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("0"), chkbytes);
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("Source Filter"), CStringFromGUID(CLSID_AsyncReader));
*/
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("0"), chkbytes);
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("Source Filter"), CStringFromGUID(clsid));
	
	DeleteRegKey(_T("Media Type\\") + null, subtype);

	va_list marker;
	va_start(marker, ext);
	for(; ext; ext = va_arg(marker, LPCTSTR))
		DeleteRegKey(_T("Media Type\\Extensions"), ext);
	va_end(marker);
}

void UnRegisterSourceFilter(const GUID& subtype)
{
	DeleteRegKey(_T("Media Type\\") + CStringFromGUID(MEDIATYPE_Stream), CStringFromGUID(subtype));
}

template <class T>
CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new T(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}
