// PPageFilters.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include <afxtempl.h>
#include "PPageFilters.h"
#include "ComPropertySheet.h"
#include "RegFilterChooserDlg.h"
#include "SelectMediaType.h"
#include "GraphBuilder.h"
#include "..\..\DSUtil\DSUtil.h"
#include "..\..\..\include\moreuuids.h"

#include <initguid.h>
#include <Dmoreg.h>

// CPPageFilters dialog

IMPLEMENT_DYNAMIC(CPPageFilters, CPPageBase)
CPPageFilters::CPPageFilters()
	: CPPageBase(CPPageFilters::IDD, CPPageFilters::IDD)
	, m_iLoadType(Filter::PREFERRED)
	, m_pLastSelFilter(NULL)
{
}

CPPageFilters::~CPPageFilters()
{
}

void CPPageFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_filters);
	DDX_Radio(pDX, IDC_RADIO1, m_iLoadType);
	DDX_Control(pDX, IDC_EDIT1, m_dwMerit);
	DDX_Control(pDX, IDC_TREE2, m_tree);
}

void CPPageFilters::StepUp(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if(i < 1) return;

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i--;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);
}

void CPPageFilters::StepDown(CCheckListBox& list)
{
	int i = list.GetCurSel();
	if(i < 0 || i >= list.GetCount()-1) return;

	CString str;
	list.GetText(i, str);
	DWORD_PTR dwItemData = list.GetItemData(i);
	int nCheck = list.GetCheck(i);
	list.DeleteString(i);
	i++;
	list.InsertString(i, str);
	list.SetItemData(i, dwItemData);
	list.SetCheck(i, nCheck);
	list.SetCurSel(i);
}

Filter* CPPageFilters::GetCurFilter()
{
	int i = m_filters.GetCurSel();
	return i >= 0 ? (Filter*)m_pFilters.GetAt((POSITION)m_filters.GetItemDataPtr(i)) : (Filter*)NULL;
}

void CPPageFilters::SetupMajorTypes(CArray<GUID>& guids)
{
	guids.RemoveAll();
	guids.Add(MEDIATYPE_NULL);
	guids.Add(MEDIATYPE_Video);
	guids.Add(MEDIATYPE_Audio);
	guids.Add(MEDIATYPE_Text);
	guids.Add(MEDIATYPE_Midi);
	guids.Add(MEDIATYPE_Stream);
	guids.Add(MEDIATYPE_Interleaved);
	guids.Add(MEDIATYPE_File);
	guids.Add(MEDIATYPE_ScriptCommand);
	guids.Add(MEDIATYPE_AUXLine21Data);
	guids.Add(MEDIATYPE_VBI);
	guids.Add(MEDIATYPE_Timecode);
	guids.Add(MEDIATYPE_LMRT);
	guids.Add(MEDIATYPE_URL_STREAM);
	guids.Add(MEDIATYPE_MPEG1SystemStream);
	guids.Add(MEDIATYPE_AnalogVideo);
	guids.Add(MEDIATYPE_AnalogAudio);
	guids.Add(MEDIATYPE_MPEG2_PACK);
	guids.Add(MEDIATYPE_MPEG2_PES);
	guids.Add(MEDIATYPE_MPEG2_SECTIONS);
	guids.Add(MEDIATYPE_DVD_ENCRYPTED_PACK);
	guids.Add(MEDIATYPE_DVD_NAVIGATION);
}

