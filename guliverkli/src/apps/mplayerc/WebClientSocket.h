#pragma once

class CWebServer;

class CWebClientSocket : public CAsyncSocket
{
	CWebServer* m_pWebServer;
	CMainFrame* m_pMainFrame;

	CString m_hdr;

	struct cookie_attribs {CString path, expire, domain;};
	CAtlMap<CString, cookie_attribs, CStringElementTraits<CString> > m_cookieattribs;

	void Clear();
	void Header();

protected:
	void OnReceive(int nErrorCode);
	void OnClose(int nErrorCode);

public:
	CWebClientSocket(CWebServer* pWebServer, CMainFrame* pMainFrame);
	virtual ~CWebClientSocket();

	bool SetCookie(CString name, CString value = _T(""), __time64_t expire = -1, CString path = _T("/"), CString domain = _T(""));

	typedef CAtlMap<CString, CString, CStringElementTraits<CString>, CStringElementTraits<CString> > CAtlStringMap;

	CString m_sessid;
	CString m_cmd, m_path, m_ver;
	CAtlStringMap m_hdrlines;
	CAtlStringMap m_get, m_post, m_cookie;
	CAtlStringMap m_request;

	bool OnCommand(CStringA& hdr, CStringA& body);
	bool OnIndex(CStringA& hdr, CStringA& body);
	bool OnBrowser(CStringA& hdr, CStringA& body);
	bool OnControls(CStringA& hdr, CStringA& body);
    bool OnError404(CStringA& hdr, CStringA& body);
};