// ConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "..\..\filters\filters.h"
#include "..\..\..\include\matroska\matroska.h"
#include "GraphBuilder.h"
#include "ConvertPropsDlg.h"
#include "ConvertDlg.h"
#include ".\convertdlg.h"

class CTreeItem
{
public:
	CComPtr<IUnknown> pUnk;	
	CString fn;
	~CTreeItem() {}
};

// CConvertDlg dialog

CConvertDlg::CConvertDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CConvertDlg::IDD, pParent)
	, m_dwRegister(0)
	, m_fn(_T(""))
{
}

CConvertDlg::~CConvertDlg()
{
}

void CConvertDlg::AddFile(CString fn)
{
	CPath path(fn);
	path.StripPath();

	CGraphBuilder gb(m_pGB, NULL);

	CComPtr<IBaseFilter> pBF;
	if(FAILED(gb.AddSourceFilter(fn, &pBF, true)))
		return;

	int cnt = 0;
	while(S_OK == m_pCGB->RenderStream(NULL, NULL, pBF, NULL, m_pMux)) cnt++;
	if(!cnt) {MessageBeep(-1); DeleteFilter(pBF); return;}
	
	HTREEITEM hTI = m_tree.InsertItem(path);

	CTreeItem* t = new CTreeItem;
	t->pUnk = pBF;
	t->fn = fn;
	m_tree.SetItemData(hTI, (DWORD_PTR)t);

	AddFilter(hTI, pBF);

	m_tree.Expand(hTI, TVE_EXPAND);
	m_tree.EnsureVisible(hTI);
}

void CConvertDlg::AddFilter(HTREEITEM hTIParent, IBaseFilter* pBF)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinTo;
		CComPtr<IBaseFilter> pBF;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
		|| FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo
		|| !(pBF = GetFilterFromPin(pPinTo)))
			continue;

		HTREEITEM hTI = m_tree.InsertItem(_T(""), hTIParent);

		bool fTheEnd = pBF == m_pMux;

		CTreeItem* t = new CTreeItem;
		t->pUnk = fTheEnd ? (IUnknown*)pPin : (IUnknown*)pBF;
		m_tree.SetItemData(hTI, (DWORD_PTR)t);

		m_tree.Expand(hTI, TVE_EXPAND);

		UpdateTreeNode(hTI);

		if(!fTheEnd) AddFilter(hTI, pBF);
	}
	EndEnumPins
}

void CConvertDlg::DeleteFilter(IBaseFilter* pBF)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinTo;
		CComPtr<IBaseFilter> pBF;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
		|| FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo
		|| !(pBF = GetFilterFromPin(pPinTo)))
			continue;

		if(pBF != m_pMux) DeleteFilter(pBF);
	}
	EndEnumPins

	m_pGB->RemoveFilter(pBF);
}

void CConvertDlg::DeleteTreeNode(HTREEITEM hTI)
{
	if(!hTI) return;

	if(m_tree.ItemHasChildren(hTI))
	{
		HTREEITEM hChildItem = m_tree.GetChildItem(hTI);

		while(hChildItem != NULL)
		{
			HTREEITEM hNextItem = m_tree.GetNextItem(hChildItem, TVGN_NEXT);
			DeleteTreeNode(hChildItem);
			hChildItem = hNextItem;
		}
	}

	delete (CTreeItem*)m_tree.GetItemData(hTI);
	m_tree.DeleteItem(hTI);
}

void CConvertDlg::UpdateTreeNode(HTREEITEM hTI)
{
	CTreeItem* pTI = (CTreeItem*)m_tree.GetItemData(hTI);

	CString label;

	if(CComQIPtr<IPin> pPin = pTI->pUnk)
	{
		CComPtr<IPin> pPinTo;
		pPin->ConnectedTo(&pPinTo);
		label = GetPinName(pPin);
		if(!pPinTo) label = _T("[D] ") + label;

		CMediaType mt;
		if(S_OK == pPin->ConnectionMediaType(&mt))
		{
			if(mt.majortype == MEDIATYPE_Video) m_tree.SetItemImage(hTI, 1, 1);
			else if(mt.majortype == MEDIATYPE_Audio) m_tree.SetItemImage(hTI, 2, 2);
			else if(mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_Subtitle) m_tree.SetItemImage(hTI, 3, 3);
		}
	}
	else if(!pTI->fn.IsEmpty())
	{
		CPath path = pTI->fn;
		path.StripPath();
		label = (LPCTSTR)path;
	}
	else if(CComQIPtr<IBaseFilter> pBF = pTI->pUnk)
	{
		label = CString(GetFilterName(pBF));
	}

	m_tree.SetItemText(hTI, label);
}

