#pragma once

#include "afxcmn.h"
#include "PPageBase.h"
#include "FloatEdit.h"

// CPPageAudioSwitcher dialog

class CPPageAudioSwitcher : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudioSwitcher)

private:
	CComPtr<IUnknown> m_pAudioSwitcher;
	DWORD m_pSpeakerToChannelMap[18][18];
	DWORD m_dwChannelMask;

public:
	CPPageAudioSwitcher(IUnknown* pAudioSwitcher);
	virtual ~CPPageAudioSwitcher();

// Dialog Data
	enum { IDD = IDD_PPAGEAUDIOSWITCHER };

	BOOL m_fEnableAudioSwitcher;
	BOOL m_fDownSampleTo441;
	CButton m_fDownSampleTo441Ctrl;
	BOOL m_fCustomChannelMapping;
	CButton m_fCustomChannelMappingCtrl;
	CEdit m_nChannelsCtrl;
	int m_nChannels;
	CSpinButtonCtrl m_nChannelsSpinCtrl;
	CListCtrl m_list;
	int m_tAudioTimeShift;
	CButton m_fAudioTimeShiftCtrl;
	CIntEdit m_tAudioTimeShiftCtrl;
	CSpinButtonCtrl m_tAudioTimeShiftSpin;
	BOOL m_fAudioTimeShift;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnUpdateAudioSwitcher(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChannelMapping(CCmdUI* pCmdUI);
};
