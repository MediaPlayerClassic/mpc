// ConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "..\..\filters\filters.h"
#include "..\..\..\include\matroska\matroska.h"
#include "GraphBuilder.h"
#include "ConvertPropsDlg.h"
#include "ConvertResDlg.h"
#include "ConvertDlg.h"

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
	
	CTreeItemFile* t = new CTreeItemFile(fn, pBF, m_tree, NULL);

	AddFilter(*t, pBF);

	m_tree.Expand(*t, TVE_EXPAND);
	m_tree.EnsureVisible(*t);
}

void CConvertDlg::AddFilter(HTREEITEM hTIParent, IBaseFilter* pBFParent)
{
	BeginEnumPins(pBFParent, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinTo;
		CComPtr<IBaseFilter> pBF;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
		|| FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo
		|| !(pBF = GetFilterFromPin(pPinTo)))
			continue;

		CTreeItem* t = NULL;

		if(pBF == m_pMux)
		{
			t = new CTreeItemPin(pPin, m_tree, hTIParent);
		}
		else
		{
			t = new CTreeItemFilter(pBF, m_tree, hTIParent);
			AddFilter(*t, pBF);
		}
	}
	EndEnumPins

	CTreeItem* t2 = new CTreeItemResourceFolder(m_tree, hTIParent);
	if(CComQIPtr<IDSMResourceBag> pRB = pBFParent)
	{
		for(DWORD i = 0, cnt = pRB->ResGetCount(); i < cnt; i++)
		{
			CComBSTR name, mime, desc;
			BYTE* pData = NULL;
			DWORD len = 0;
			if(FAILED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, NULL)))
				continue;

			if(len > 0)
			{
				m_pTIs.AddTail(new CTreeItemResource(CDSMResource(name, desc, mime, pData, len), m_tree, *t2));
			}

			CoTaskMemFree(pData);
		}
	}
	m_tree.Expand(*t2, TVE_EXPAND);

	CTreeItem* t3 = new CTreeItemChapterFolder(m_tree, hTIParent);
	if(CComQIPtr<IDSMChapterBag> pCB = pBFParent)
	{
		for(DWORD i = 0, cnt = pCB->ChapGetCount(); i < cnt; i++)
		{
			REFERENCE_TIME rt;
			CComBSTR name;
			if(FAILED(pCB->ChapGet(i, &rt, &name)))
				continue;

			m_pTIs.AddTail(new CTreeItemChapter(CDSMChapter(rt, name), m_tree, *t3));
		}
	}
	m_tree.Expand(*t3, TVE_EXPAND);

	m_tree.Expand(hTIParent, TVE_EXPAND);
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

void CConvertDlg::DeleteItem(HTREEITEM hTI)
{
	if(!hTI) return;

	DeleteChildren(hTI);

	CTreeItem* t = (CTreeItem*)m_tree.GetItemData(hTI);
	if(POSITION pos = m_pTIs.Find(t)) m_pTIs.RemoveAt(pos);
	delete t;
	m_tree.DeleteItem(hTI);
}

void CConvertDlg::DeleteChildren(HTREEITEM hTI)
{
	if(!hTI) return;

	if(m_tree.ItemHasChildren(hTI))
	{
		HTREEITEM hChildItem = m_tree.GetChildItem(hTI);

		while(hChildItem != NULL)
		{
			HTREEITEM hNextItem = m_tree.GetNextItem(hChildItem, TVGN_NEXT);
			DeleteItem(hChildItem);
			hChildItem = hNextItem;
		}
	}
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
	m.AppendMenu(MF_STRING, i++, _T("Add File..."));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING, i++, _T("Properties..."));

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
	CTreeItemFile* t = dynamic_cast<CTreeItemFile*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Remove"));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		DeleteFilter(t->m_pBF);
		DeleteItem(hTI);
		break;
	}
}