HTREEITEM CConvertDlg::HitTest(CPoint& sp, CPoint& cp)
{
	sp = CPoint((LPARAM)GetMessagePos());
	cp = sp;
	m_tree.ScreenToClient(&cp);
	UINT flags = 0;
	HTREEITEM hTI = m_tree.HitTest(cp, &flags);
	return hTI && (flags&TVHT_ONITEM) ? hTI : NULL;
}

void CConvertDlg::ShowPopup(CPoint p)
{
	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Browse..."));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING, i++, _T("File Properties..."));
	m.AppendMenu(MF_STRING, i++, _T("Chapters..."));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		{
		CFileDialog fd(TRUE, NULL, m_fn, 
			OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY, 
			_T("Media files|*.*||"), this, 0);
		if(fd.DoModal() == IDOK) AddFile(fd.GetPathName());
		}
		break;
	case 2:
		EditProperties(CComQIPtr<IDSMPropertyBag>(m_pMux));
		break;
	}
}

void CConvertDlg::ShowFilePopup(HTREEITEM hTI, CPoint p)
{
	CTreeItem* pTI = (CTreeItem*)m_tree.GetItemData(hTI);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Remove"));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		DeleteFilter(CComQIPtr<IBaseFilter>(pTI->pUnk));
		DeleteTreeNode(hTI);
		break;
	}
}

void CConvertDlg::ShowPinPopup(HTREEITEM hTI, CPoint p)
{
	CTreeItem* pTI = (CTreeItem*)m_tree.GetItemData(hTI);

	CComQIPtr<IPin> pPin = pTI->pUnk;
	if(!pPin) return;

	CComPtr<IPin> pPinTo;
	pPin->ConnectedTo(&pPinTo);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, !pPinTo ? _T("Connect") : _T("Disconnect"));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | (!pPinTo ? MF_GRAYED : 0), i++, _T("Pin Properties..."));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		if(pPinTo) {m_pGB->Disconnect(pPinTo); m_pGB->Disconnect(pPin);}
		else if(pPinTo = GetFirstDisconnectedPin(m_pMux, PINDIR_INPUT)) m_pGB->ConnectDirect(pPin, pPinTo, NULL);
		UpdateTreeNode(hTI);
		break;
	case 2:
		EditProperties(CComQIPtr<IDSMPropertyBag>(pPinTo));
		break;
	}
}

void CConvertDlg::EditProperties(IDSMPropertyBag* pPB)
{
	CConvertPropsDlg dlg(!!CComQIPtr<IPin>(pPB), this);

	ULONG props;
	if(FAILED(pPB->CountProperties(&props)))
		props = 0;

	for(ULONG i = 0; i < props; i++)
	{
		PROPBAG2 PropBag;
		memset(&PropBag, 0, sizeof(PropBag));
		ULONG cPropertiesReturned = 0;
		if(FAILED(pPB->GetPropertyInfo(i, 1, &PropBag, &cPropertiesReturned)))
			continue;

		HRESULT hr;
		CComVariant var;
		if(SUCCEEDED(pPB->Read(1, &PropBag, NULL, &var, &hr)) && SUCCEEDED(hr))
		{
			dlg.m_props[CString(PropBag.pstrName)] = CString(var);
		}

		CoTaskMemFree(PropBag.pstrName);
	}

	if(IDOK != dlg.DoModal())
		return;

	pPB->DelAllProperties();

	POSITION pos = dlg.m_props.GetStartPosition();
	while(pos)
	{
		CString key, value;
		dlg.m_props.GetNextAssoc(pos, key, value);
		pPB->SetProperty(CStringW(key), CStringW(value));
	}
}

void CConvertDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_tree);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
}

BOOL CConvertDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;

	return __super::PreTranslateMessage(pMsg);
}

