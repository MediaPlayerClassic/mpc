#pragma once

#include <afxcmn.h>
#include "IDirectVobSub.h"

class CDVSBasePPage : public CBasePropertyPage
{
public:
	// we have to override these to use external, resource-only dlls
	STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
	STDMETHODIMP Activate(HWND hwndParent, LPCRECT pRect, BOOL fModal);

protected:
	CComQIPtr<IDirectVobSub2> m_pDirectVobSub;

	virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {return(false);}
	virtual void UpdateObjectData(bool fSave) {}
	virtual void UpdateControlData(bool fSave) {}

protected:
    CDVSBasePPage(TCHAR* pName, LPUNKNOWN lpunk, int DialogId, int TitleId);

	bool m_fDisableInstantUpdate;

private:
    BOOL m_bIsInitialized;

    HRESULT OnConnect(IUnknown* pUnknown), OnDisconnect(), OnActivate(), OnDeactivate(), OnApplyChanges();
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	bool m_fAttached;
	void AttachControls(), DetachControls();

	CMap<UINT, UINT&, CWnd*, CWnd*> m_controls;

protected:
	void BindControl(UINT id, CWnd& control);
};

class CDVSMainPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSMainPPage(LPUNKNOWN lpunk);
	virtual ~CDVSMainPPage();

	void FreeLangs(), AllocLangs(int nLangs);

	WCHAR m_fn[MAX_PATH];
	int m_iSelectedLanguage, m_nLangs;
	WCHAR** m_ppLangs;
	bool m_fOverridePlacement;
	int	m_PlacementXperc, m_PlacementYperc;
	STSStyle m_defStyle;
	bool m_fOnlyShowForcedVobSubs;

	CEdit m_fnedit;
	CComboBox m_langs;
	CButton m_oplacement;
	CSpinButtonCtrl m_subposx, m_subposy;
	CButton m_font, m_forcedsubs;
};

class CDVSGeneralPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSGeneralPPage(LPUNKNOWN lpunk);

	int m_HorExt, m_VerExt, m_ResX2, m_ResX2minw, m_ResX2minh;
	int m_LoadLevel;
	bool m_fExternalLoad, m_fWebLoad, m_fEmbeddedLoad;

	CComboBox m_verext;
	CButton m_mod32fix;
	CComboBox m_resx2;
	CSpinButtonCtrl m_resx2w, m_resx2h;
	CComboBox m_load;
	CButton m_extload, m_webload, m_embload;
};

class CDVSMiscPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSMiscPPage(LPUNKNOWN lpunk);

	bool m_fFlipPicture, m_fFlipSubtitles, m_fHideSubtitles, m_fOSD, m_fDoPreBuffering, m_fReloaderDisabled, m_fSaveFullPath;

	CButton m_flippic, m_flipsub, m_hidesub, m_showosd, m_prebuff, m_autoreload, m_savefullpath, m_instupd;
};

class CDVSTimingPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSTimingPPage(LPUNKNOWN lpunk);

	int m_SubtitleSpeedMul, m_SubtitleSpeedDiv, m_SubtitleDelay;
	bool m_fMediaFPSEnabled;
	double m_MediaFPS;

	CButton m_modfps;
	CEdit m_fps;
	CSpinButtonCtrl m_subdelay, m_subspeedmul, m_subspeeddiv;
};

class CDVSAboutPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CDVSAboutPPage(LPUNKNOWN lpunk);
};

class CDVSZoomPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSZoomPPage(LPUNKNOWN lpunk);

	NORMALIZEDRECT m_rect;

	CSpinButtonCtrl m_posx, m_posy, m_scalex, m_scaley;
};

class CDVSColorPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSColorPPage(LPUNKNOWN lpunk);

	CListBox m_preflist, m_dynchglist;
	CButton m_forcergb;
};

class CDVSPathsPPage : public CDVSBasePPage
{
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

protected:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void UpdateControlData(bool fSave);
	virtual void UpdateObjectData(bool fSave);

private:
    CDVSPathsPPage(LPUNKNOWN lpunk);

	CStringArray m_paths;

	CListBox m_pathlist;
	CEdit m_path;
	CButton m_browse, m_remove, m_add;
};
