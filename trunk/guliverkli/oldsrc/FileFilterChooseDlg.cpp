// FileFilterChooseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "FileFilterChooseDlg.h"
#include "..\..\DSUtil\DSUtil.h"


// CFileFilterChooseDlg dialog

IMPLEMENT_DYNAMIC(CFileFilterChooseDlg, CCmdUIDialog)
CFileFilterChooseDlg::CFileFilterChooseDlg(CWnd* pParent /*=NULL*/, CString path, CString name, CString clsid)
	: CCmdUIDialog(CFileFilterChooseDlg::IDD, pParent)
	, m_path(path)
	, m_name(name)
	, m_clsid(clsid)
	, m_iMediaType(Unknown)
	, m_fDVDDecoder(FALSE)
{
}

CFileFilterChooseDlg::~CFileFilterChooseDlg()
{
}

void CFileFilterChooseDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_path);
	DDX_Text(pDX, IDC_EDIT3, m_name);
	DDX_Text(pDX, IDC_EDIT2, m_clsid);
	DDX_Radio(pDX, IDC_RADIO1, m_iMediaType);
	DDX_Check(pDX, IDC_CHECK1, m_fDVDDecoder);
}


BEGIN_MESSAGE_MAP(CFileFilterChooseDlg, CCmdUIDialog)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateOK)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
END_MESSAGE_MAP()


// CFileFilterChooseDlg message handlers

void CFileFilterChooseDlg::OnUpdateOK(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!m_path.IsEmpty() && !m_name.IsEmpty() && GUIDFromCString(m_clsid) != GUID_NULL);
}


void CFileFilterChooseDlg::OnBnClickedButton1()
{
	UpdateData();

	CFileDialog dlg(TRUE, NULL, m_path, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY, 
		_T("DirectShow Filters (*.dll,*.ax)|*.dll;*.ax|"), this, 0);

	if(dlg.DoModal() == IDOK)
	{
		m_path = dlg.GetPathName();
		m_name = _T("");
		m_clsid = _T("");
		m_iMediaType = 0;
		m_fDVDDecoder = FALSE;

		CPath p(m_path);
		int i = p.FindFileName();
		if(i >= 0)
		{
			CString fn = m_path.Mid(i).MakeLower();

			if(fn == _T("ffdshow.ax"))
			{
				m_name = _T("ffdshow MPEG-4 Video Decoder");
				m_clsid = _T("{04FE9017-F873-410E-871E-AB91661A4EF7}");
				m_iMediaType = 1;
			}
			else if(fn == _T("divxdec.ax"))
			{
				m_name = _T("DivX Decoder Filter");
				m_clsid = _T("{78766964-0000-0010-8000-00AA00389B71}");
				m_iMediaType = 1;
			}
			else if(fn == _T("divx_c32.ax"))
			{
				m_name = _T("DivX MPEG-4 DVD Video Decompressor");
				m_clsid = _T("{82CCD3E0-F71A-11D0-9FE5-00609778AAAA}");
				m_iMediaType = 1;
			}
			else if(fn == _T("mpg4ds32.ax"))
			{
				m_name = _T("Microsoft MPEG-4 Video Decompressor");
				m_clsid = _T("{82CCD3E0-F71A-11D0-9FE5-00609778EA66}");
				m_iMediaType = 1;
			}
			else if(fn == _T("ivivideo.ax"))
			{
				m_name = _T("InterVideo Video Decoder");
				m_clsid = _T("{0246CA20-776D-11D2-8010-00104B9B8592}");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("iviaudio.ax"))
			{
				m_name = _T("InterVideo Audio Decoder");
				m_clsid = _T("{7E2E0DC1-31FD-11D2-9C21-00104B3801F6}");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("dvdvideo.ax"))
			{
				m_name = _T("Fraunhofer Video Decoder");
				m_clsid = _T("{9BC1B781-85E3-11D2-98D0-0080C84E9C39}");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("dvdaudio.ax"))
			{
				m_name = _T("Fraunhofer Audio Decoder");
				m_clsid = _T("{9BC1B780-85E3-11D2-98D0-0080C84E9C39}");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("clvsd.ax"))
			{
				m_name = _T("CyberLink Video/SP Decoder");
				m_clsid = _T("{9BC1B781-85E3-11D2-98D0-0080C84E9C39}");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("claud.ax"))
			{
				m_name = _T("CyberLink Audio Decoder");
				m_clsid = _T("{9BC1B780-85E3-11D2-98D0-0080C84E9C39}");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("mcdsmpeg.ax"))
			{
				m_name = _T("MainConcept MPEG Video Decoder");
				m_clsid = _T("{2BE4D140-6F2E-4B3A-B0BD-E880917238DC}");
				m_iMediaType = 1;
			}
			else if(fn == _T("ac3filter.ax"))
			{
				m_name = _T("Valex's AC3 Filter");
				m_clsid = _T("{A753A1EC-973E-4718-AF8E-A3F554D45C44}");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T("subtitds.ax"))
			{
				m_name = _T("Subtitler Mixer");
				m_clsid = _T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}");
				m_iMediaType = 0;
			}
			else if(fn == _T("mpgdec.ax"))
			{
				m_name = _T("Elecard MPEG2 Video Decoder");
				m_clsid = _T("{F50B3F13-19C4-11CF-AA9A-02608C9BABA2}");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE; // let's hope...
			}
/*			// TODO
			else if(fn == _T("xvid.ax"))
			{
				m_name = _T("XviD MPEG-4 Video Decoder");
				m_clsid = _T("");
				m_iMediaType = 1;
			}
			else if(fn == _T(".ax")) // nvidia mpeg2
			{
				m_name = _T("");
				m_clsid = _T("");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T(".ax")) // nvidia ac3
			{
				m_name = _T("");
				m_clsid = _T("");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T(".ax")) // sonic mpeg2 (?)
			{
				m_name = _T("");
				m_clsid = _T("");
				m_iMediaType = 1;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T(".ax")) // sonic ac3 (?)
			{
				m_name = _T("");
				m_clsid = _T("");
				m_iMediaType = 2;
				m_fDVDDecoder = TRUE;
			}
			else if(fn == _T(".ax")) // moonlight odio ac3
			{
				m_name = _T("");
				m_clsid = _T("");
				m_iMediaType = 1;
				m_fDVDDecoder = FALSE;
			}
*/
		}

		UpdateData(FALSE);
	}
}

void CFileFilterChooseDlg::OnBnClickedButton2()
{
	UpdateData();

	CComPtr<IBaseFilter> pBF;
	AfxMessageBox(SUCCEEDED(LoadExternalFilter(m_path, GUIDFromCString(m_clsid), &pBF)) ? _T("Success") : _T("Failed"));
}