BOOL CConvertDlg::OnInitDialog()
{
	__super::OnInitDialog();

	AddAnchor(IDC_TREE1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT1, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, BOTTOM_RIGHT);
	AddAnchor(IDC_HLINE, BOTTOM_LEFT, BOTTOM_RIGHT);	
	AddAnchor(IDC_BUTTON2, BOTTOM_CENTER);
	AddAnchor(IDC_BUTTON3, BOTTOM_CENTER);
	AddAnchor(IDC_BUTTON4, BOTTOM_CENTER);

	CSize s(400, 200);
	SetMinTrackSize(s);

	m_streamtypesbm.LoadBitmap(IDB_STREAMTYPES);
	m_streamtypes.Create(16, 16, ILC_MASK|ILC_COLOR32, 0, 4);
	m_streamtypes.Add(&m_streamtypesbm, 0xffffff);
	m_tree.SetImageList(&m_streamtypes, TVSIL_NORMAL);

	GetWindowText(m_title);
	m_nIDEventStatus = SetTimer(1, 1000, NULL);

	HRESULT hr;
	m_pMux = new CDSMMuxerFilter(NULL, &hr, false, true);

	if(FAILED(m_pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2))
	|| FAILED(m_pGB.CoCreateInstance(CLSID_FilterGraph))
	|| FAILED(m_pCGB->SetFiltergraph(m_pGB))
	|| FAILED(m_pGB->AddFilter(m_pMux, L"Mux"))
	|| !(m_pMC = m_pGB) || !(m_pME = m_pGB) || !(m_pMS = m_pMux)
	|| FAILED(m_pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0))) 
	{
		MessageBeep(-1);
		SendMessage(WM_CLOSE);
		return TRUE;
	}

	AddToRot(m_pGB, &m_dwRegister);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CConvertDlg, CResizableDialog)
	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_CLICK, IDC_TREE1, OnNMClickTree1)
	ON_NOTIFY(NM_RCLICK, IDC_TREE1, OnNMRclickTree1)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateButton4)
ON_NOTIFY(NM_DBLCLK, IDC_TREE1, OnNMDblclkTree1)
END_MESSAGE_MAP()

// CConvertDlg message handlers

LRESULT CConvertDlg::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

	LONG evCode, evParam1, evParam2;
    while(m_pME && SUCCEEDED(m_pME->GetEvent(&evCode, (LONG_PTR*)&evParam1, (LONG_PTR*)&evParam2, 0)))
    {
		hr = m_pME->FreeEventParams(evCode, evParam1, evParam2);

		bool fStop = false;

        if(EC_COMPLETE == evCode)
        {
			fStop = true;
		}
		else if(EC_ERRORABORT == evCode)
		{
			fStop = true;

			CString errmsg;
			LPVOID lpMsgBuf;
			if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL))
			{
				errmsg = (LPCTSTR)lpMsgBuf;
				LocalFree(lpMsgBuf);
			}

			CString str;
			str.Format(_T("Could not complete conversion, the output file is most likely unusable.\n\nError code: 0x%08x"), evParam1);
			if(!errmsg.IsEmpty()) str += _T(" (") + errmsg + _T(")");
			AfxMessageBox(str, MB_OK);
		}

		if(fStop && m_pMC)
		{
			m_pMC->Stop();
			m_tree.EnableWindow(TRUE);
		}
	}

	return hr;
}

void CConvertDlg::OnDropFiles(HDROP hDropInfo)
{
	for(int i = 0, j = DragQueryFile(hDropInfo, 0xffffffff, 0, 0); i < j; i++)
	{
		CString fn;
		fn.ReleaseBufferSetLength(DragQueryFile(hDropInfo, i, fn.GetBuffer(MAX_PATH), MAX_PATH));

		AddFile(fn);
	}

	__super::OnDropFiles(hDropInfo);
}

void CConvertDlg::OnClose()
{
	DeleteTreeNode(m_tree.GetRootItem());

	if(m_dwRegister) RemoveFromRot(m_dwRegister);

	__super::OnClose();
}

void CConvertDlg::OnNMClickTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	CPoint sp, cp;
	HTREEITEM hTI = HitTest(sp, cp);
	if(!hTI) return;
	m_tree.SelectItem(hTI);

	*pResult = 0;
}

void CConvertDlg::OnNMRclickTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	CPoint sp, cp;
	HTREEITEM hTI = HitTest(sp, cp);

	if(hTI)
	{
		m_tree.SelectItem(hTI);

		CTreeItem* pTI = (CTreeItem*)m_tree.GetItemData(hTI);

		if(CComQIPtr<IPin> pPin = pTI->pUnk)
		{
			ShowPinPopup(hTI, sp);
		}
		else if(!m_tree.GetParentItem(hTI))
		{
			ShowFilePopup(hTI, sp);
		}
	}
	else
	{
		ShowPopup(sp);
	}

	*pResult = 0;
}

void CConvertDlg::OnNMDblclkTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint sp, cp;
	HTREEITEM hTI = HitTest(sp, cp);

	if(hTI)
	{
		CTreeItem* pTI = (CTreeItem*)m_tree.GetItemData(hTI);

		if(CComQIPtr<IPin> pPin = pTI->pUnk)
		{
			CComPtr<IPin> pPinTo;
			pPin->ConnectedTo(&pPinTo);

			if(CComQIPtr<IDSMPropertyBag> pPB = pPinTo)
				EditProperties(pPB);
		}
	}
	
	*pResult = 0;
}