void CConvertDlg::ShowPinPopup(HTREEITEM hTI, CPoint p)
{
	CTreeItemPin* t = dynamic_cast<CTreeItemPin*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	if(!t->m_pPin) return;

	CComPtr<IPin> pPinTo;
	t->m_pPin->ConnectedTo(&pPinTo);

	CMediaType mtconn;
	if(pPinTo) t->m_pPin->ConnectionMediaType(&mtconn);

	CArray<CMediaType> mts;
	BeginEnumMediaTypes(t->m_pPin, pEMT, pmt)
		mts.Add(*pmt);
	EndEnumMediaTypes(pmt)	

	CMenu m;
	m.CreatePopupMenu();

	int i = 1, mtbase = 1000, mti = mtbase;

	m.AppendMenu(MF_STRING, i++, !pPinTo ? _T("Enable Stream") : _T("Disable Stream"));

	if(mts.GetCount() > 1)
	{
		m.AppendMenu(MF_SEPARATOR);
		for(int i = 0; i < mts.GetCount(); i++)
			m.AppendMenu(MF_STRING | (mts[i] == mtconn ? MF_CHECKED : 0), mti++, CMediaTypeEx(mts[i]).ToString());
	}

	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING | (!pPinTo ? MF_GRAYED : 0), i++, _T("Pin Properties..."));

	switch(i = (int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		if(pPinTo) {m_pGB->Disconnect(pPinTo); m_pGB->Disconnect(t->m_pPin);}
		else if(pPinTo = GetFirstDisconnectedPin(m_pMux, PINDIR_INPUT)) m_pGB->ConnectDirect(t->m_pPin, pPinTo, NULL);
		t->Update();
		break;
	case 2:
		EditProperties(CComQIPtr<IDSMPropertyBag>(pPinTo));
		break;
	default:
		i -= mtbase;
		if(i >= 0 && i < mts.GetCount())
		{
			if(pPinTo) {m_pGB->Disconnect(pPinTo); m_pGB->Disconnect(t->m_pPin);}
			else {pPinTo = GetFirstDisconnectedPin(m_pMux, PINDIR_INPUT);}
			HRESULT hr = m_pGB->ConnectDirect(t->m_pPin, pPinTo, &mts[i]);
			if(FAILED(hr))
			{
				AfxMessageBox(_T("Reconnection attempt failed!"));
				if(mtconn.majortype != GUID_NULL) 
					hr = m_pGB->ConnectDirect(t->m_pPin, pPinTo, &mtconn);
			}
			t->Update();
		}
		break;
	}
}

void CConvertDlg::ShowResourceFolderPopup(HTREEITEM hTI, CPoint p)
{
	CTreeItemResourceFolder* t = dynamic_cast<CTreeItemResourceFolder*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Add Resource..."));
	if(m_tree.ItemHasChildren(*t))
	{
		m.AppendMenu(MF_SEPARATOR);
		m.AppendMenu(MF_STRING, i++, _T("Remove All"));
	}

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		{
			CFileDialog fd(TRUE, NULL, NULL, 
				OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY, 
				_T("All files|*.*||"), this, 0);
			if(fd.DoModal() == IDOK) 
			{
				if(FILE* f = _tfopen(fd.GetPathName(), _T("rb")))
				{
					CDSMResource res;
					CTreeItemResource* t = new CTreeItemResource(res, m_tree, hTI);
					m_pTIs.AddTail(t);

					if(EditResource(t))
					{
						fseek(f, 0, 2);
						long size = ftell(f);
						t->m_res.data.SetSize(size);
						for(BYTE* ptr = t->m_res.data.GetData(),* end = ptr + size; 
							size > 0 && end - ptr >= size && fread(ptr, min(size, 1024), 1, f) > 0; 
							ptr += 1024, size -= 1024);
						fclose(f);
					}
					else
					{
						DeleteItem(*t);
					}
				}
				else
				{
					AfxMessageBox(_T("Cannot open file!"));
				}
			}
		}
		break;
	case 2:
		DeleteChildren(hTI);
		break;
	}
}

void CConvertDlg::ShowResourcePopup(HTREEITEM hTI, CPoint p)
{
	CTreeItemResource* t = dynamic_cast<CTreeItemResource*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, t->m_fEnabled ? _T("Disable") : _T("Enable"));
	m.AppendMenu(MF_STRING, i++, _T("Save As..."));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING, i++, _T("Resource Properties..."));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		t->m_fEnabled = !t->m_fEnabled;
		t->Update();
		break;
	case 2:
		{
			CFileDialog fd(FALSE, NULL, CString(t->m_res.name), 
				OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
				_T("All files|*.*||"), this, 0);
			if(fd.DoModal() == IDOK)
			{
				if(FILE* f = _tfopen(fd.GetPathName(), _T("wb")))
				{
					fwrite(t->m_res.data.GetData(), 1, t->m_res.data.GetSize(), f);
					fclose(f);
				}
			}
		}
		break;
	case 3:
		EditResource(t);
		break;
	}
}

