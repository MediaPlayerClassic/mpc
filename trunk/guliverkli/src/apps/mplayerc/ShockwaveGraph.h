#pragma once

#include "BaseGraph.h"
#include "CShockwaveFlash.h"

namespace DSObjects
{

class CShockwaveGraph : public CBaseGraph
{
	CPlayerWindow m_wndWindowFrame;
	CShockwaveFlash m_wndDestFrame;

	FILTER_STATE m_fs;

public:
	CShockwaveGraph(HWND hParent, HRESULT& hr);
	virtual ~CShockwaveGraph();

protected:
	// IGraphBuilder
    STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);

	// IMediaControl
    STDMETHODIMP Run();
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
	STDMETHODIMP GetState(LONG msTimeout, OAFilterState* pfs);

	// IMediaSeeking
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

	// IVideoWindow
	STDMETHODIMP put_Visible(long Visible);
	STDMETHODIMP get_Visible(long* pVisible);
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height);

	// IBasicVideo
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height);
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight);

	// IBasicAudio
    STDMETHODIMP put_Volume(long lVolume);
    STDMETHODIMP get_Volume(long* plVolume);

	// IAMOpenProgress
	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
};

}
using namespace DSObjects;