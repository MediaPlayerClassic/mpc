#include "stdafx.h"
#include <math.h>
#include "QuicktimeGraph.h"
#include "..\..\DSUtil\DSUtil.h"

//
// CQuicktimeGraph
//

#pragma warning(disable:4355) // 'this' : used in base member initializer list

using namespace QT;

CQuicktimeGraph::CQuicktimeGraph(HWND hWndParent, HRESULT& hr)
	: CBaseGraph()
	, m_hWndParent(hWndParent)
	, m_theMovie(NULL)
	, m_theMovieRect(0,0,0,0)
	, m_fs(State_Stopped)
{
	hr = S_OK;

	m_fQtInitialized = false;
	if(InitializeQTML(0) != 0) {hr = E_FAIL; return;}
	if(EnterMovies() != 0) {TerminateQTML(); hr = E_FAIL; return;}
	m_fQtInitialized = true;
}

CQuicktimeGraph::~CQuicktimeGraph()
{
	if(m_fQtInitialized)
	{
		ExitMovies();
		TerminateQTML();
	}
}

STDMETHODIMP CQuicktimeGraph::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IVideoFrameStep)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CQuicktimeGraph::OpenMovie(CString fn)
{
	if(!(fn.GetLength() > 0 && fn.GetLength() < 255))
		return(false);

	CloseMovie();
/*
CreatePortAssociation(m_hWndParent, NULL, 0);	
// Set the port	
SetGWorld((CGrafPtr)GetHWNDPort(m_hWndParent), NULL);
*/
	unsigned char theFullPath[256] = {fn.GetLength(), 0};
	strcat((char*)&theFullPath[1], (const char*)CStringA(fn));

	// Make a FSSpec with a pascal string filename
	FSSpec sfFile;
	FSMakeFSSpec(0, 0L, theFullPath, &sfFile);

	// Open the movie file
	short movieResFile;
	OSErr err = OpenMovieFile(&sfFile, &movieResFile, fsRdPerm);
	if(err == noErr)
	{
		err = NewMovieFromFile(&m_theMovie, movieResFile, 0, 0, newMovieActive, 0);
		CloseMovieFile(movieResFile);
		if(err != noErr) return(false);

		Rect theMovieRect;
		GetMovieBox(m_theMovie, &theMovieRect);
		MacOffsetRect(&theMovieRect, -theMovieRect.left, -theMovieRect.top);
		m_theMovieRect.SetRect(theMovieRect.left, theMovieRect.top, theMovieRect.right, theMovieRect.bottom);


		{
			// Create the movie controller
//			CreateNewMovieController(m_theMovie);
		}
	}

	return(m_theMovie != NULL);
}

void CQuicktimeGraph::CloseMovie()
{
	if(m_theMovie) DisposeMovie(m_theMovie), m_theMovie = NULL;
	m_theMovieRect.SetRectEmpty();
	m_fs = State_Stopped;
/*
if(CGrafPtr windowPort = (CGrafPtr)GetHWNDPort(m_hWndParent))
	DestroyPortAssociation(windowPort);
*/
}

// IGraphBuilder
STDMETHODIMP CQuicktimeGraph::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
	bool fRet = OpenMovie(CString(lpcwstrFile));
	if(fRet) NotifyEvent(EC_BG_AUDIO_CHANGED, 2, 0); // FIXME: dunno how to get some info on the audio yet
	return fRet ? S_OK : E_FAIL;
}

// IMediaControl
STDMETHODIMP CQuicktimeGraph::Run()
{
	if(!m_theMovie) return E_FAIL;
	StartMovie(m_theMovie);
	m_fs = State_Running;
	return S_OK;
}
STDMETHODIMP CQuicktimeGraph::Pause()
{	
	if(!m_theMovie) return E_FAIL;
	StopMovie(m_theMovie);
	m_fs = State_Paused;
	return S_OK;
}
STDMETHODIMP CQuicktimeGraph::Stop()
{
	if(!m_theMovie) return E_FAIL;
	StopMovie(m_theMovie);
	GoToBeginningOfMovie(m_theMovie);
	m_fs = State_Stopped;
	return S_OK;
}
STDMETHODIMP CQuicktimeGraph::GetState(LONG msTimeout, OAFilterState* pfs)
{
//if(m_theMovie) MoviesTask(m_theMovie, 0);
	return pfs ? *pfs = m_fs, S_OK : E_POINTER;
}

