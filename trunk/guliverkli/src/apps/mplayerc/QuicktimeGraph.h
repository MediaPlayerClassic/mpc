#pragma once
#include "BaseGraph.h"
#include "DX7AllocatorPresenter.h"

namespace DSObjects
{

class CQuicktimeGraph;

class CQuicktimeWindow : public CPlayerWindow
{
	CDC m_dc;
	CBitmap m_bm;
	QT::GWorldPtr m_offscreenGWorld;

	CQuicktimeGraph* m_pGraph;
	FILTER_STATE m_fs;
	UINT m_idEndPoller;

	static QT::OSErr MyMovieDrawingCompleteProc(QT::Movie theMovie, long refCon);

	void ProcessMovieEvent(unsigned int message, unsigned int wParam, long lParam);

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	CQuicktimeWindow(CQuicktimeGraph* pGraph);

	bool OpenMovie(CString fn);
	void CloseMovie();

	void Run(), Pause(), Stop();
	FILTER_STATE GetState();

	QT::Movie			theMovie;
    QT::MovieController theMC;
	CSize				m_size;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
};

class CQuicktimeGraph : public CBaseGraph, public IVideoFrameStep
{
public:
	enum {MC, GDI, DX7};
	int m_iRenderer;

protected:
	bool m_fQtInitialized;

	CPlayerWindow m_wndWindowFrame;
	CQuicktimeWindow m_wndDestFrame;

	CComPtr<ISubPicAllocatorPresenter> m_pQTAP;

public:
	CQuicktimeGraph(HWND hParent, int iRenderer, HRESULT& hr);
	virtual ~CQuicktimeGraph();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

protected:
	// IGraphBuilder
    STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);

	// IMediaControl
    STDMETHODIMP Run();
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
	STDMETHODIMP GetState(LONG msTimeout, OAFilterState* pfs);

	// IMediaSeeking
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);

	// IVideoWindow
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height);

	// IBasicVideo
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height);
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight);

	// IBasicAudio
    STDMETHODIMP put_Volume(long lVolume);
    STDMETHODIMP get_Volume(long* plVolume);
	
	// IVideoFrameStep
    STDMETHODIMP Step(DWORD dwFrames, IUnknown* pStepObject);
    STDMETHODIMP CanStep(long bMultiple, IUnknown* pStepObject);    
    STDMETHODIMP CancelStep();
};

}
using namespace DSObjects;