// subresyncDlg.h : header file
//

#pragma once

class CharImg
{
public:
	CString m_str;

	CSize m_size;
	CAutoVectorPtr<BYTE> m_p;
	
	// feature list
	int m_topbottom;

	CharImg(DWORD* p, int pitch, CRect r, int* left, int* right, int topbottom, CString str = _T(""));
	CharImg(FILE* f);
	~CharImg();

	bool Match(CharImg* img);

	bool Write(FILE* f);
	bool Read(FILE* f);
};

class CharSegment
{
public:
	int* left;
	int* right;
	int h, srow, erow;

	CharSegment(int* left, int* right, int h, int srow, int erow);
	~CharSegment();
};

// CSubresyncDlg dialog
class CSubresyncDlg : public CDialog
{
// Construction
public:
	CSubresyncDlg(CString fn, CWnd* pParent = NULL);	// standard constructor
	virtual ~CSubresyncDlg();

	bool Open(CString fn, int CharSet = DEFAULT_CHARSET, bool fAppend = false, int timeoff = 0);
	bool Save(CString fn, exttype et, CTextFile::enc e, bool fClearImgLetterDb = false, bool fOcrDll = false);

// Dialog Data
	enum { IDD = IDD_SUBRESYNC_DIALOG };
	CListCtrl	m_list;
	CButton m_saveasbtn;
	CButton m_resetbtn;
	CButton m_editbtn;
	CButton m_exitbtn;
	BOOL m_fRender;
	CButton m_previewchk;
	BOOL m_fUnlink;
	CButton m_unlinkchk;
	CComboBox m_vslangs;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