// IMediaSeeking
STDMETHODIMP CQuicktimeGraph::GetDuration(LONGLONG* pDuration)
{
	return pDuration ? *pDuration = (m_theMovie ? 10000i64*GetMovieDuration(m_theMovie) : 0), S_OK : E_POINTER;
}
STDMETHODIMP CQuicktimeGraph::GetCurrentPosition(LONGLONG* pCurrent)
{
	TimeRecord tr;
	return pCurrent ? *pCurrent = (m_theMovie ? 10000i64*GetMovieTime(m_theMovie, &tr) : 0), S_OK : E_POINTER;
}
STDMETHODIMP CQuicktimeGraph::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	CheckPointer(pCurrent, E_POINTER);
	if(!(dwCurrentFlags&AM_SEEKING_AbsolutePositioning)) return E_INVALIDARG;
	return m_theMovie ? SetMovieTimeValue(m_theMovie, (TimeValue)(*pCurrent/10000i64)), S_OK : E_UNEXPECTED;

}
STDMETHODIMP CQuicktimeGraph::SetRate(double dRate)
{
	return m_theMovie ? SetMovieRate(m_theMovie, (Fixed)(dRate * 0x10000)), S_OK : E_UNEXPECTED;
}
STDMETHODIMP CQuicktimeGraph::GetRate(double* pdRate)
{
	CheckPointer(pdRate, E_POINTER);
	*pdRate = 1.0;
	return m_theMovie ? *pdRate = (double)GetMovieRate(m_theMovie) / 0x10000, S_OK : E_UNEXPECTED;
}

// IVideoWindow
STDMETHODIMP CQuicktimeGraph::SetWindowPosition(long Left, long Top, long Width, long Height)
{
//	if(IsWindow(m_wndWindowFrame.m_hWnd))
//		m_wndWindowFrame.MoveWindow(Left, Top, Width, Height);

	return S_OK;
}

// IBasicVideo
STDMETHODIMP CQuicktimeGraph::SetDestinationPosition(long Left, long Top, long Width, long Height)// {return E_NOTIMPL;}
{
/*
	if(IsWindow(m_wndDestFrame.m_hWnd))
	{
		m_wndDestFrame.MoveWindow(Left, Top, Width, Height);

		if(m_wndDestFrame.movieOpened)
		{
			Rect bounds = {0,0,(short)Height,(short)Width};
			MCPositionController(m_wndDestFrame.theMC, &bounds, NULL, mcTopLeftMovie|mcScaleMovieToFit);
//			MCSetControllerBoundsRect(m_wndDestFrame.theMC, &bounds);
		}
	}
*/
	return S_OK;
}

STDMETHODIMP CQuicktimeGraph::GetVideoSize(long* pWidth, long* pHeight)
{
	if(!pWidth || !pHeight) return E_POINTER;

	*pWidth = m_theMovieRect.Width();
	*pHeight = m_theMovieRect.Height();

	return S_OK;
}

// IBasicAudio
STDMETHODIMP CQuicktimeGraph::put_Volume(long lVolume)
{
	if(m_theMovie)
	{
		lVolume = (lVolume == -10000) ? 0 : (int)pow(10, (double)lVolume/4152.41 + 2.41);
		SetMovieVolume(m_theMovie, (short)max(min(lVolume, 256), 0));
		return S_OK;
	}

	return E_UNEXPECTED;
}
STDMETHODIMP CQuicktimeGraph::get_Volume(long* plVolume)
{
	CheckPointer(plVolume, E_POINTER);

	if(m_theMovie)
	{
		*plVolume = (int)((log10(*plVolume)-2.41)*4152.41);
		*plVolume = max(min(*plVolume, 0), -10000);
		return S_OK;
	}

	return E_UNEXPECTED;
}

