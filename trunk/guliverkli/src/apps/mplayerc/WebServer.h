#pragma once

#include <afxsock.h>
#include <atlcoll.h>

class CServerSocket;
class CClientSocket;
class CMainFrame;

class CWebServer
{
	CMainFrame* m_pMainFrm;
	int m_nPort;

	DWORD ThreadProc();
    static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
    DWORD m_ThreadId;
    HANDLE m_hThread;

	CAutoPtrList<CClientSocket> m_clients;

	typedef bool (CWebServer::*RequestHandler)(CClientSocket* pClient, CStringA& hdr, CStringA& body);
	CMap<CString,LPCTSTR,CWebServer::RequestHandler,CWebServer::RequestHandler> m_internalpages;
	CMap<CString,LPCTSTR,UINT,UINT> m_downloads;
	CMap<CStringA,LPCSTR,CStringA,CStringA> m_mimes;
	CPath m_wwwroot;

	bool HandlerCommand(CClientSocket* pClient, CStringA& hdr, CStringA& body);
	bool HandlerIndex(CClientSocket* pClient, CStringA& hdr, CStringA& body);
	bool HandlerBrowser(CClientSocket* pClient, CStringA& hdr, CStringA& body);
	bool HandlerControls(CClientSocket* pClient, CStringA& hdr, CStringA& body);
	bool Handler404(CClientSocket* pClient, CStringA& hdr, CStringA& body);

public:
	CWebServer(CMainFrame* pMainFrm, int nPort = 13579);
	virtual ~CWebServer();

	void OnAccept(CServerSocket* pServer);
	void OnClose(CClientSocket* pClient);
	void OnRequest(CClientSocket* pClient, CStringA& reshdr, CStringA& resbody);
};
