/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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

// PPageFileInfoDetails.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFileInfoDetails.h"
#include <atlbase.h>
#include "..\..\DSUtil\DSUtil.h"
#include "d3d9.h"
#include "Vmr9.h"

// CPPageFileInfoDetails dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoDetails, CPropertyPage)
CPPageFileInfoDetails::CPPageFileInfoDetails(CString fn, IFilterGraph* pFG)
	: CPropertyPage(CPPageFileInfoDetails::IDD, CPPageFileInfoDetails::IDD)
	, m_fn(fn)
	, m_pFG(pFG)
	, m_hIcon(NULL)
	, m_type(_T("Not known"))
	, m_size(_T("Not known"))
	, m_time(_T("Not known"))
	, m_res(_T("Not known"))
	, m_created(_T("Not known"))
{
}

CPPageFileInfoDetails::~CPPageFileInfoDetails()
{
	if(m_hIcon) DestroyIcon(m_hIcon);
}

void CPPageFileInfoDetails::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_type);
	DDX_Text(pDX, IDC_EDIT3, m_size);
	DDX_Text(pDX, IDC_EDIT2, m_time);
	DDX_Text(pDX, IDC_EDIT5, m_res);
	DDX_Text(pDX, IDC_EDIT6, m_created);
}

BEGIN_MESSAGE_MAP(CPPageFileInfoDetails, CPropertyPage)
END_MESSAGE_MAP()

inline int LNKO(int a, int b)
{
	if(a == 0 || b == 0)
		return(1);

	while(a != b)
	{
		if(a < b) b -= a;
		else if(a > b) a -= b;
	}

	return(a);
}

// CPPageFileInfoDetails message handlers

BOOL CPPageFileInfoDetails::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CString ext = m_fn.Left(m_fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + m_fn.Mid(m_fn.ReverseFind('.')+1);

	if(m_hIcon = LoadIcon(m_fn, false))
		m_icon.SetIcon(m_hIcon);

	if(!LoadType(ext, m_type))
		m_type = _T("Not known");

	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(m_fn, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);

		__int64 size = (__int64(wfd.nFileSizeHigh)<<32)|wfd.nFileSizeLow;
		__int64 shortsize = size;
		CString measure = _T("B");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("KB");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("MB");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("GB");
		m_size.Format(_T("%I64d%s (%I64d bytes)"), shortsize, measure, size);

		SYSTEMTIME t;
		FileTimeToSystemTime(&wfd.ftCreationTime, &t);
		TCHAR buff[256];
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &t, NULL, buff, 256);
		m_created = buff;
		m_created += _T(" ");
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &t, NULL, buff, 256);
		m_created += buff;
	}

	REFERENCE_TIME rtDur = 0;
	CComQIPtr<IMediaSeeking> pMS = m_pFG;
	if(pMS && SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur > 0)
	{
		m_time.Format(_T("%02d:%02d:%02d"), 
			int(rtDur/10000000/60/60),
			int((rtDur/10000000/60)%60),
			int((rtDur/10000000)%60));
	}

	long w = 0, h = 0, arx = 0, ary = 0, fps = 0;
	if(CComQIPtr<IBasicVideo> pBV = m_pFG)
	{
		if(SUCCEEDED(pBV->GetVideoSize(&w, &h)))
		{
			if(CComQIPtr<IBasicVideo2> pBV2 = m_pFG)
				pBV2->GetPreferredAspectRatio(&arx, &ary);
		}
		else
		{
			w = h = 0;
		}
	}

	if(w == 0 && h == 0)
	{
		BeginEnumFilters(m_pFG, pEF, pBF)
		{
			if(CComQIPtr<IBasicVideo> pBV = pBF)
			{
				pBV->GetVideoSize(&w, &h);

				if(CComQIPtr<IBasicVideo2> pBV2 = pBF)
					pBV2->GetPreferredAspectRatio(&arx, &ary);

				break;
			}
			else if(CComQIPtr<IVMRWindowlessControl> pWC = pBF)
			{
				pWC->GetNativeVideoSize(&w, &h, &arx, &ary);

				break;
			}
			else if(CComQIPtr<IVMRWindowlessControl9> pWC = pBF)
			{
				pWC->GetNativeVideoSize(&w, &h, &arx, &ary);

				break;
			}
		}
		EndEnumFilters
	}

	if(w > 0 && h > 0)
	{
		m_res.Format(_T("%d x %d"), w, h);

		int lnko = 0;
		do
		{
			lnko = LNKO(arx, ary);
			if(lnko > 1) arx /= lnko, ary /= lnko;
		}
		while(lnko > 1);

		if(arx > 0 && ary > 0 && arx*h != ary*w)
		{
			CString ar;
			ar.Format(_T(" (AR %d:%d)"), arx, ary);
			m_res += ar;
		}
	}

	m_fn.TrimRight('/');
	m_fn.Replace('\\', '/');
	m_fn = m_fn.Mid(m_fn.ReverseFind('/')+1);

	UpdateData(FALSE);

	m_pFG = NULL;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