// IVideoFrameStep
STDMETHODIMP CQuicktimeGraph::Step(DWORD dwFrames, IUnknown* pStepObject)
{
	if(pStepObject) return E_INVALIDARG;
	if(dwFrames == 0) return S_OK;
	if(!m_theMovie) return E_UNEXPECTED;

	OSType myTypes[] = {VisualMediaCharacteristic};
	TimeValue myCurrTime = GetMovieTime(m_theMovie, NULL);
	Fixed theRate = (int)dwFrames > 0 ? 0x00010000 : 0xffff0000;

	for(int nSteps = abs((int)dwFrames); nSteps > 0; nSteps--)
	{
		TimeValue myNextTime;
		GetMovieNextInterestingTime(m_theMovie, nextTimeStep, 1, myTypes, myCurrTime, theRate, &myNextTime, NULL);
		if(GetMoviesError() != noErr) return E_FAIL;
		myCurrTime = myNextTime;
	}

	if(myCurrTime >= 0 && myCurrTime < GetMovieDuration(m_theMovie))
	{
        SetMovieTimeValue(m_theMovie, myCurrTime);
		if(GetMoviesError() != noErr) return E_FAIL;
		UpdateMovie(m_theMovie);
		if(GetMoviesError() != noErr) return E_FAIL;
		MoviesTask(m_theMovie, 0L);
	}
	
/*
	short myStep = (short)(long)dwFrames;
	return noErr == MCDoAction(m_wndDestFrame.theMC, mcActionStep, (Ptr)myStep)
		? NotifyEvent(EC_STEP_COMPLETE), S_OK : E_FAIL;
*/
	NotifyEvent(EC_STEP_COMPLETE);

	return S_OK;
}
STDMETHODIMP CQuicktimeGraph::CanStep(long bMultiple, IUnknown* pStepObject)
{
	return m_theMovie ? S_OK : S_FALSE;
}
STDMETHODIMP CQuicktimeGraph::CancelStep()
{
	return E_NOTIMPL;
}

///////////////////////////////////////

//
// CQuicktimeWindow
//

CQuicktimeWindow::CQuicktimeWindow(CQuicktimeGraph* pGraph, QT::MovieController theMC)
	: m_pGraph(pGraph)
	, m_theMC(theMC)
	, m_idEndPoller(0)
{
}

void CQuicktimeWindow::ProcessMovieEvent(unsigned int message, unsigned int wParam, long lParam) 
{	
	if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST
	|| message >= WM_KEYFIRST && message <= WM_KEYLAST)
		return;

	if(!m_theMC)
		return;

	// Convert the Windows event to a QTML event
	MSG				theMsg;
	EventRecord		macEvent;
	LONG			thePoints = GetMessagePos();

	theMsg.hwnd = m_hWnd;
	theMsg.message = message;
	theMsg.wParam = wParam;
	theMsg.lParam = lParam;
	theMsg.time = GetMessageTime();
	theMsg.pt.x = LOWORD(thePoints);
	theMsg.pt.y = HIWORD(thePoints);

	// tranlate a windows event to a mac event
	WinEventToMacEvent(&theMsg, &macEvent);

	// Pump messages as mac event
	MCIsPlayerEvent(m_theMC, (const EventRecord*)&macEvent);
}

LRESULT CQuicktimeWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_ERASEBKGND)
	{
		LRESULT theResult = __super::WindowProc(message, wParam, lParam);
		ProcessMovieEvent(message, wParam, lParam);	
		return theResult;
	}
	else
	{
		ProcessMovieEvent(message, wParam, lParam);
		return __super::WindowProc(message, wParam, lParam);
	}
}

BEGIN_MESSAGE_MAP(CQuicktimeWindow, CPlayerWindow)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
END_MESSAGE_MAP()

int CQuicktimeWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Create GrafPort <-> HWND association
	CreatePortAssociation(m_hWnd, NULL, 0);	

	return 0;
}

void CQuicktimeWindow::OnDestroy()
{
	__super::OnDestroy();

	if(m_hWnd)
		if(CGrafPtr windowPort = (CGrafPtr)GetHWNDPort(m_hWnd))
			DestroyPortAssociation(windowPort);
}

BOOL CQuicktimeWindow::OnEraseBkgnd(CDC* pDC)
{
	return m_theMC ? TRUE : __super::OnEraseBkgnd(pDC);
}

void CQuicktimeWindow::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == m_idEndPoller)
	{
		if(movieOpened && IsMovieDone(theMovie))
		{
			Pause();
			m_pGraph->NotifyEvent(EC_COMPLETE);
		}
	}

	CPlayerWindow::OnTimer(nIDEvent);
}