void CPPageFilters::SetupSubTypes(CArray<GUID>& guids)
{
	guids.RemoveAll();
	guids.Add(MEDIASUBTYPE_None);
	guids.Add(MEDIASUBTYPE_CLPL);
	guids.Add(MEDIASUBTYPE_YUYV);
	guids.Add(MEDIASUBTYPE_IYUV);
	guids.Add(MEDIASUBTYPE_YVU9);
	guids.Add(MEDIASUBTYPE_Y411);
	guids.Add(MEDIASUBTYPE_Y41P);
	guids.Add(MEDIASUBTYPE_YUY2);
	guids.Add(MEDIASUBTYPE_YVYU);
	guids.Add(MEDIASUBTYPE_UYVY);
	guids.Add(MEDIASUBTYPE_Y211);
	guids.Add(MEDIASUBTYPE_CLJR);
	guids.Add(MEDIASUBTYPE_IF09);
	guids.Add(MEDIASUBTYPE_CPLA);
	guids.Add(MEDIASUBTYPE_MJPG);
	guids.Add(MEDIASUBTYPE_TVMJ);
	guids.Add(MEDIASUBTYPE_WAKE);
	guids.Add(MEDIASUBTYPE_CFCC);
	guids.Add(MEDIASUBTYPE_IJPG);
	guids.Add(MEDIASUBTYPE_Plum);
	guids.Add(MEDIASUBTYPE_DVCS);
	guids.Add(MEDIASUBTYPE_DVSD);
	guids.Add(MEDIASUBTYPE_MDVF);
	guids.Add(MEDIASUBTYPE_RGB1);
	guids.Add(MEDIASUBTYPE_RGB4);
	guids.Add(MEDIASUBTYPE_RGB8);
	guids.Add(MEDIASUBTYPE_RGB565);
	guids.Add(MEDIASUBTYPE_RGB555);
	guids.Add(MEDIASUBTYPE_RGB24);
	guids.Add(MEDIASUBTYPE_RGB32);
	guids.Add(MEDIASUBTYPE_ARGB1555);
	guids.Add(MEDIASUBTYPE_ARGB4444);
	guids.Add(MEDIASUBTYPE_ARGB32);
	guids.Add(MEDIASUBTYPE_A2R10G10B10);
	guids.Add(MEDIASUBTYPE_A2B10G10R10);
	guids.Add(MEDIASUBTYPE_AYUV);
	guids.Add(MEDIASUBTYPE_AI44);
	guids.Add(MEDIASUBTYPE_IA44);
	guids.Add(MEDIASUBTYPE_RGB32_D3D_DX7_RT);
	guids.Add(MEDIASUBTYPE_RGB16_D3D_DX7_RT);
	guids.Add(MEDIASUBTYPE_ARGB32_D3D_DX7_RT);
	guids.Add(MEDIASUBTYPE_ARGB4444_D3D_DX7_RT);
	guids.Add(MEDIASUBTYPE_ARGB1555_D3D_DX7_RT);
	guids.Add(MEDIASUBTYPE_RGB32_D3D_DX9_RT);
	guids.Add(MEDIASUBTYPE_RGB16_D3D_DX9_RT);
	guids.Add(MEDIASUBTYPE_ARGB32_D3D_DX9_RT);
	guids.Add(MEDIASUBTYPE_ARGB4444_D3D_DX9_RT);
	guids.Add(MEDIASUBTYPE_ARGB1555_D3D_DX9_RT);
	guids.Add(MEDIASUBTYPE_YV12);
	guids.Add(MEDIASUBTYPE_NV12);
	guids.Add(MEDIASUBTYPE_IMC1);
	guids.Add(MEDIASUBTYPE_IMC2);
	guids.Add(MEDIASUBTYPE_IMC3);
	guids.Add(MEDIASUBTYPE_IMC4);
	guids.Add(MEDIASUBTYPE_S340);
	guids.Add(MEDIASUBTYPE_S342);
	guids.Add(MEDIASUBTYPE_Overlay);
	guids.Add(MEDIASUBTYPE_MPEG1Packet);
	guids.Add(MEDIASUBTYPE_MPEG1Payload);
	guids.Add(MEDIASUBTYPE_MPEG1AudioPayload);
	guids.Add(MEDIASUBTYPE_MPEG1System);
	guids.Add(MEDIASUBTYPE_MPEG1VideoCD);
	guids.Add(MEDIASUBTYPE_MPEG1Video);
	guids.Add(MEDIASUBTYPE_MPEG1Audio);
	guids.Add(MEDIASUBTYPE_Avi);
	guids.Add(MEDIASUBTYPE_Asf);
	guids.Add(MEDIASUBTYPE_QTMovie);
	guids.Add(MEDIASUBTYPE_QTRpza);
	guids.Add(MEDIASUBTYPE_QTSmc);
	guids.Add(MEDIASUBTYPE_QTRle);
	guids.Add(MEDIASUBTYPE_QTJpeg);
	guids.Add(MEDIASUBTYPE_PCMAudio_Obsolete);
	guids.Add(MEDIASUBTYPE_PCM);
	guids.Add(MEDIASUBTYPE_WAVE);
	guids.Add(MEDIASUBTYPE_AU);
	guids.Add(MEDIASUBTYPE_AIFF);
	guids.Add(MEDIASUBTYPE_dvsd);
	guids.Add(MEDIASUBTYPE_dvhd);
	guids.Add(MEDIASUBTYPE_dvsl);
	guids.Add(MEDIASUBTYPE_dv25);
	guids.Add(MEDIASUBTYPE_dv50);
	guids.Add(MEDIASUBTYPE_dvh1);
	guids.Add(MEDIASUBTYPE_Line21_BytePair);
	guids.Add(MEDIASUBTYPE_Line21_GOPPacket);
	guids.Add(MEDIASUBTYPE_Line21_VBIRawData);
	guids.Add(MEDIASUBTYPE_TELETEXT);
	guids.Add(MEDIASUBTYPE_DRM_Audio);
	guids.Add(MEDIASUBTYPE_IEEE_FLOAT);
	guids.Add(MEDIASUBTYPE_DOLBY_AC3_SPDIF);
	guids.Add(MEDIASUBTYPE_RAW_SPORT);
	guids.Add(MEDIASUBTYPE_SPDIF_TAG_241h);
	guids.Add(MEDIASUBTYPE_DssVideo);
	guids.Add(MEDIASUBTYPE_DssAudio);
	guids.Add(MEDIASUBTYPE_VPVideo);
	guids.Add(MEDIASUBTYPE_VPVBI);
	guids.Add(MEDIASUBTYPE_ATSC_SI);
	guids.Add(MEDIASUBTYPE_DVB_SI);
	guids.Add(MEDIASUBTYPE_MPEG2DATA);
	guids.Add(MEDIASUBTYPE_MPEG2_VIDEO);
	guids.Add(MEDIASUBTYPE_MPEG2_PROGRAM);
	guids.Add(MEDIASUBTYPE_MPEG2_TRANSPORT);
	guids.Add(MEDIASUBTYPE_MPEG2_TRANSPORT_STRIDE);
	guids.Add(MEDIASUBTYPE_MPEG2_AUDIO);
	guids.Add(MEDIASUBTYPE_DOLBY_AC3);
	guids.Add(MEDIASUBTYPE_DVD_SUBPICTURE);
	guids.Add(MEDIASUBTYPE_DVD_LPCM_AUDIO);
	guids.Add(MEDIASUBTYPE_DTS);
	guids.Add(MEDIASUBTYPE_SDDS);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_PCI);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_DSI);
	guids.Add(MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER);
	guids.Add(MEDIASUBTYPE_I420);
	guids.Add(MEDIASUBTYPE_WAVE_DOLBY_AC3);
	guids.Add(MEDIASUBTYPE_WAVE_DTS);
}

