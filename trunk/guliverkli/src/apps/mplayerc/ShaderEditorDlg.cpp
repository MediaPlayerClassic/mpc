// ShaderEditorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "ShaderEditorDlg.h"
#include <d3dx9shader.h>
#include ".\shadereditordlg.h"

#undef SubclassWindow

// CShaderLabelComboBox

BEGIN_MESSAGE_MAP(CShaderLabelComboBox, CComboBox)
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

HBRUSH CShaderLabelComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if(nCtlColor == CTLCOLOR_EDIT)
	{
		if(m_edit.GetSafeHwnd() == NULL)
            m_edit.SubclassWindow(pWnd->GetSafeHwnd());
	}

	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CShaderLabelComboBox::OnDestroy()
{
	if(m_edit.GetSafeHwnd() != NULL)
		m_edit.UnsubclassWindow();

	__super::OnDestroy();
}

// CShaderEditorDlg dialog

CShaderEditorDlg::CShaderEditorDlg(CString label, ISubPicAllocatorPresenter* pCAP, CWnd* pParent /*=NULL*/)
	: CResizableDialog(CShaderEditorDlg::IDD, pParent)
	, m_label(label)
	, m_pCAP(pCAP)
	, m_fSplitterGrabbed(false)
{
}

CShaderEditorDlg::~CShaderEditorDlg()
{
}

void CShaderEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_labels);
	DDX_Control(pDX, IDC_COMBO2, m_targets);
	DDX_Control(pDX, IDC_EDIT1, m_srcdata);
	DDX_Control(pDX, IDC_EDIT2, m_output);
}

bool CShaderEditorDlg::HitTestSplitter(CPoint p)
{
	CRect r, rs, ro;
	m_srcdata.GetWindowRect(&rs);
	m_output.GetWindowRect(&ro);
	ScreenToClient(&rs);
	ScreenToClient(&ro);
	GetClientRect(&r);
	r.left = ro.left;
	r.right = ro.right;
	r.top = rs.bottom;
	r.bottom = ro.top;
	return !!r.PtInRect(p);
}