bool CConvertDlg::EditProperties(IDSMPropertyBag* pPB)
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
			dlg.m_props[CString(PropBag.pstrName)] = CString(var);

		CoTaskMemFree(PropBag.pstrName);
	}

	if(IDOK != dlg.DoModal())
		return false;

	pPB->DelAllProperties();

	POSITION pos = dlg.m_props.GetStartPosition();
	while(pos)
	{
		CString key, value;
		dlg.m_props.GetNextAssoc(pos, key, value);
		pPB->SetProperty(CStringW(key), CStringW(value));
	}

	return true;
}

bool CConvertDlg::EditResource(CTreeItemResource* t)
{
	CConvertResDlg dlg(this);

	dlg.m_name = t->m_res.name;
	dlg.m_mime = t->m_res.mime;
	dlg.m_desc = t->m_res.desc;

	if(IDOK != dlg.DoModal())
		return false;

	t->m_res.name = dlg.m_name;
	t->m_res.mime = dlg.m_mime;
	t->m_res.desc = dlg.m_desc;

	t->Update();

	return true;
}

void CConvertDlg::ShowChapterFolderPopup(HTREEITEM hTI, CPoint p)
{
	CTreeItemChapterFolder* t = dynamic_cast<CTreeItemChapterFolder*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Add Chapter..."));
	if(m_tree.ItemHasChildren(*t))
	{
		m.AppendMenu(MF_SEPARATOR);
		m.AppendMenu(MF_STRING, i++, _T("Remove All"));
	}

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		// TODO
		break;
	case 2:
		DeleteChildren(hTI);
		break;
	}
}

void CConvertDlg::ShowChapterPopup(HTREEITEM hTI, CPoint p)
{
	CTreeItemChapter* t = dynamic_cast<CTreeItemChapter*>((CTreeItem*)m_tree.GetItemData(hTI));
	ASSERT(t);

	CMenu m;
	m.CreatePopupMenu();

	int i = 1;
	m.AppendMenu(MF_STRING, i++, _T("Remove"));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING, i++, _T("Chapter Properties..."));

	switch((int)m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case 1:
		DeleteItem(hTI);
		break;
	case 2:
		// TODO
		break;
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

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);

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
	m_streamtypes.Create(16, 18, ILC_MASK|ILC_COLOR32, 0, 4);
	m_streamtypes.Add(&m_streamtypesbm, 0xffffff);
	m_tree.SetImageList(&m_streamtypes, TVSIL_NORMAL);

	GetWindowText(m_title);
	m_nIDEventStatus = SetTimer(1, 1000, NULL);

	HRESULT hr;
	m_pMux = new CDSMMuxerFilter(NULL, &hr, true /* FIXME */, false);

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

void CConvertDlg::OnOK()
{
}

