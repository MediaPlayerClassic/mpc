#include "StdAfx.h"
#include "AviReportWnd.h"

#define IDC_DONOTSHOWAGAINCHECK 1000

CAviReportWnd::CAviReportWnd()
{
	m_font.CreateFont(12, 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 0, 0, _T("MS Shell Dlg"));
}

bool CAviReportWnd::DoModal(CAviFile* pAF, bool fHideChecked, bool fShowWarningText)
{
	CRect r, r2;
	GetDesktopWindow()->GetWindowRect(r);
	r.DeflateRect(r.Width()/4, r.Height()/4);

	LPCTSTR wndclass = AfxRegisterWndClass(
		CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS, 
		AfxGetApp()->LoadStandardCursor(IDC_ARROW), 
		(HBRUSH)(COLOR_BTNFACE + 1), 0);

	CreateEx(/*0*/WS_EX_TOOLWINDOW, wndclass, 
		_T("AVI Chunk Viewer"), WS_POPUPWINDOW|WS_CAPTION|WS_CLIPCHILDREN, r, NULL, 0);

	CRect cr;
	GetClientRect(cr);
	cr.DeflateRect(10, 10);

	SetFont(&m_font, FALSE);

	CDC* pDC = GetDC();
	CFont* pOldFont = pDC->SelectObject(&m_font);

	//

	CString str(
		_T("This AVI file was not prepared for sequential reading, the alternative ")
		_T("'Avi Splitter' will not load now and will let the default splitter handle it. ")
		_T("The complete reinterleaving of this file is strongly recommended before ")
		_T("burning it onto a slow media like cd-rom."));

	r = cr;

	pDC->DrawText(str, r, DT_WORDBREAK|DT_CALCRECT);
	r.right = cr.right;

	m_message.Create(str, WS_CHILD|WS_VISIBLE, r, this);
	m_message.SetFont(&m_font, FALSE);

	//

	r.SetRect(cr.left, r.bottom + 10, cr.right, cr.bottom);

	str = _T("Do not show this dialog again (hold Shift to re-enable it)");

	pDC->DrawText(str, r, DT_WORDBREAK|DT_CALCRECT);
	r.right = cr.right;

	m_checkbox.Create(str, WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_AUTOCHECKBOX, r, this, IDC_DONOTSHOWAGAINCHECK);
	m_checkbox.SetFont(&m_font, FALSE);

	CheckDlgButton(IDC_DONOTSHOWAGAINCHECK, fHideChecked?BST_CHECKED:BST_UNCHECKED);

	//

	if(!fShowWarningText)
	{
		m_message.ShowWindow(SW_HIDE);
		m_checkbox.ShowWindow(SW_HIDE);
		r = cr;
	}
	else
	{
		r.SetRect(cr.left, r.bottom + 10, cr.right, cr.bottom);
	}

	m_graph.Create(pAF, r, this);

	//

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	ShowWindow(SW_SHOWNORMAL);

	return !!RunModalLoop();
}

IMPLEMENT_DYNCREATE(CAviReportWnd, CWnd)