BEGIN_MESSAGE_MAP(CShaderEditorDlg, CResizableDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CShaderEditorDlg message handlers

BOOL CShaderEditorDlg::OnInitDialog()
{
	__super::OnInitDialog();

	AddAnchor(IDC_COMBO1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBO2, TOP_RIGHT);
	AddAnchor(IDC_BUTTON1, TOP_RIGHT);
	AddAnchor(IDC_BUTTON2, TOP_RIGHT);
	AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT2, BOTTOM_LEFT, BOTTOM_RIGHT);

	m_srcdata.SetTabStops(16);
	m_srcdata.SetMargins(16, 4);

	CRect r;
	GetWindowRect(r);
	CSize s = r.Size();
	s.cx = 400;
	s.cy = 150;
	SetMinTrackSize(s);

	CMap<CString, LPCTSTR, bool, bool> targetmap;
	targetmap[_T("ps_1_1")] = true;
	targetmap[_T("ps_1_2")] = true;
	targetmap[_T("ps_1_3")] = true;
	targetmap[_T("ps_1_4")] = true;
	targetmap[_T("ps_2_0")] = true;
	targetmap[_T("ps_2_x")] = true;
	targetmap[_T("ps_2_sw")] = true;
	targetmap[_T("ps_3_0")] = true;
	targetmap[_T("ps_3_sw")] = true;

	int nSelIndex = -1;

	for(int i = 0; ; i++)
	{
		CString str;
		str.Format(_T("%d"), i);
		str = AfxGetApp()->GetProfileString(_T("Shaders"), str);

		CList<CString> sl;
		Explode(str, sl, '|', 3);
		if(sl.GetCount() != 3) break;

		CString label = sl.RemoveHead();
		CString target = sl.RemoveHead();
		CString srcdata = sl.RemoveHead();
		srcdata.Replace(_T("\\n"), _T("\r\n"));
		srcdata.Replace(_T("\\t"), _T("\t"));

		targetmap[target] = false;

		shader_t s = {target, srcdata};
		m_shaders[label] = s;

		int nIndex = m_labels.AddString(label);

		if(m_label == label) 
			nSelIndex = nIndex;
	}

	m_labels.SetCurSel(nSelIndex);

	POSITION pos = targetmap.GetStartPosition();
	while(pos) 
	{
		CString target;
		bool b;
		targetmap.GetNextAssoc(pos, target, b);
		m_targets.AddString(target);
	}

	OnCbnSelchangeCombo1();

	m_nIDEventShader = SetTimer(1, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CShaderEditorDlg::OnCbnSelchangeCombo1()
{
	int i = m_labels.GetCurSel();
	if(i < 0) return;
	CString label;
	m_labels.GetLBText(i, label);
	if(label.IsEmpty()) return;

	shader_t s;

	if(!m_shaders.Lookup(label, s))
	{
		s.target = _T("ps_2_0");
		s.srcdata = 
            "sampler s0 : register(s0);\r\n"
			"float4 p0 : register(c0);\r\n"
            "\r\n"
			"#define width (p0[0])\r\n"
			"#define height (p0[1])\r\n"
			"#define counter (p0[2])\r\n"
            "#define clock (p0[3])\r\n"
			"\r\n"
			"float4 main(float2 t0 : TEXCOORD0) : COLOR\r\n"
			"{\r\n"
			"\tfloat4 c0 = tex2D(s0, t0);\r\n"
			"\t// TODO\r\n"
			"\treturn c0;\r\n"
			"}\r\n";
		m_shaders[label] = s;
	}

	if(m_shaders.Lookup(label, s))
	{
		m_targets.SetWindowText(s.target);
		m_srcdata.SetWindowText(s.srcdata);
	}
}

BOOL CShaderEditorDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN
	&& pMsg->hwnd == m_labels.m_edit.GetSafeHwnd())
	{
		CString label;
		m_labels.GetWindowText(label);

		shader_t s;
		m_labels.SetCurSel(!m_shaders.Lookup(label, s) 
			? m_labels.AddString(label) 
			: m_labels.FindStringExact(0, label));

		OnCbnSelchangeCombo1();
        
		return TRUE;
	}
	else if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB
	&& pMsg->hwnd == m_srcdata.GetSafeHwnd())
	{
		int nStartChar, nEndChar;
		m_srcdata.GetSel(nStartChar, nEndChar);

		if(nStartChar == nEndChar)
            m_srcdata.ReplaceSel(_T("\t"));

		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

void CShaderEditorDlg::OnBnClickedButton1()
{
	CString label;
	m_labels.GetWindowText(label);
	int nIndex = m_labels.FindStringExact(0, label);
	if(nIndex >= 0) m_labels.DeleteString(nIndex);
	m_labels.SetWindowText(_T(""));
	m_targets.SetWindowText(_T(""));
	m_srcdata.SetWindowText(_T(""));
	m_shaders.RemoveKey(label);
}

void CShaderEditorDlg::OnBnClickedButton2()
{
	CString label;
	m_labels.GetWindowText(label);
	m_targets.GetWindowText(m_shaders[label].target);
	m_srcdata.GetWindowText(m_shaders[label].srcdata);
	if(m_labels.FindStringExact(0, label) < 0)
		m_labels.SetCurSel(m_labels.AddString(label));
}

void CShaderEditorDlg::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == m_nIDEventShader)
	{
		CString srcdata, target;
		m_srcdata.GetWindowText(srcdata);
		m_targets.GetWindowText(target);

		static CString s_srcdata, s_target;
		
		srcdata.Trim(); target.Trim();
		if(!srcdata.IsEmpty() && !target.IsEmpty() 
		&& (s_srcdata != srcdata || s_target != target))
		{
			CStringA err = "Unknown Error";

			CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
			HRESULT hr = D3DXCompileShader(
				CStringA(srcdata), srcdata.GetLength(), NULL, NULL, 
				"main", CStringA(target), D3DXSHADER_DEBUG, &pShader, &pErrorMsgs, NULL);
			if(FAILED(hr))
			{
				err = "Compiler Error";

				if(pErrorMsgs)
				{
					int len = pErrorMsgs->GetBufferSize();
					strncpy(err.GetBufferSetLength(len), (const char*)pErrorMsgs->GetBufferPointer(), len);
				}
			}
			else
			{
				err = "D3DXCompileShader(..) succeeded\n";
				if(m_pCAP && FAILED(m_pCAP->SetPixelShader(CStringA(srcdata), CStringA(target), NULL, 0)))
					err += "SetPixelShader(..) failed\n";
				err += "\n";

				CComPtr<ID3DXBuffer> pDisAsm;
				hr = D3DXDisassembleShader((DWORD*)pShader->GetBufferPointer(), FALSE, NULL, &pDisAsm);
				if(pDisAsm) err += CStringA((const char*)pDisAsm->GetBufferPointer());
			}

			err.Replace("\n", "\r\n");
			m_output.SetWindowText(CString(err));
		}

		s_srcdata = srcdata;
		s_target = target;
	}

	__super::OnTimer(nIDEvent);
}

