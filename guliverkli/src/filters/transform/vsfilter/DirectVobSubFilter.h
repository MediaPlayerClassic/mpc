#pragma once

#include <atlsync.h>
#include "DirectVobSub.h"
#include "DirectVobSubAllocator.h"
#include "..\..\..\subtitles\VobSubFile.h"
#include "..\..\..\subtitles\RTS.h"
#include "..\..\..\..\include\moreuuids.h"

typedef struct
{
	HWND hSystrayWnd;
	IFilterGraph* graph;
	IDirectVobSub* dvs;
	bool fRunOnce, fShowIcon;
} SystrayIconData;

/* This is for graphedit */

class CDirectVobSubFilter
	: public CTransformFilter
	, public CDirectVobSub
	, public ISpecifyPropertyPages
	, public IAMStreamSelect
	, public CAMThread
{
    friend class CDirectVobSubInputPin;
    friend class CDirectVobSubOutputPin;
    friend class CTextInputPin;

public:
    DECLARE_IUNKNOWN;
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // Overriden from CTransformFilter base class
	HRESULT Receive(IMediaSample* pSample),
			Transform(IMediaSample* pIn, IMediaSample* pOut),
			CheckInputType(const CMediaType* mtIn),
			CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut),
			DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties),
			GetMediaType(int iPosition, CMediaType* pMediaType),
			SetMediaType(PIN_DIRECTION direction, const CMediaType* pMediaType),
			CheckConnect(PIN_DIRECTION direction, IPin* pPin),
			CompleteConnect(PIN_DIRECTION direction, IPin* pReceivePin),
			BreakConnect(PIN_DIRECTION direction),
			StartStreaming(), 
			StopStreaming(),
			NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT CheckOutputType(const CMediaType* mtOut);

    // Overriden from CBaseFilter base class
	STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

    CArray<CTextInputPin*> m_pTextInput;

	CDirectVobSubAllocator m_Allocator;
	bool m_fUsingOwnAllocator;

	CBasePin* GetPin(int n);
	int GetPinCount();

    // IDirectVobSub
    STDMETHODIMP put_FileName(WCHAR* fn);
	STDMETHODIMP get_LanguageCount(int* nLangs);
	STDMETHODIMP get_LanguageName(int iLanguage, WCHAR** ppName);
	STDMETHODIMP put_SelectedLanguage(int iSelected);
    STDMETHODIMP put_HideSubtitles(bool fHideSubtitles);
	STDMETHODIMP put_PreBuffering(bool fDoPreBuffering);
    STDMETHODIMP put_Placement(bool fOverridePlacement, int xperc, int yperc);
    STDMETHODIMP put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fPolygonize);
    STDMETHODIMP put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer);
    STDMETHODIMP put_SubtitleTiming(int delay, int speedmul, int speeddiv);
    STDMETHODIMP get_MediaFPS(bool* fEnabled, double* fps);
    STDMETHODIMP put_MediaFPS(bool fEnabled, double fps);
    STDMETHODIMP get_ZoomRect(NORMALIZEDRECT* rect);
    STDMETHODIMP put_ZoomRect(NORMALIZEDRECT* rect);
    STDMETHODIMP get_ColorFormat(int* iPosition);
    STDMETHODIMP put_ColorFormat(int iPosition);
	STDMETHODIMP HasConfigDialog(int iSelected);
	STDMETHODIMP ShowConfigDialog(int iSelected, HWND hWndParent);

	// IDirectVobSub2
	STDMETHODIMP put_TextSettings(STSStyle* pDefStyle);

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID* pPages);

	// IAMStreamSelect
	STDMETHODIMP Count(DWORD* pcStreams); 
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags); 
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);  

    // CPersistStream
	STDMETHODIMP GetClassID(CLSID* pClsid);

protected:
    CDirectVobSubFilter(TCHAR* tszName, LPUNKNOWN punk, HRESULT* phr, const GUID& guid);
	virtual ~CDirectVobSubFilter();

protected:
	int m_wIn;
	BITMAPINFOHEADER m_bihIn, m_bihOut;
	CSize m_sizeSub;

	HRESULT ChangeMediaType(int iPosition);

	bool AdjustFrameSize(CSize& s);
	HRESULT ConvertMediaTypeInputToOutput(CMediaType* pmt, int iVIHTemplate = -1, bool fVIH2 = false);

	HDC m_hdc;
	HBITMAP m_hbm;
	HFONT m_hfont;
	void PrintMessages(BYTE* pOut);

/* ResX2 */
	bool m_fResX2Active;
	CAutoVectorPtr<BYTE> m_pTempPicBuff;
	HRESULT Copy(BYTE* pSub, BYTE* pIn, CSize sub, CSize in, int widthIn, int bpp, const GUID& subtype, DWORD black, bool fResX2);

	// segment start time, absolute time
	CRefTime m_tPrev;
	REFERENCE_TIME CalcCurrentTime();

	double m_fps;

	// 3.x- versions of microsoft's mpeg4 codec output flipped image
	bool m_fMSMpeg4Fix;

	// DivxPlus puts our picture upside-down on the tv
	bool m_fDivxPlusFix;

	// don't set the "hide subtitles" stream until we are finished with loading
	bool m_fLoading;

	bool Open();

	int FindPreferedLanguage(bool fHideToo = true);
	void UpdatePreferedLanguages(CString lang);

	CCritSec m_csQueueLock;
	CComPtr<ISubPicQueue> m_pSubPicQueue;
	void InitSubPicQueue();
	SubPicDesc m_spd;

	CCritSec m_csSubLock;
	CInterfaceList<ISubStream> m_pSubStreams;
	DWORD_PTR m_nSubtitleId;
	void UpdateSubtitle(bool fApplyDefStyle = true);
	void SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle = true);
	void InvalidateSubtitle(DWORD_PTR nSubtitleId = -1);

	// the text input pin is using these
	void AddSubStream(ISubStream* pSubStream);
	void RemoveSubStream(ISubStream* pSubStream);
	void Post_EC_OLE_EVENT(CString str, DWORD_PTR nSubtitleId = -1);

private:
	class CFileReloaderData
	{
	public:
		ATL::CEvent EndThreadEvent, RefreshEvent;
		CStringList files;
		CArray<CTime> mtime;
	} m_frd;

	void SetupFRD(CStringArray& paths, CArray<HANDLE>& handles);
	DWORD ThreadProc();

private:
	HANDLE m_hSystrayThread;
	SystrayIconData m_tbid;
};

/* The "auto-loading" version */

class CDirectVobSubFilter2
	: public CDirectVobSubFilter
{
	bool ShouldWeAutoload(IFilterGraph* pGraph);
	void GetRidOfInternalScriptRenderer();

public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

	// Overriden from CTransformFilter base class
	HRESULT CheckConnect(PIN_DIRECTION dir, IPin* pPin);
	STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);
    HRESULT CheckInputType(const CMediaType* mtIn);

private:
    CDirectVobSubFilter2(TCHAR* tszName, LPUNKNOWN punk, HRESULT* phr, const GUID& guid);
};