BEGIN_MESSAGE_MAP(CConvertDlg, CResizableDialog)
	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_CLICK, IDC_TREE1, OnNMClickTree1)
	ON_NOTIFY(NM_RCLICK, IDC_TREE1, OnNMRclickTree1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, OnNMDblclkTree1)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateButton4)
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
	HTREEITEM hTI = m_tree.GetRootItem();
	while(hTI)
	{
		HTREEITEM hTINext = m_tree.GetNextSiblingItem(hTI);
		DeleteItem(hTI);
		hTI = hTINext;
	}

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

		CTreeItem* t = (CTreeItem*)m_tree.GetItemData(hTI);

		if(dynamic_cast<CTreeItemPin*>(t))
			ShowPinPopup(hTI, sp);
		else if(dynamic_cast<CTreeItemFile*>(t))
			ShowFilePopup(hTI, sp);
		else if(dynamic_cast<CTreeItemResourceFolder*>(t))
			ShowResourceFolderPopup(hTI, sp);
		else if(dynamic_cast<CTreeItemResource*>(t))
			ShowResourcePopup(hTI, sp);
		else if(dynamic_cast<CTreeItemChapterFolder*>(t))
			ShowChapterFolderPopup(hTI, sp);
		else if(dynamic_cast<CTreeItemChapter*>(t))
			ShowChapterPopup(hTI, sp);
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
		CTreeItem* t = (CTreeItem*)m_tree.GetItemData(hTI);

		if(CTreeItemPin* t2 = dynamic_cast<CTreeItemPin*>(t))
		{
			CComPtr<IPin> pPinTo;
			t2->m_pPin->ConnectedTo(&pPinTo);

			if(CComQIPtr<IDSMPropertyBag> pPB = pPinTo)
				EditProperties(pPB);
		}
		else if(CTreeItemResource* t2 = dynamic_cast<CTreeItemResource*>(t))
		{
			EditResource(t2);
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

	if(CComQIPtr<IDSMResourceBag> pRB = m_pMux)
	{
		pRB->ResRemoveAll(0);
		POSITION pos = m_pTIs.GetHeadPosition();
		while(pos)
		{
			CTreeItem* t = m_pTIs.GetNext(pos);
			if(CTreeItemResource* t2 = dynamic_cast<CTreeItemResource*>(t))
				pRB->ResAppend(
					t2->m_res.name, t2->m_res.desc, t2->m_res.mime, 
					t2->m_res.data.GetData(), t2->m_res.data.GetSize(), 
					NULL);
		}		
	}

	// TODO: chapters

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

//
// CFilterTreeCtrl
//

CFilterTreeCtrl::CFilterTreeCtrl()
{
}

void CFilterTreeCtrl::PreSubclassWindow()
{
	EnableToolTips(TRUE);

	__super::PreSubclassWindow();
}

INT_PTR CFilterTreeCtrl::OnToolHitTest(CPoint p, TOOLINFO* pTI) const
{
	UINT nFlags;
	HTREEITEM hTI = HitTest(p, &nFlags);
	if(nFlags & TVHT_ONITEM)
	{
		CRect r;
		GetItemRect(hTI, r, TRUE);
		pTI->hwnd = m_hWnd;
		pTI->uId = (UINT)hTI;
		pTI->lpszText = LPSTR_TEXTCALLBACK;
		pTI->rect = r;
		return pTI->uId;
	}
	
	return -1;
}

BEGIN_MESSAGE_MAP(CFilterTreeCtrl, CTreeCtrl)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

BOOL CFilterTreeCtrl::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	UINT nID = pNMHDR->idFrom;

	if(nID == (UINT)m_hWnd
	&& (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND)
	|| pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND)))
		return FALSE;

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	HTREEITEM hTI = (HTREEITEM)nID;

	CString str;
	static CStringA m_strTipTextA;
	static CStringW m_strTipTextW;

	CConvertDlg::CTreeItem* t = (CConvertDlg::CTreeItem*)GetItemData(hTI);
	if(!t || !t->ToolTip(str)) return FALSE;

	m_strTipTextA = str;
	m_strTipTextW = str;

	if(pNMHDR->code == TTN_NEEDTEXTA) pTTTA->lpszText = (LPSTR)(LPCSTR)m_strTipTextA;
	else pTTTW->lpszText = (LPWSTR)(LPCWSTR)m_strTipTextW;

	*pResult = 0;

	return TRUE;    // message was handled
}

//
// CConvertDlg::CTreeItem*
//

CConvertDlg::CTreeItem::CTreeItem(CTreeCtrl& tree, HTREEITEM hTIParent) 
	: m_tree(tree)
{
	m_hTI = m_tree.InsertItem(_T(""), hTIParent);
	m_tree.SetItemData(m_hTI, (DWORD_PTR)this);
	Update();
}

CConvertDlg::CTreeItem::~CTreeItem()
{
}

void CConvertDlg::CTreeItem::SetLabel(LPCTSTR label)
{
	m_tree.SetItemText(m_hTI, label);
}

void CConvertDlg::CTreeItem::SetImage(int nImage, int nSelectedImage)
{
	m_tree.SetItemImage(m_hTI, nImage, nSelectedImage);
}

// 

CConvertDlg::CTreeItemFilter::CTreeItemFilter(IBaseFilter* pBF, CTreeCtrl& tree, HTREEITEM hTIParent) 
	: CTreeItem(tree, hTIParent)
	, m_pBF(pBF)
{
	Update();
}

void CConvertDlg::CTreeItemFilter::Update()
{
	SetLabel(CString(GetFilterName(m_pBF)));
}

//

