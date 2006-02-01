/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// CPPageOutput.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "MainFrm.h"
#include "PPageOutput.h"
#include "../../../include/moreuuids.h"

// CPPageOutput dialog

IMPLEMENT_DYNAMIC(CPPageOutput, CPPageBase)
CPPageOutput::CPPageOutput()
	: CPPageBase(CPPageOutput::IDD, CPPageOutput::IDD)
	, m_iDSVideoRendererType(0)
	, m_iRMVideoRendererType(0)
	, m_iQTVideoRendererType(0)
	, m_iAPSurfaceUsage(0)
	, m_iAudioRendererType(0)
	, m_fVMRSyncFix(FALSE)
	, m_iDX9Resizer(0)
	, m_fVMR9MixerMode(FALSE)
{
}

CPPageOutput::~CPPageOutput()
{
}

void CPPageOutput::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_DSSYSDEF, m_iDSVideoRendererType);
	DDX_Radio(pDX, IDC_RMSYSDEF, m_iRMVideoRendererType);
	DDX_Radio(pDX, IDC_QTSYSDEF, m_iQTVideoRendererType);
	DDX_Radio(pDX, IDC_REGULARSURF, m_iAPSurfaceUsage);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iAudioRendererType);
	DDX_Control(pDX, IDC_COMBO1, m_iAudioRendererTypeCtrl);
	DDX_Check(pDX, IDC_CHECK1, m_fVMRSyncFix);
	DDX_CBIndex(pDX, IDC_DX9RESIZER_COMBO, m_iDX9Resizer);
	DDX_Check(pDX, IDC_DSVMR9MIX, m_fVMR9MixerMode);
}


BEGIN_MESSAGE_MAP(CPPageOutput, CPPageBase)
END_MESSAGE_MAP()


// CPPageOutput message handlers

BOOL CPPageOutput::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_iDSVideoRendererType = s.iDSVideoRendererType;
	m_iRMVideoRendererType = s.iRMVideoRendererType;
	m_iQTVideoRendererType = s.iQTVideoRendererType;
	m_iAPSurfaceUsage = s.iAPSurfaceUsage;
	m_fVMRSyncFix = s.fVMRSyncFix;
	m_iDX9Resizer = s.iDX9Resizer;
	m_fVMR9MixerMode = s.fVMR9MixerMode;

	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.AddString(_T("System Default"));
	m_iAudioRendererType = 0;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
	{
		LPOLESTR olestr = NULL;
		if(FAILED(pMoniker->GetDisplayName(0, 0, &olestr)))
			continue;

		CStringW str(olestr);
		CoTaskMemFree(olestr);

		m_AudioRendererDisplayNames.Add(CString(str));

		CComPtr<IPropertyBag> pPB;
		if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
		{
			CComVariant var;
			pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);

			CString fstr(var.bstrVal);

			var.Clear();
			if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
			{			
				BSTR* pbstr;
				if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pbstr)))
				{
					fstr.Format(_T("%s (%08x)"), CString(fstr), *((DWORD*)pbstr + 1));
					SafeArrayUnaccessData(var.parray);
				}
			}

			m_iAudioRendererTypeCtrl.AddString(fstr);
		}
		else
		{
			m_iAudioRendererTypeCtrl.AddString(CString(str));
		}

		if(s.AudioRendererDisplayName == str && m_iAudioRendererType == 0)
		{
			m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
		}
	}
	EndEnumSysDev

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	m_iAudioRendererTypeCtrl.AddString(AUDRNDT_NULL_COMP);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	m_iAudioRendererTypeCtrl.AddString(AUDRNDT_NULL_UNCOMP);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	CorrectComboListWidth(m_iAudioRendererTypeCtrl, GetFont());

	UpdateData(FALSE);

	if(!AfxGetAppSettings().fXpOrBetter)
	{
		GetDlgItem(IDC_DSVMR7WIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_DSVMR7REN)->EnableWindow(FALSE);
	}

	if(!IsCLSIDRegistered(CLSID_VideoMixingRenderer9))
	{
		GetDlgItem(IDC_DSVMR9WIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_DSVMR9REN)->EnableWindow(FALSE);
		GetDlgItem(IDC_RMDX9)->EnableWindow(FALSE);
		GetDlgItem(IDC_QTDX9)->EnableWindow(FALSE);
	}

	if(!IsCLSIDRegistered(CLSID_DXR))
	{
		GetDlgItem(IDC_DSDXR)->EnableWindow(FALSE);
	}

	CreateToolTip();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageOutput::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.iDSVideoRendererType = m_iDSVideoRendererType;
	s.iRMVideoRendererType = m_iRMVideoRendererType;
	s.iQTVideoRendererType = m_iQTVideoRendererType;
	s.iAPSurfaceUsage = m_iAPSurfaceUsage;
	s.fVMRSyncFix = !!m_fVMRSyncFix;
	s.AudioRendererDisplayName = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.iDX9Resizer = m_iDX9Resizer;
	s.fVMR9MixerMode = !!m_fVMR9MixerMode;

	return __super::OnApply();
}

