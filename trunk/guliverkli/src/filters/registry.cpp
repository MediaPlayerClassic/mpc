
bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
	bool bOK = false;

	HKEY hKey;
	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
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