BEGIN_MESSAGE_MAP(CPPageFilters, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO1, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO2, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_RADIO3, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON11, OnUpdateFilterUp)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON6, OnUpdateFilterDown)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateFilterMerit)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON13, OnUpdateFilter)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON14, OnUpdateSubType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON9, OnUpdateDeleteType)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON10, OnUpdateFilter)
	ON_BN_CLICKED(IDC_BUTTON1, OnAddRegistered)
	ON_BN_CLICKED(IDC_BUTTON2, OnRemoveFilter)
	ON_BN_CLICKED(IDC_BUTTON11, OnMoveFilterUp)
	ON_BN_CLICKED(IDC_BUTTON6, OnMoveFilterDown)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkFilter)
	ON_BN_CLICKED(IDC_BUTTON13, OnAddMajorType)
	ON_BN_CLICKED(IDC_BUTTON14, OnAddSubType)
	ON_BN_CLICKED(IDC_BUTTON9, OnDeleteType)
	ON_BN_CLICKED(IDC_BUTTON10, OnResetTypes)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO3, OnBnClickedRadio)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE2, OnNMDblclkTree2)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()


// CPPageFilters message handlers

BOOL CPPageFilters::OnInitDialog()
{
	__super::OnInitDialog();
	
	DragAcceptFiles(TRUE);

	AppSettings& s = AfxGetAppSettings();

	m_pFilters.RemoveAll();

	POSITION pos = s.filters.GetHeadPosition();
	while(pos)
	{
		CAutoPtr<Filter> f(new Filter(s.filters.GetNext(pos)));

		CString name(_T("<unknown>"));

		if(f->type == Filter::REGISTERED)
		{
			name = CGraphRegFilter(f->dispname).GetName();
			if(name.IsEmpty()) name = f->name + _T(" <not registered>");
		}
		else if(f->type == Filter::EXTERNAL)
		{
			name = f->name;
			if(f->fTemporary) name += _T(" <temporary>");
			if(!CPath(MakeFullPath(f->path)).FileExists()) name += _T(" <not found!>");
		}

		int i = m_filters.AddString(name);
		m_filters.SetCheck(i, f->fDisabled ? 0 : 1);
		m_filters.SetItemDataPtr(i, m_pFilters.AddTail(f));
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageFilters::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.filters.RemoveAll();

	for(int i = 0; i < m_filters.GetCount(); i++)
	{
		if(POSITION pos = (POSITION)m_filters.GetItemData(i))
		{
			CAutoPtr<Filter> f(new Filter(m_pFilters.GetAt(pos)));
			f->fDisabled = !m_filters.GetCheck(i);
			s.filters.AddTail(f);
		}
	}

	return __super::OnApply();
}

void CPPageFilters::OnUpdateFilter(CCmdUI* pCmdUI)
{
	if(Filter* f = GetCurFilter())
	{
		pCmdUI->Enable(!(pCmdUI->m_nID == IDC_RADIO2 && f->type == Filter::EXTERNAL));
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CPPageFilters::OnUpdateFilterUp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() > 0);
}

void CPPageFilters::OnUpdateFilterDown(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_filters.GetCurSel() >= 0 && m_filters.GetCurSel() < m_filters.GetCount()-1);
}

