#pragma once

// CStatusLabel

class CStatusLabel : public CStatic
{
	DECLARE_DYNAMIC(CStatusLabel)

private:
	bool m_fRightAlign;
	CFont m_font;

public:
	CStatusLabel(bool fRightAlign /*= false*/);
	virtual ~CStatusLabel();

	CFont& GetFont() {return m_font;}
	

	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
