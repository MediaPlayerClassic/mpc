#pragma once

#include "..\..\subtitles\TextFile.h"

// CSaveTextFileDialog

class CSaveTextFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSaveTextFileDialog)

private:
	CTextFile::enc m_e;

public:
	CSaveTextFileDialog(
		CTextFile::enc e,
		LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, 
		LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	virtual ~CSaveTextFileDialog();

	CComboBox m_encoding;

	CTextFile::enc GetEncoding() {return(m_e);}

protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnFileNameOK();

public:
	afx_msg void OnEncodingChange();
};


