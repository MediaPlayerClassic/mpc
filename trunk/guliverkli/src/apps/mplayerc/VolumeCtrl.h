#pragma once

// CVolumeCtrl

class CVolumeCtrl : public CSliderCtrl
{
	DECLARE_DYNAMIC(CVolumeCtrl)

private:
	bool m_fSelfDrawn;

public:
	CVolumeCtrl(bool fSelfDrawn = true);
	virtual ~CVolumeCtrl();

	bool Create(CWnd* pParentWnd);

	void IncreaseVolume(), DecreaseVolume();

	void SetPosInternal(int pos);

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void HScroll(UINT nSBCode, UINT nPos);
};
