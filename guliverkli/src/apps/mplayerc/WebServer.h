#pragma once

#include <afxsock.h>
#include <atlcoll.h>

#define UTF8(str) UTF16To8(TToW(str))
#define UTF8Arg(str) UrlEncode(UTF8(str))

#define CMD_SETPOS "-1"
#define CMD_SETVOLUME "-2"


class CWebServerSocket;
class CWebClientSocket;
class CMainFrame;

class CWebServer
{
	CMainFrame* m_pMainFrame;
	int m_nPort;

	DWORD ThreadProc();
    static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
    DWORD m_ThreadId;
    HANDLE m_hThread;

	CAutoPtrList<CWebClientSocket> m_clients;

	typedef bool (CWebClientSocket::*RequestHandler)(CStringA& hdr, CStringA& body, CStringA& mime);
	static CAtlMap<CString, RequestHandler, CStringElementTraits<CString> > m_internalpages;
	static CAtlMap<CString, UINT, CStringElementTraits<CString> > m_downloads;
	static CAtlMap<CStringA, CStringA, CStringElementTraits<CStringA> > m_mimes;
	CPath m_webroot;

	CAtlMap<CString, CString, CStringElementTraits<CString> > m_cgi;
	bool CallCGI(CWebClientSocket* pClient, CStringA& hdr, CStringA& body, CStringA& mime);

public:
	CWebServer(CMainFrame* pMainFrame, int nPort = 13579);
	virtual ~CWebServer();

	static void Deploy(CString dir);

	bool ToLocalPath(CString& path);
	bool LoadPage(UINT resid, CStringA& str, CString path = _T(""));

	void OnAccept(CWebServerSocket* pServer);
	void OnClose(CWebClientSocket* pClient);
	void OnRequest(CWebClientSocket* pClient, CStringA& reshdr, CStringA& resbody);
};
