#pragma once

// CFloatEdit

class CFloatEdit : public CEdit
{
public:
	bool GetFloat(float& f);
	double operator = (double d);
	operator double();

	DECLARE_DYNAMIC(CFloatEdit)
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
};

// CIntEdit

class CIntEdit : public CEdit
{
public:
	DECLARE_DYNAMIC(CIntEdit)
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
};

// CHexEdit

class CHexEdit : public CEdit
{
public:
	bool GetDWORD(DWORD& dw);
	DWORD operator = (DWORD dw);
	operator DWORD();

	DECLARE_DYNAMIC(CHexEdit)
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
};