void CConvertDlg::OnBnClickedButton1()
{
	UpdateData();

	CFileDialog fd(FALSE, _T(".dsm"), m_fn, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, 
		_T("DirectShow Media file|*.dsm||"), this, 0);

	if(fd.DoModal() != IDOK) return;

	NukeDownstream(m_pMux, m_pGB);

	CComPtr<IBaseFilter> pFW;
	CComQIPtr<IFileSinkFilter2> pFSF = m_pMux;
	if(pFSF) {pFW = m_pMux;}
	else {pFW.CoCreateInstance(CLSID_FileWriter); pFSF = pFW;}

	if(!pFSF
	|| FAILED(m_pGB->AddFilter(pFW, NULL))
	|| FAILED(pFSF->SetFileName(CStringW(fd.GetPathName()), NULL))
	|| FAILED(pFSF->SetMode(AM_FILE_OVERWRITE))
	|| FAILED(m_pCGB->RenderStream(NULL, NULL, m_pMux, NULL, pFW)))
	{
		m_pGB->RemoveFilter(pFW);
		AfxMessageBox(_T("Could not set output file"));
		return;
	}

	m_fn = fd.GetPathName();

	UpdateData(FALSE);
}

void CConvertDlg::OnUpdateButton1(CCmdUI* pCmdUI)
{
	OAFilterState fs;
	pCmdUI->Enable(m_pMC && SUCCEEDED(m_pMC->GetState(0, &fs)) && fs == State_Stopped);
}

void CConvertDlg::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == m_nIDEventStatus && m_pMS && m_pMC)
	{
		OAFilterState fs;
		if(SUCCEEDED(m_pMC->GetState(0, &fs)) && fs != State_Stopped)
		{
			GUID tf;
			m_pMS->GetTimeFormat(&tf);

			REFERENCE_TIME rtCur, rtDur;
			HRESULT hr = m_pMS->GetDuration(&rtDur);
			m_pMS->GetCurrentPosition(&rtCur);
			
			CString str;
			if(hr == S_OK) str.Format(_T("%.2f%%"), 1.0 * (rtCur * 100) / rtDur);
			else if(tf == TIME_FORMAT_BYTE) str.Format(_T("%.2fKB"), 1.0 * rtCur / 1024);
			else if(tf == TIME_FORMAT_MEDIA_TIME) str.Format(_T("%02d:%02d:%02d"), int(rtCur/3600000000)%60, int(rtCur/60000000)%60, int(rtCur/1000000)%60);
			else str = _T("Please Wait");
		
			SetWindowText(_T("Converting - ") + str);
		}
		else
		{
			SetWindowText(m_title);
		}
	}

	__super::OnTimer(nIDEvent);
}

void CConvertDlg::OnBnClickedButton2()
{
	if(m_pMS)
	{
		LONGLONG pos = 0;
		m_pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
	}

	if(CComQIPtr<IDSMPropertyBag> pPB = m_pMux)
	{
		pPB->SetProperty(L"APPL", L"Media Player Classic");
	}

	if(m_pMC) 
	{
		if(SUCCEEDED(m_pMC->Run()))
			m_tree.EnableWindow(FALSE);
	}
}

void CConvertDlg::OnUpdateButton2(CCmdUI* pCmdUI)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(m_pMux, nIn, nOut, nInC, nOutC);

	OAFilterState fs;
	pCmdUI->Enable(nInC > 0 && nOutC > 0 && GetDlgItem(IDC_EDIT1)->GetWindowTextLength() > 0
        && m_pMS && m_pMC && SUCCEEDED(m_pMC->GetState(0, &fs)) && fs != State_Running);
}

void CConvertDlg::OnBnClickedButton3()
{
	if(m_pMC) m_pMC->Pause();
}

void CConvertDlg::OnUpdateButton3(CCmdUI* pCmdUI)
{
	OAFilterState fs;
	pCmdUI->Enable(m_pMC && SUCCEEDED(m_pMC->GetState(0, &fs)) && fs == State_Running);
}

void CConvertDlg::OnBnClickedButton4()
{
	if(m_pMC) m_pMC->Stop();
	m_tree.EnableWindow(TRUE);
}

void CConvertDlg::OnUpdateButton4(CCmdUI* pCmdUI)
{
	OAFilterState fs;
	pCmdUI->Enable(m_pMC && SUCCEEDED(m_pMC->GetState(0, &fs)) && fs != State_Stopped);
}

void CConvertDlg::OnOK()
{
}