BEGIN_MESSAGE_MAP(CAviReportWnd, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CAviReportWnd::OnClose()
{
	EndModalLoop(IsDlgButtonChecked(IDC_DONOTSHOWAGAINCHECK));
	__super::OnClose();
}

//////////////

CAviPlotterWnd::CAviPlotterWnd()
{
}

bool CAviPlotterWnd::Create(CAviFile* pAF, CRect r, CWnd* pParentWnd)
{
	if(!CreateEx(WS_EX_CLIENTEDGE, _T("STATIC"), _T(""), WS_CHILD|WS_VISIBLE, r, pParentWnd, 0))
		return(false);

	GetClientRect(r);
	int w = r.Width();
	int h = r.Height();

	CDC* pDC = GetDC();
	m_dc.CreateCompatibleDC(pDC);
	m_bm.CreateCompatibleBitmap(pDC, r.Width(), r.Height());
	ReleaseDC(pDC);

	CBitmap* pOldBitmap = m_dc.SelectObject(&m_bm);
	
	m_dc.FillSolidRect(r, 0);

	{
		CPen pen(PS_DOT, 1, 0x008000);
		CPen* pOldPen = m_dc.SelectObject(&pen);
		for(int y = 0, dy = max(h/10,1); y < h; y += dy) {if(y == 0) continue; m_dc.MoveTo(0, y); m_dc.LineTo(w, y);}
		for(int x = 0, dx = max(w/10,1); x < w; x += dx) {if(x == 0) continue; m_dc.MoveTo(x, 0); m_dc.LineTo(x, w);}
		m_dc.SelectObject(pOldPen);
	}

	{
		CPen pen(PS_SOLID, 1, 0x00ff00);
		CPen* pOldPen = m_dc.SelectObject(&pen);
		m_dc.MoveTo(5, 30); 
		m_dc.LineTo(5, 2);
		m_dc.LineTo(9, 10);
		m_dc.LineTo(1, 10);
		m_dc.LineTo(5, 2);
		m_dc.MoveTo(w-30, h-5); 
		m_dc.LineTo(w-2, h-5);
		m_dc.LineTo(w-10, h-9);
		m_dc.LineTo(w-10, h-1);
		m_dc.LineTo(w-2, h-5);
		m_dc.SelectObject(pOldPen);

		m_dc.SetTextColor(0x008000);
		m_dc.TextOut(10, 10, _T("Chunk"));

		CSize size = m_dc.GetTextExtent(_T("Time"));
		m_dc.TextOut(w - size.cx - 10, h - size.cy - 10, _T("Time"));
	}

	int nmax = 0, tmax = 0;

	for(int i = 0; i < (int)pAF->m_avih.dwStreams; i++)
	{
		int cnt = pAF->m_strms[i]->cs2.GetCount();
		if(cnt <= 0) continue;
		CAviFile::strm_t::chunk2& c2 = pAF->m_strms[i]->cs2[cnt-1];
		nmax = max(nmax, c2.n);
		tmax = max(tmax, c2.t);
	}

	if(nmax > 0 && tmax > 0)
	{
		COLORREF clr[] = {0x0000ff,0xff0000,0x80ffff,0xff80ff,0xffff80,0xffffff};
/*
		CPen pen(PS_SOLID, 1, 0xffffff);
		CPen* pOldPen = m_dc.SelectObject(&pen);

		DWORD* curchunks = new DWORD[pAF->m_avih.dwStreams];

		{
			memset(curchunks, 0, sizeof(DWORD)*pAF->m_avih.dwStreams);

			CAviFile::strm_t::chunk2 cs2last = {-1, 0};

			while(1)
			{
				CAviFile::strm_t::chunk2 cs2min = {LONG_MAX, LONG_MAX};

				int n = -1;
				for(int i = 0; i < (int)pAF->m_avih.dwStreams; i++)
				{
					int curchunk = curchunks[i];
					if(curchunk >= pAF->m_strms[i]->cs2.GetSize()) continue;
					CAviFile::strm_t::chunk2& cs2 = pAF->m_strms[i]->cs2[curchunk];
					if(cs2.t < cs2min.t) {cs2min = cs2; n = i;}
				}
				if(n == -1) break;

				int x = 1.0 * w * cs2min.t / tmax;
				int y = h - 1.0 * h * cs2min.n / nmax;

				if(cs2last.t >= 0 && abs(cs2min.n - cs2last.n) >= 500)
				{
					m_dc.MoveTo(x, h);
					m_dc.LineTo(x, h - 1.0 * h * abs(cs2min.n - cs2last.n) / nmax);
				}

				curchunks[n]++;
				cs2last = cs2min;
			}
		}

		{
			memset(curchunks, 0, sizeof(DWORD)*pAF->m_avih.dwStreams);

			CAviFile::strm_t::chunk2 cs2last = {-1, 0};

			while(1)
			{
				CAviFile::strm_t::chunk2 cs2min = {LONG_MAX, LONG_MAX};

				int n = -1;
				for(int i = 0; i < (int)pAF->m_avih.dwStreams; i++)
				{
					int curchunk = curchunks[i];
					if(curchunk >= pAF->m_strms[i]->cs2.GetSize()) continue;
					CAviFile::strm_t::chunk2& cs2 = pAF->m_strms[i]->cs2[curchunk];
					if(cs2.t < cs2min.t) {cs2min = cs2; n = i;}
				}
				if(n == -1) break;

				int x = 1.0 * w * cs2min.t / tmax;
				int y = h - 1.0 * h * cs2min.n / nmax;

				m_dc.SetPixelV(x, y, clr[n%(sizeof(clr)/sizeof(clr[0]))]);

				curchunks[n]++;
				cs2last = cs2min;
			}
		}

		delete [] curchunks;

		m_dc.SelectObject(pOldPen);
*/
		for(int i = 0; i < (int)pAF->m_avih.dwStreams; i++)
		{
			CArray<CAviFile::strm_t::chunk2>& cs2 = pAF->m_strms[i]->cs2;

			CPen pen(PS_SOLID, 2, clr[i%(sizeof(clr)/sizeof(clr[0]))]);
			CPen* pOldPen = m_dc.SelectObject(&pen);

			bool fFirst = true;

			int px = -1, py = -1;

			for(int j = 0, cnt = cs2.GetSize(); j < cnt; j++)
			{
				int x = 1.0 * w * cs2[j].t / tmax;
				int y = h - 1.0 * h * cs2[j].n / nmax;
//				m_dc.SetPixelV(x, y, clr[i]);
				if(px != x || py != y)
				{
					if(fFirst) {m_dc.MoveTo(x, y); m_dc.SetPixelV(x, y, clr[i]); fFirst = false;}
					else m_dc.LineTo(x, y);
				}
				px = x;
				py = y;
			}

			m_dc.SelectObject(pOldPen);
		}
	}

	m_dc.SelectObject(pOldBitmap);

	return(true);
}

IMPLEMENT_DYNCREATE(CAviPlotterWnd, CStatic)

BEGIN_MESSAGE_MAP(CAviPlotterWnd, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CAviPlotterWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect r;
	GetClientRect(r);

	CBitmap* pOld = m_dc.SelectObject(&m_bm);
	dc.BitBlt(0, 0, r.Width(), r.Height(), &m_dc, 0, 0, SRCCOPY);
	m_dc.SelectObject(pOld);
}
