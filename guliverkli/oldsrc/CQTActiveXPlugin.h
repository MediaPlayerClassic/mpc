// CQTActiveXPlugin.h  : Declaration of ActiveX Control wrapper class(es) created by Microsoft Visual C++

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CQTActiveXPlugin

class CQTActiveXPlugin : public CWnd
{
protected:
	DECLARE_DYNCREATE(CQTActiveXPlugin)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0x2BF25D5, 0x8C17, 0x4B23, { 0xBC, 0x80, 0xD3, 0x48, 0x8A, 0xBD, 0xDC, 0x6B } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); 
	}

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, 
				UINT nID, CFile* pPersist = NULL, BOOL bStorage = FALSE,
				BSTR bstrLicKey = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// Attributes
public:

// Operations
public:

	void AddParam(LPCTSTR bstrName, LPCTSTR bstrValue)
	{
		static BYTE parms[] = VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x60020000, DISPATCH_METHOD, VT_EMPTY, NULL, parms, bstrName, bstrValue);
	}
	void Show()
	{
		InvokeHelper(0x60020001, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Hide()
	{
		InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Clear()
	{
		InvokeHelper(0x60020003, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	LPDISPATCH get_dispatch()
	{
		LPDISPATCH result;
		InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	void Play()
	{
		InvokeHelper(0x101, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Stop()
	{
		InvokeHelper(0x102, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Rewind()
	{
		InvokeHelper(0x103, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Step(long count)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x104, DISPATCH_METHOD, VT_EMPTY, NULL, parms, count);
	}
	void GoToChapter(LPCTSTR language)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x105, DISPATCH_METHOD, VT_EMPTY, NULL, parms, language);
	}
	void ShowDefaultView()
	{
		InvokeHelper(0x106, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void GoPreviousNode()
	{
		InvokeHelper(0x107, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void SendSpriteEvent(long trackIndex, long spriteID, long messageID)
	{
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x108, DISPATCH_METHOD, VT_EMPTY, NULL, parms, trackIndex, spriteID, messageID);
	}
	void SetRate(float rate)
	{
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0x109, DISPATCH_METHOD, VT_EMPTY, NULL, parms, rate);
	}
	float GetRate()
	{
		float result;
		InvokeHelper(0x10a, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	void SetTime(long time)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x10b, DISPATCH_METHOD, VT_EMPTY, NULL, parms, time);
	}
	long GetTime()
	{
		long result;
		InvokeHelper(0x10c, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetVolume(long volume)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x10d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, volume);
	}
	long GetVolume()
	{
		long result;
		InvokeHelper(0x10e, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetMovieName(LPCTSTR movieName)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x10f, DISPATCH_METHOD, VT_EMPTY, NULL, parms, movieName);
	}
	CString GetMovieName()
	{
		CString result;
		InvokeHelper(0x110, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetMovieID(long movieID)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x111, DISPATCH_METHOD, VT_EMPTY, NULL, parms, movieID);
	}
	long GetMovieID()
	{
		long result;
		InvokeHelper(0x112, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetStartTime(long time)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x113, DISPATCH_METHOD, VT_EMPTY, NULL, parms, time);
	}
	long GetStartTime()
	{
		long result;
		InvokeHelper(0x114, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetEndTime(long time)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x115, DISPATCH_METHOD, VT_EMPTY, NULL, parms, time);
	}
	long GetEndTime()
	{
		long result;
		InvokeHelper(0x116, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetBgColor(LPCTSTR color)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x117, DISPATCH_METHOD, VT_EMPTY, NULL, parms, color);
	}
	CString GetBgColor()
	{
		CString result;
		InvokeHelper(0x118, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetIsLooping(long loop)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x119, DISPATCH_METHOD, VT_EMPTY, NULL, parms, loop);
	}
	long GetIsLooping()
	{
		long result;
		InvokeHelper(0x11a, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetLoopIsPalindrome(long loop)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x11b, DISPATCH_METHOD, VT_EMPTY, NULL, parms, loop);
	}
	long GetLoopIsPalindrome()
	{
		long result;
		InvokeHelper(0x11c, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetMute()
	{
		long result;
		InvokeHelper(0x11d, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetMute(long mute)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x11e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, mute);
	}
	void SetPlayEveryFrame(long playAll)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x11f, DISPATCH_METHOD, VT_EMPTY, NULL, parms, playAll);
	}
	long GetPlayEveryFrame()
	{
		long result;
		InvokeHelper(0x120, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetAutoPlay(long autoPlay)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x121, DISPATCH_METHOD, VT_EMPTY, NULL, parms, autoPlay);
	}
	long GetAutoPlay()
	{
		long result;
		InvokeHelper(0x122, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetControllerVisible(long visible)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x123, DISPATCH_METHOD, VT_EMPTY, NULL, parms, visible);
	}
	long GetControllerVisible()
	{
		long result;
		InvokeHelper(0x124, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetHREF(LPCTSTR url)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x125, DISPATCH_METHOD, VT_EMPTY, NULL, parms, url);
	}
	CString GetHREF()
	{
		CString result;
		InvokeHelper(0x126, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetTarget(LPCTSTR target)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x127, DISPATCH_METHOD, VT_EMPTY, NULL, parms, target);
	}
	CString GetTarget()
	{
		CString result;
		InvokeHelper(0x128, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetQTNEXTUrl(long index, LPCTSTR url)
	{
		static BYTE parms[] = VTS_I4 VTS_BSTR ;
		InvokeHelper(0x129, DISPATCH_METHOD, VT_EMPTY, NULL, parms, index, url);
	}
	CString GetQTNEXTUrl(long index)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x12a, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, index);
		return result;
	}
	void SetHotspotUrl(long hotspotID, LPCTSTR url)
	{
		static BYTE parms[] = VTS_I4 VTS_BSTR ;
		InvokeHelper(0x12b, DISPATCH_METHOD, VT_EMPTY, NULL, parms, hotspotID, url);
	}
	CString GetHotspotUrl(long hotspotID)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x12c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, hotspotID);
		return result;
	}
	void SetHotspotTarget(long hotspotID, LPCTSTR target)
	{
		static BYTE parms[] = VTS_I4 VTS_BSTR ;
		InvokeHelper(0x12d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, hotspotID, target);
	}
	CString GetHotspotTarget(long hotspotID)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x12e, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, hotspotID);
		return result;
	}
	void SetURL(LPCTSTR url)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x12f, DISPATCH_METHOD, VT_EMPTY, NULL, parms, url);
	}
	CString GetURL()
	{
		CString result;
		InvokeHelper(0x130, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetKioskMode(long kioskMode)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x131, DISPATCH_METHOD, VT_EMPTY, NULL, parms, kioskMode);
	}
	long GetKioskMode()
	{
		long result;
		InvokeHelper(0x132, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetDuration()
	{
		long result;
		InvokeHelper(0x133, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetMaxTimeLoaded()
	{
		long result;
		InvokeHelper(0x134, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetTimeScale()
	{
		long result;
		InvokeHelper(0x135, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetMovieSize()
	{
		long result;
		InvokeHelper(0x136, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetMaxBytesLoaded()
	{
		long result;
		InvokeHelper(0x137, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetMatrix(LPCTSTR matrix)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x138, DISPATCH_METHOD, VT_EMPTY, NULL, parms, matrix);
	}
	CString GetMatrix()
	{
		CString result;
		InvokeHelper(0x139, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetRectangle(LPCTSTR rect)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x13a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, rect);
	}
	CString GetRectangle()
	{
		CString result;
		InvokeHelper(0x13b, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void SetLanguage(LPCTSTR language)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x13c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, language);
	}
	CString GetLanguage()
	{
		CString result;
		InvokeHelper(0x13d, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetMIMEType()
	{
		CString result;
		InvokeHelper(0x13e, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetUserData(LPCTSTR type)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x13f, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, type);
		return result;
	}
	long GetTrackCount()
	{
		long result;
		InvokeHelper(0x140, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString GetTrackName(long index)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x141, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, index);
		return result;
	}
	CString GetTrackType(long index)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x142, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, index);
		return result;
	}
	long GetTrackEnabled(long index)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x143, DISPATCH_METHOD, VT_I4, (void*)&result, parms, index);
		return result;
	}
	void SetTrackEnabled(long index, long enabled)
	{
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x144, DISPATCH_METHOD, VT_EMPTY, NULL, parms, index, enabled);
	}
	long GetChapterCount()
	{
		long result;
		InvokeHelper(0x145, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString GetChapterName(long chapterNum)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x146, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, chapterNum);
		return result;
	}
	void SetSpriteTrackVariable(long trackIndex, long variableIndex, LPCTSTR value)
	{
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_BSTR ;
		InvokeHelper(0x147, DISPATCH_METHOD, VT_EMPTY, NULL, parms, trackIndex, variableIndex, value);
	}
	CString GetSpriteTrackVariable(long trackIndex, long variableIndex)
	{
		CString result;
		static BYTE parms[] = VTS_I4 VTS_I4 ;
		InvokeHelper(0x148, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, trackIndex, variableIndex);
		return result;
	}
	long GetIsVRMovie()
	{
		long result;
		InvokeHelper(0x149, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetPanAngle(float angle)
	{
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0x14a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, angle);
	}
	float GetPanAngle()
	{
		float result;
		InvokeHelper(0x14b, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	void SetTiltAngle(float angle)
	{
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0x14c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, angle);
	}
	float GetTiltAngle()
	{
		float result;
		InvokeHelper(0x14d, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	void SetFieldOfView(float fov)
	{
		static BYTE parms[] = VTS_R4 ;
		InvokeHelper(0x14e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, fov);
	}
	float GetFieldOfView()
	{
		float result;
		InvokeHelper(0x14f, DISPATCH_METHOD, VT_R4, (void*)&result, NULL);
		return result;
	}
	long GetNodeCount()
	{
		long result;
		InvokeHelper(0x150, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetNodeID(long id)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x151, DISPATCH_METHOD, VT_EMPTY, NULL, parms, id);
	}
	long GetNodeID()
	{
		long result;
		InvokeHelper(0x152, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString GetPluginVersion()
	{
		CString result;
		InvokeHelper(0x153, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetPluginStatus()
	{
		CString result;
		InvokeHelper(0x154, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	long GetResetPropertiesOnReload()
	{
		long result;
		InvokeHelper(0x155, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	void SetResetPropertiesOnReload(long reset)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x156, DISPATCH_METHOD, VT_EMPTY, NULL, parms, reset);
	}
	CString GetQuickTimeVersion()
	{
		CString result;
		InvokeHelper(0x157, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetQuickTimeLanguage()
	{
		CString result;
		InvokeHelper(0x158, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	long GetQuickTimeConnectionSpeed()
	{
		long result;
		InvokeHelper(0x159, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long GetIsQuickTimeRegistered()
	{
		long result;
		InvokeHelper(0x15a, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString GetComponentVersion(LPCTSTR type, LPCTSTR subType, LPCTSTR manufacturer)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x15b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, type, subType, manufacturer);
		return result;
	}


};