CConvertDlg::CTreeItemFile::CTreeItemFile(CString fn, IBaseFilter* pBF, CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItemFilter(pBF, tree, hTIParent)
	, m_fn(fn)
{
	Update();
}

void CConvertDlg::CTreeItemFile::Update()
{
	CPath path = m_fn;
	path.StripPath();
	SetLabel(path);
}

bool CConvertDlg::CTreeItemFile::ToolTip(CString& str)
{
	str = m_fn;
	return true;
}

//

CConvertDlg::CTreeItemPin::CTreeItemPin(IPin* pPin, CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItem(tree, hTIParent)
	, m_pPin(pPin)
{
	Update();
}

void CConvertDlg::CTreeItemPin::Update()
{
	if(!m_pPin) {ASSERT(0); return;}

	CString label = GetPinName(m_pPin);
	if(!IsConnected()) label = _T("[D] ") + label;
	SetLabel(label);

	CMediaType mt;
	if(S_OK == m_pPin->ConnectionMediaType(&mt))
	{
		if(mt.majortype == MEDIATYPE_Video) SetImage(1, 1);
		else if(mt.majortype == MEDIATYPE_Audio) SetImage(2, 2);
		else if(mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_Subtitle) SetImage(3, 3);
	}
}

bool CConvertDlg::CTreeItemPin::ToolTip(CString& str)
{
	CMediaTypeEx mt;
	if(FAILED(m_pPin->ConnectionMediaType(&mt))) return false;
	str = mt.ToString(m_pPin);
	return true;
}

bool CConvertDlg::CTreeItemPin::IsConnected()
{
	CComPtr<IPin> pPinTo;
	return m_pPin && SUCCEEDED(m_pPin->ConnectedTo(&pPinTo)) && pPinTo;
}

//

CConvertDlg::CTreeItemResourceFolder::CTreeItemResourceFolder(CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItem(tree, hTIParent)
{
	Update();
}

void CConvertDlg::CTreeItemResourceFolder::Update()
{
	SetLabel(_T("Resources"));
}

bool CConvertDlg::CTreeItemResourceFolder::ToolTip(CString& str)
{
	if(!m_tree.ItemHasChildren(m_hTI))
		return false;

	int files = 0;
	float size = 0;

	HTREEITEM hChildItem = m_tree.GetChildItem(m_hTI);

	while(hChildItem != NULL)
	{
		HTREEITEM hNextItem = m_tree.GetNextItem(hChildItem, TVGN_NEXT);
		if(CTreeItemResource* t = dynamic_cast<CTreeItemResource*>((CTreeItem*)m_tree.GetItemData(hChildItem)))
			size += t->m_res.data.GetSize(), files++;
		hChildItem = hNextItem;
	}

	size /= 1024;
	if(size < 1024) str.Format(_T("%d file(s), %.2f KB"), files, size);
	else str.Format(_T("%d file(s), %.2f MB"), files, size/1024);

	return true;
}

//

CConvertDlg::CTreeItemResource::CTreeItemResource(const CDSMResource& res, CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItem(tree, hTIParent)
	, m_fEnabled(true)
{
	m_res = res;
	Update();
}

void CConvertDlg::CTreeItemResource::Update()
{
	CString label = CString(m_res.name);
	if(!m_fEnabled) label = _T("[D] ") + label;
	SetLabel(label);

	CStringW mime = m_res.mime;
	mime.Trim();
	mime.MakeLower();
	if(mime == L"application/x-truetype-font") SetImage(4, 4);
	else if(mime.Find(L"text/") == 0) SetImage(5, 5);
	else SetImage(6, 6);
}

bool CConvertDlg::CTreeItemResource::ToolTip(CString& str)
{
	if(!m_res.mime.IsEmpty()) str = CString(m_res.mime) + _T("\r\n\r\n");
	if(!m_res.desc.IsEmpty()) str += CString(m_res.desc);
	str.Trim();
	return true;
}

//

CConvertDlg::CTreeItemChapterFolder::CTreeItemChapterFolder(CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItem(tree, hTIParent)
{
	Update();
}

void CConvertDlg::CTreeItemChapterFolder::Update()
{
	SetLabel(_T("Chapters"));
}

//

CConvertDlg::CTreeItemChapter::CTreeItemChapter(const CDSMChapter& chap, CTreeCtrl& tree, HTREEITEM hTIParent)
	: CTreeItem(tree, hTIParent)
{
	m_chap = chap;
	Update();
}

void CConvertDlg::CTreeItemChapter::Update()
{
	m_chap.rt /= 10000000;
	int s = (int)(m_chap.rt%60);
	m_chap.rt /= 10000;
	int m = (int)(m_chap.rt%60);
	m_chap.rt /= 10000;
	int h = (int)(m_chap.rt);

	CString label;
	label.Format(_T("%02d:%02d:%02d - %s"), h, m, s, CString(m_chap.name));
	SetLabel(label);

	SetImage(7, 7);
}