void CPPageFilters::OnUpdateFilterMerit(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iLoadType == Filter::MERIT);
}

void CPPageFilters::OnUpdateSubType(CCmdUI* pCmdUI)
{
	HTREEITEM node = m_tree.GetSelectedItem();
	pCmdUI->Enable(node != NULL && m_tree.GetItemData(node) == NULL);
}

void CPPageFilters::OnUpdateDeleteType(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_tree.GetSelectedItem());
}

void CPPageFilters::OnAddRegistered()
{
	CRegFilterChooserDlg dlg(this);
	if(dlg.DoModal() == IDOK)
	{
		while(!dlg.m_filters.IsEmpty())
		{
			if(Filter* f = dlg.m_filters.RemoveHead())
			{
				CAutoPtr<Filter> p(f);

				CString name = f->name;

				if(f->type == Filter::EXTERNAL)
				{
					if(!CPath(MakeFullPath(f->path)).FileExists()) name += _T(" <not found!>");
				}

				int i = m_filters.AddString(name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if(dlg.m_filters.IsEmpty())
				{
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}
			}
		}
	}
}

void CPPageFilters::OnRemoveFilter()
{
	int i = m_filters.GetCurSel();
	m_pFilters.RemoveAt((POSITION)m_filters.GetItemDataPtr(i));
	m_filters.DeleteString(i);
	if(i >= m_filters.GetCount()) i--;
	m_filters.SetCurSel(i);
	OnLbnSelchangeList1();
}

void CPPageFilters::OnMoveFilterUp()
{
	StepUp(m_filters);
}

void CPPageFilters::OnMoveFilterDown()
{
	StepDown(m_filters);
}

void CPPageFilters::OnLbnDblclkFilter()
{
	if(Filter* f = GetCurFilter())
	{
		CComPtr<IBaseFilter> pBF;
		CString name;

		if(f->type == Filter::REGISTERED)
		{
			CStringW namew;
			if(CreateFilter(f->dispname, &pBF, namew))
				name = namew;
		}
		else if(f->type == Filter::EXTERNAL)
		{
			if(SUCCEEDED(LoadExternalFilter(f->path, f->clsid, &pBF)))
				name = f->name;
		}

		if(CComQIPtr<ISpecifyPropertyPages> pSPP = pBF)
		{
			CComPropertySheet ps(name, this);
			if(ps.AddPages(pSPP) > 0)
			{
				CComPtr<IFilterGraph> pFG;
				if(SUCCEEDED(pFG.CoCreateInstance(CLSID_FilterGraph)))
					pFG->AddFilter(pBF, L"");

				ps.DoModal();
			}
		}
	}
}

void CPPageFilters::OnAddMajorType()
{
	Filter* f = GetCurFilter();
	if(!f) return;

	CArray<GUID> guids;
	SetupMajorTypes(guids);
	
	CSelectMediaType dlg(guids, MEDIATYPE_NULL, this);
	if(dlg.DoModal() == IDOK)
	{
		POSITION pos = f->guids.GetHeadPosition();
		while(pos)
		{
			if(f->guids.GetNext(pos) == dlg.m_guid) {AfxMessageBox(_T("Already on the list!")); return;}
			f->guids.GetNext(pos);
		}

		f->guids.AddTail(dlg.m_guid);
		pos = f->guids.GetTailPosition();
		f->guids.AddTail(GUID_NULL);

		CString major = GetMediaTypeName(dlg.m_guid);
		CString sub = GetMediaTypeName(GUID_NULL);

		HTREEITEM node = m_tree.InsertItem(major);
		m_tree.SetItemData(node, NULL);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)pos);
	}
}

