#pragma once
#include "BaseGraph.h"

namespace DSObjects
{

//class CQuicktimeGraph2;
/*
class CQtWindow : public CPlayerWindow
{
	CQuicktimeGraph2* m_pGraph;
	UINT m_idEndPoller;

	void ProcessMovieEvent(unsigned int message, unsigned int wParam, long lParam);
	void CreateNewMovieController(QT::Movie theMovie);

	static QT::Boolean MCFilter(QT::MovieController mc, short action, void* params, long refCon);
	QT::Boolean MCFilter(QT::MovieController mc, short action, void* params);

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	CQtWindow(CQuicktimeGraph2* pGraph);

	void Run(), Pause(), Stop();
	FILTER_STATE GetState();

    QT::MovieController theMC;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
};
*/
class CQuicktimeGraph : public CBaseGraph, public IVideoFrameStep
{
protected:
	HWND m_hWndParent;
	bool m_fQtInitialized;
	QT::Movie m_theMovie;
	CRect m_theMovieRect;
	FILTER_STATE m_fs;

	virtual bool OpenMovie(CString fn);
	virtual void CloseMovie();

public:
	CQuicktimeGraph(HWND hWndParent, HRESULT& hr);
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
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height)/* = 0*/;

	// IBasicVideo
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height)/* = 0*/;
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight);

	// IBasicAudio
    STDMETHODIMP put_Volume(long lVolume);
    STDMETHODIMP get_Volume(long* plVolume);
	
	// IVideoFrameStep
    STDMETHODIMP Step(DWORD dwFrames, IUnknown* pStepObject);
    STDMETHODIMP CanStep(long bMultiple, IUnknown* pStepObject);    
    STDMETHODIMP CancelStep();
};

///////////////////////////////////////////

class CQuicktimeWindow : public CPlayerWindow
{
protected:
	CQuicktimeGraph* m_pGraph;
	UINT m_idEndPoller;

	void ProcessMovieEvent(unsigned int message, unsigned int wParam, long lParam);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	CQuicktimeWindow(CQuicktimeGraph* pGraph);

    QT::MovieController m_theMC;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT nIDEvent);
};
/*
class CQuicktimeGraphWindowed : public CQuicktimeGraph
{
protected:
    QT::MovieController m_theMC;

	CPlayerWindow m_wndWindowFrame;
	CQuicktimeWindow m_wndDestFrame;

	virtual bool OpenMovie(CString fn);
	virtual void CloseMovie();

public:
	CQuicktimeGraphWindowed(HWND hParent, HRESULT& hr);
	virtual ~CQuicktimeGraphWindowed();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

protected:
	// IMediaControl
    STDMETHODIMP Run();
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

	// IVideoWindow
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height);

	// IBasicVideo
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height);

	// IVideoFrameStep
    STDMETHODIMP Step(DWORD dwFrames, IUnknown* pStepObject);
};
*/
}
using namespace DSObjects;