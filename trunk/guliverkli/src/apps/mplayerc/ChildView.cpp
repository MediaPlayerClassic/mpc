// ChildView.cpp : implementation of the CChildView class
//

#include "stdafx.h"
#include "mplayerc.h"
#include "ChildView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildView

CChildView::CChildView() : m_vrect(0,0,0,0)
{
	m_lastlmdowntime = 0;
	m_lastlmdownpoint.SetPoint(0, 0);

	CString logofile = AfxGetApp()->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOFILE), _T(""));

	if(!logofile.IsEmpty())
	{
		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx(&vi);
		if(vi.dwMajorVersion >= 5 && vi.dwMinorVersion >= 1)
			m_logo.Load(logofile);
	}
	else
	{
		m_logo.LoadFromResource(AfxGetInstanceHandle(), IDB_LOGO);
	}
}

CChildView::~CChildView()
{
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if(!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_HAND), HBRUSH(COLOR_WINDOW+1), NULL);

	return TRUE;
}

BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MYMOUSELAST)
	{
		CWnd* pParent = GetParent();
		CPoint p(pMsg->lParam);
		::MapWindowPoints(pMsg->hwnd, pParent->m_hWnd, &p, 1);

		bool fDblClick = false;

		bool fInteractiveVideo = ((CMainFrame*)AfxGetMainWnd())->IsInteractiveVideo();
/*
		if(fInteractiveVideo)
		{
			if(pMsg->message == WM_LBUTTONDOWN)
			{
				if((pMsg->time - m_lastlmdowntime) <= GetDoubleClickTime()
				&& abs(pMsg->pt.x - m_lastlmdownpoint.x) <= GetSystemMetrics(SM_CXDOUBLECLK)
				&& abs(pMsg->pt.y - m_lastlmdownpoint.y) <= GetSystemMetrics(SM_CYDOUBLECLK))
				{
					fDblClick = true;
					m_lastlmdowntime = 0;
					m_lastlmdownpoint.SetPoint(0, 0);
				}
				else
				{
					m_lastlmdowntime = pMsg->time;
					m_lastlmdownpoint = pMsg->pt;
				}
			}
			else if(pMsg->message == WM_LBUTTONDBLCLK)
			{
				m_lastlmdowntime = pMsg->time;
				m_lastlmdownpoint = pMsg->pt;
			}
		}
*/
		if((pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)
		&& fInteractiveVideo)
		{
			if(pMsg->message == WM_MOUSEMOVE)
			{
				pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
			}

			if(fDblClick)
			{
				pParent->PostMessage(WM_LBUTTONDOWN, pMsg->wParam, MAKELPARAM(p.x, p.y));
				pParent->PostMessage(WM_LBUTTONDBLCLK, pMsg->wParam, MAKELPARAM(p.x, p.y));
			}
		}
		else
		{
			pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
            return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CChildView::SetVideoRect(CRect r)
{
	m_vrect = r;

	Invalidate();
}

IMPLEMENT_DYNAMIC(CChildView, CWnd)

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	//{{AFX_MSG_MAP(CChildView)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_COMMAND_EX(ID_PLAY_PLAYPAUSE, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_PAUSE, OnPlayPlayPauseStop)
	ON_COMMAND_EX(ID_PLAY_STOP, OnPlayPlayPauseStop)
	ON_WM_SETCURSOR()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChildView message handlers

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	((CMainFrame*)GetParentFrame())->RepaintVideo();

	// Do not call CWnd::OnPaint() for painting messages
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	CRect r;

	if(((CMainFrame*)GetParentFrame())->IsSomethingLoaded())
	{
		pDC->ExcludeClipRect(m_vrect);
	}
	else if(!m_logo.IsNull() && ((CMainFrame*)GetParentFrame())->IsPlaylistEmpty())
	{
		GetClientRect(r);
		int w = min(m_logo.GetWidth(), r.Width());
		int h = min(m_logo.GetHeight(), r.Height());
		int x = (r.Width() - w) / 2;
		int y = (r.Height() - h) / 2;
		r = CRect(CPoint(x, y), CSize(w, h));

		m_logo.Draw(*pDC, r);

		pDC->ExcludeClipRect(r);
	}

	GetClientRect(r);
	pDC->FillSolidRect(r, 0);

	return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	((CMainFrame*)GetParentFrame())->MoveVideoWindow();
}

void CChildView::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanged(lpwndpos);

	((CMainFrame*)GetParentFrame())->MoveVideoWindow();
}

BOOL CChildView::OnPlayPlayPauseStop(UINT nID)
{
	if(nID == ID_PLAY_STOP) SetVideoRect();
	return FALSE;
}

BOOL CChildView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if(((CMainFrame*)GetParentFrame())->m_fHideCursor)
	{
		SetCursor(NULL);
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CChildView::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if(!((CMainFrame*)GetParentFrame())->IsFrameLessWindow()) 
	{
		InflateRect(&lpncsp->rgrc[0], -1, -1);
	}

	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
}

void CChildView::OnNcPaint()
{
	if(!((CMainFrame*)GetParentFrame())->IsFrameLessWindow()) 
	{
		CRect r;
		GetWindowRect(r);
		r.OffsetRect(-r.left, -r.top);

		CWindowDC(this).Draw3dRect(&r, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT)); 
	}
}