void CPPageFilters::OnAddSubType()
{
	Filter* f = GetCurFilter();
	if(!f) return;

	HTREEITEM node = m_tree.GetSelectedItem();
	if(!node) return;

	HTREEITEM child = m_tree.GetChildItem(node);
	if(!child) return;

	POSITION pos = (POSITION)m_tree.GetItemData(child);
	GUID major = f->guids.GetAt(pos);

	CArray<GUID> guids;
	SetupSubTypes(guids);
	
	CSelectMediaType dlg(guids, MEDIASUBTYPE_NULL, this);
	if(dlg.DoModal() == IDOK)
	{
		for(child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child))
		{
			pos = (POSITION)m_tree.GetItemData(child);
			f->guids.GetNext(pos);
			if(f->guids.GetAt(pos) == dlg.m_guid) {AfxMessageBox(_T("Already on the list!")); return;}
		}

		f->guids.AddTail(major);
		pos = f->guids.GetTailPosition();
		f->guids.AddTail(dlg.m_guid);

		CString sub = GetMediaTypeName(dlg.m_guid);

		node = m_tree.InsertItem(sub, node);
		m_tree.SetItemData(node, (DWORD_PTR)pos);
	}
}

void CPPageFilters::OnDeleteType()
{
	if(Filter* f = GetCurFilter())
	{
		HTREEITEM node = m_tree.GetSelectedItem();
		if(!node) return;

		POSITION pos = (POSITION)m_tree.GetItemData(node);

		if(pos == NULL)
		{
			for(HTREEITEM child = m_tree.GetChildItem(node); child; child = m_tree.GetNextSiblingItem(child))
			{
				pos = (POSITION)m_tree.GetItemData(child);

				POSITION pos1 = pos;
				f->guids.GetNext(pos);
				POSITION pos2 = pos;
				f->guids.GetNext(pos);

				f->guids.RemoveAt(pos1);
				f->guids.RemoveAt(pos2);
			}

			m_tree.DeleteItem(node);
		}
		else
		{
			HTREEITEM parent = m_tree.GetParentItem(node);

			POSITION pos1 = pos;
			f->guids.GetNext(pos);
			POSITION pos2 = pos;
			f->guids.GetNext(pos);

			m_tree.DeleteItem(node);

			if(!m_tree.ItemHasChildren(parent))
			{
				f->guids.SetAt(pos2, GUID_NULL);
				node = m_tree.InsertItem(GetMediaTypeName(GUID_NULL), parent);
				m_tree.SetItemData(node, (DWORD_PTR)pos1);
			}
			else
			{
				f->guids.RemoveAt(pos1);
				f->guids.RemoveAt(pos2);
			}
		}
	}
}