void CShaderEditorDlg::OnClose()
{
	if(IDYES == AfxMessageBox(_T("Save changes?"), MB_YESNO))
	{
		CWinApp* pApp = AfxGetApp();

		pApp->WriteProfileString(_T("Shaders"), NULL, NULL);
		pApp->WriteProfileInt(_T("Shaders"), _T("Initialized"), 1);

		for(int i = 0; i < m_labels.GetCount(); i++)
		{
			CString label;
			m_labels.GetLBText(i, label);

			shader_t s;
			if(m_shaders.Lookup(label, s))
			{
				CString str;
				str.Format(_T("%d"), i);
				s.srcdata.Replace(_T("\r"), _T(""));
				s.srcdata.Replace(_T("\n"), _T("\\n"));
				s.srcdata.Replace(_T("\t"), _T("\\t"));
				AfxGetApp()->WriteProfileString(_T("Shaders"), str, label + _T("|") + s.target + _T("|") + s.srcdata);
			}
		}
	}

	__super::OnClose();
}

void CShaderEditorDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(HitTestSplitter(point))
	{
		m_fSplitterGrabbed = true;
		SetCapture();
	}
	
	__super::OnLButtonDown(nFlags, point);
}

void CShaderEditorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(m_fSplitterGrabbed)
	{
		ReleaseCapture();
		m_fSplitterGrabbed = false;
	}

	__super::OnLButtonUp(nFlags, point);
}

void CShaderEditorDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_fSplitterGrabbed)
	{
		CRect r, rs, ro;
		GetClientRect(&r);
		m_srcdata.GetWindowRect(&rs);
		m_output.GetWindowRect(&ro);
		ScreenToClient(&rs);
		ScreenToClient(&ro);

		int dist = ro.top - rs.bottom;
		int avgdist = dist / 2;

		rs.bottom = min(max(point.y, rs.top + 40), ro.bottom - 40) - avgdist;
		ro.top = rs.bottom + dist;
		m_srcdata.MoveWindow(&rs);
		m_output.MoveWindow(&ro);

		int div = 100 * ((rs.bottom + ro.top) / 2) / (ro.bottom - rs.top);

		RemoveAnchor(IDC_EDIT1);
		RemoveAnchor(IDC_EDIT2);
		AddAnchor(IDC_EDIT1, TOP_LEFT, CSize(100, div)/*BOTTOM_RIGHT*/);
		AddAnchor(IDC_EDIT2, CSize(0, div)/*BOTTOM_LEFT*/, BOTTOM_RIGHT);
	}

	__super::OnMouseMove(nFlags, point);
}

BOOL CShaderEditorDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);
	if(HitTestSplitter(p))
	{
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return TRUE;
	}

	return __super::OnSetCursor(pWnd, nHitTest, message);
}