void CPPageFilters::OnResetTypes()
{
	if(Filter* f = GetCurFilter())
	{
		f->guids.RemoveAll();
		f->guids.AddTail(&f->backup);

		m_pLastSelFilter = NULL;
		OnLbnSelchangeList1();
	}
}

void CPPageFilters::OnLbnSelchangeList1()
{
	if(Filter* f = GetCurFilter())
	{
		if(m_pLastSelFilter == f) return;
		m_pLastSelFilter = f;

		m_iLoadType = f->iLoadType;
		UpdateData(FALSE);
		m_dwMerit = f->dwMerit;

		HTREEITEM dummy_item = m_tree.InsertItem(_T(""), 0,0, NULL, TVI_FIRST);
		if(dummy_item)
			for(HTREEITEM item = m_tree.GetNextVisibleItem(dummy_item); item; item = m_tree.GetNextVisibleItem(dummy_item)) 
				m_tree.DeleteItem(item);

		CMapStringToPtr map;

		POSITION pos = f->guids.GetHeadPosition();
		while(pos)
		{
			POSITION tmp = pos;
			CString major = GetMediaTypeName(f->guids.GetNext(pos));
			CString sub = GetMediaTypeName(f->guids.GetNext(pos));

			HTREEITEM node = NULL;

			void* val = NULL;
			if(map.Lookup(major, val)) node = (HTREEITEM)val;
			else map[major] = node = m_tree.InsertItem(major);
			m_tree.SetItemData(node, NULL);

			node = m_tree.InsertItem(sub, node);
			m_tree.SetItemData(node, (DWORD_PTR)tmp);
		}

		m_tree.DeleteItem(dummy_item);

		for(HTREEITEM item = m_tree.GetFirstVisibleItem(); item; item = m_tree.GetNextVisibleItem(item)) 
			m_tree.Expand(item, TVE_EXPAND);

		m_tree.EnsureVisible(m_tree.GetRootItem());
	}
	else
	{
		m_pLastSelFilter = NULL;

		m_iLoadType = Filter::PREFERRED;
		UpdateData(FALSE);
		m_dwMerit = 0;

		m_tree.DeleteAllItems();
	}
}

void CPPageFilters::OnBnClickedRadio()
{
	UpdateData();
	if(Filter* f = GetCurFilter())
		f->iLoadType = m_iLoadType;
}

void CPPageFilters::OnEnChangeEdit1()
{
	UpdateData();
	if(Filter* f = GetCurFilter())
	{
		DWORD dw;
		if(m_dwMerit.GetDWORD(dw))
			f->dwMerit = dw;
	}
}

void CPPageFilters::OnNMDblclkTree2(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if(Filter* f = GetCurFilter())
	{
		HTREEITEM node = m_tree.GetSelectedItem();
		if(!node) return;

		POSITION pos = (POSITION)m_tree.GetItemData(node);
		if(!pos) return;

		f->guids.GetNext(pos);
		if(!pos) return;

		CArray<GUID> guids;
		SetupSubTypes(guids);

		CSelectMediaType dlg(guids, f->guids.GetAt(pos), this);
		if(dlg.DoModal() == IDOK)
		{
			f->guids.SetAt(pos, dlg.m_guid);
			m_tree.SetItemText(node, GetMediaTypeName(dlg.m_guid));
		}
	}
}

void CPPageFilters::OnDropFiles(HDROP hDropInfo)
{
	SetActiveWindow();

	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	for(UINT iFile = 0; iFile < nFiles; iFile++)
	{
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);

		CFilterMapper2 fm2(false);
		fm2.Register(szFileName);

		while(!fm2.m_filters.IsEmpty())
		{
			if(Filter* f = fm2.m_filters.RemoveHead())
			{
				CAutoPtr<Filter> p(f);
				int i = m_filters.AddString(f->name);
				m_filters.SetItemDataPtr(i, m_pFilters.AddTail(p));
				m_filters.SetCheck(i, 1);

				if(fm2.m_filters.IsEmpty())
				{
					m_filters.SetCurSel(i);
					OnLbnSelchangeList1();
				}
			}
		}
	}
	::DragFinish(hDropInfo);
}


